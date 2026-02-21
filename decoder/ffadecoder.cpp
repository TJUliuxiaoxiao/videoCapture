#include "ffadecoder.h"
#include "queue/ffaframequeue.h"
#include "resampler/ffaresampler.h"
/*实现了一个基于 FFmpeg 的音频解码器类 FFADecoder，
 * 它的核心职责是从给定的 AVStream 中解码音频数据，
 * 可选地进行重采样，并将解码后的帧放入一个线程安全的帧队列中，
 * 供后续处理（如播放、编码等）使用。*/
FFADecoder::FFADecoder():m_stop(false){}

FFADecoder::~FFADecoder()
{
    close();
}



void FFADecoder::init(AVStream *stream_,FFAFrameQueue* frmQueue_)
{
    std::lock_guard<std::mutex> lock(mutex);
    m_stop = false;//m_stop原子布尔标志，用于安全地通知解码线程停止
    stream = stream_;//要解码的音频流
    frmQueue = frmQueue_;//用于存放解码后的AVFrame*
    if(stream->codecpar==nullptr){
        return;//
    }
    const AVCodec* codec =
        avcodec_find_decoder(stream->codecpar->codec_id);
    //调用avcodec_find_decoder获得编码器
    //根据stream->codecpar->codec_id

    if(codec == nullptr){
        std::cerr<<"找不到视频解码器"<<std::endl;
        return;
    }

    //分配解码器上下文
    codecCtx = avcodec_alloc_context3(codec);

    if(codecCtx==nullptr){
        std::cerr<<"分配解码器上下文失败"<<std::endl;
        avcodec_free_context(&codecCtx);
        return;//失败则释放并返回
    }

    //复制参数,将流中的编码参数复制到codecCtx上下文，失败则打印错误、释放上下文
    int ret = avcodec_parameters_to_context(codecCtx,stream->codecpar);
    if(ret<0){
        printError(ret);
        avcodec_free_context(&codecCtx);
        return;
    }
    AVDictionary* codec_options = nullptr;

    //打开解码器,avcodec_open2，这里创建空字典codec_options，失败则释放上下文
    ret = avcodec_open2(codecCtx,codec,&codec_options);
    if(ret<0)
    {
        printError(ret);
        avcodec_free_context(&codecCtx);
        return;
    }
}


void FFADecoder::flushDecoder()
{
    //清空codecCtx的缓冲区
    avcodec_flush_buffers(codecCtx);
}

FFAudioPars *FFADecoder::getAudioPars()
{
    return swraPars;
}

int FFADecoder::getTotalsec()
{
    return static_cast<int>(0,5 + stream->duration*av_q2d(stream->time_base));
}

void FFADecoder::wakeAllThread()
{
    frmQueue->wakeAllThread();
}

void FFADecoder::stop()
{
    m_stop = true;
}

void FFADecoder::enqueueNull()
{
    frmQueue->enqueueNull();
}

void FFADecoder::flushQueue()
{
    frmQueue->flushQueue();
}

void FFADecoder::close()
{
    decode(nullptr);
    stop();
    std::lock_guard<std::mutex> lock(mutex);
    if(codecCtx){
        avcodec_free_context(&codecCtx);
    }
    if(aPars){
        delete aPars;
        aPars = nullptr;
    }
    if(swraPars){
        delete swraPars;
        swraPars = nullptr;
    }
    if(resampler){
        delete resampler;
        resampler = nullptr;
    }
}

AVCodecContext *FFADecoder::getCodecCtx()
{
    return codecCtx;
}
AVStream* FFADecoder::getStream()
{
    return stream;
}


//解码
void FFADecoder::decode(AVPacket *packet)
{//该函数负责将一个AVPacket包含一个音频帧的压缩数据
    std::lock_guard<std::mutex> lock(mutex);
    if(codecCtx ==nullptr){
        return;
    }
    int ret = avcodec_send_packet(codecCtx,packet);
    //发送包,如果返回AVERROR(EAGAIN),属于正常,需要先接收帧，其他错误则打印错误并释放上下文
    if(ret<0&&ret!=AVERROR(EAGAIN)){
        printError(ret);
        avcodec_free_context(&codecCtx);
        return;
    }
    AVFrame* frame = av_frame_alloc();
    while(ret>=0){
        //循环接收帧:
        ret = avcodec_receive_frame(codecCtx,frame);
        if(ret<0){
            if(ret==AVERROR_EOF){
                break;
            }
            else if(ret ==AVERROR(EAGAIN)){
                break;
            }else{
                printError(ret);
                av_frame_free(&frame);
                avcodec_free_context(&codecCtx);
                return;
            }
        }else{
            //如果没有初始化音频参数(aPars==nullptr)，
            //则调用initAudioPars初始化原始参数和目标参数，
            //并根据是否需要重采样创建重采样器
            if(aPars == nullptr){
                aPars = new FFAudioPars();
                swraPars = new FFAudioPars();
                initAudioPars(frame);
                if(aPars->aFormatEnum!=swraPars->aFormatEnum){
                    resampler = new FFAResampler();
                    initResampler();
                    printFmt();
                }
            }
            //如果重采样器存在
            if(resampler){
                AVFrame* swrFrame = nullptr;
                resampler->resample(frame,&swrFrame);
                //调用resampler->resample获得重采样后的帧swrFrame
                if(m_stop.load(std::memory_order_acquire)){
                    av_frame_unref(swrFrame);
                    av_frame_free(&swrFrame);
                    m_stop.store(false,std::memory_order_release);
                    break;//检查停止标志，若停止则释放帧并跳出循环
                }
                else{
                    //克隆后将加入的帧加入队列
                    AVFrame* decodeFrame = av_frame_clone(swrFrame);
                    if(frmQueue!=nullptr)
                        frmQueue->enqueue(decodeFrame);
                    av_frame_unref(swrFrame);
                    av_frame_free(&swrFrame);
                }
            }
            else{//没有重采样器,正常进行
                if(m_stop.load(std::memory_order_acquire)){
                    av_frame_unref(frame);
                    m_stop.store(false,std::memory_order_release);
                    break;
                }
                else{
                    //正常进行解码将帧放入frmQueue;
                    AVFrame* decodeFrame = av_frame_clone(frame);
                    if(frmQueue!=nullptr)
                        frmQueue->enqueue(decodeFrame);
                    av_frame_unref(frame);
                }
            }
        }
    }
    av_frame_free(&frame);
}

void FFADecoder::printError(int ret)
{
    char errorBuffer[AV_ERROR_MAX_STRING_SIZE];
    int res = av_strerror(ret,errorBuffer,sizeof errorBuffer);
    if(res<0){
        std::cerr<<"Unknow Error!"<<std::endl;
    }else{
        std::cerr<<"Error:"<<errorBuffer<<std::endl;
    }
}

void FFADecoder::initAudioPars(AVFrame *frame)
{
    aPars->timeBase = stream->time_base;
    aPars->nbChannels = frame->ch_layout.nb_channels;
    aPars->aFormatEnum = codecCtx->sample_fmt;
    aPars->sampleSize = av_get_bytes_per_sample(codecCtx->sample_fmt);
    aPars->sampleRate = frame->sample_rate;
    swraPars = new FFAudioPars();
    memcpy(swraPars,aPars,sizeof(FFAudioPars));
    swraPars->aFormatEnum = AV_SAMPLE_FMT_FLTP;
    swraPars->sampleSize = av_get_bytes_per_sample(AV_SAMPLE_FMT_FLTP);
    swraPars->sampleRate = 48000;
}

void FFADecoder::initResampler()
{
    resampler->init(aPars,swraPars);
}

void FFADecoder::printFmt()
{
    std::cout<<"audio format:"
              <<av_get_sample_fmt_name(aPars->aFormatEnum)<<std::endl;
    std::cout<<"sample_rate:"<<aPars->sampleRate<<std::endl;
    std::cout<<"channels:"<<aPars->nbChannels<<std::endl;
    std::cout<<"time_base:"<<aPars->timeBase.num<<"/"
              <<aPars->timeBase.den<<std::endl;

    std::cout<<"============================================="<<std::endl;
    std::cout<<"audio format:"
              <<av_get_sample_fmt_name(swraPars->aFormatEnum)<<std::endl;
    std::cout<<"sample_rate:"<<swraPars->sampleRate<<std::endl;
    std::cout<<"channels:"<<swraPars->nbChannels<<std::endl;
    std::cout<<"time_base:"<<swraPars->timeBase.num<<"/"
              <<swraPars->timeBase.den<<std::endl;
}
