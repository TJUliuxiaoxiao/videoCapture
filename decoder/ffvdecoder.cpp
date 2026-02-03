#include "ffvdecoder.h"
#include<iostream>
#include "queue/ffvframequeue.h"
#include "resampler/ffvresampler.h"
FFVDecoder::FFVDecoder():m_stop(false) {}
FFVDecoder::~FFVDecoder()
{
    close();
}

void FFVDecoder::init(AVStream *stream_,FFVFrameQueue* frmQueue_)
{
    std::lock_guard<std::mutex> lock(mutex);
    stream = stream_;
    frmQueue = frmQueue_;
    m_stop.store(false,std::memory_order_release);

    //查找解码器codec

    const AVCodec* codec = nullptr;
    if(stream->codecpar==nullptr){
        return;
    }
    //如果是H.264，使用特定的"h264"解码器（可能是软解）;
    if(stream->codecpar->codec_id == AV_CODEC_ID_H264){
        codec = avcodec_find_decoder_by_name("h264");
        hardDecodeFlag = false;//标记为软解
    }else{
        //其他编码方式使用默认查找方式
        codec = avcodec_find_decoder(stream->codecpar->codec_id);
    }
    if(codec == nullptr){
        std::cerr<<"找不到视频解码器"<<std::endl;
        return;
    }

    //创建编解码器上下文
    //这个函数是 FFmpeg 编解码流程中的第一步 avcodec_alloc_context3
    /*函数原型：AVCodecContext *avcodec_alloc_context3(const AVCodec *codec);
    参数：
    codec: 指向AVCodec结构的指针，通常表示一个编解码器。如果非空，函数会为该编解码器分配私有数据并设置默认值。
    如果为空，则只分配AVCodecContext结构体，并设置一些通用的默认值，但不会初始化编解码器特定的默认值。

    返回值：
    成功时返回一个指向AVCodecContext的指针，该结构体已被设置为默认值；失败时返回NULL。*/
    codecCtx = avcodec_alloc_context3(codec);//为解码器分配上下文
    if(codecCtx==nullptr){
        std::cerr<<"分配解码器上下文失败"<<std::endl;
        avcodec_free_context(&codecCtx);
        return;
    }
    /*函数行为：avcodec_parameters_to_context(AVCodecContext *codecCtx,
                                  const struct AVCodecParameters *par)
    将传入的AVCodecParameters（par）中的字段复制到AVCodecContext（codecCtx）中。
    对于codecCtx中已经分配内存的字段（例如extradata），如果par中有对应的字段，则会先释放codec中的原有内存，然后复制par中的内容（深拷贝）。
    如果codecCtx中的字段在par中没有对应项，则这些字段不会被修改
    参数：
    codecCtx：目标AVCodecContext，必须已经被分配（例如通过avcodec_alloc_context3分配）。
    par：源AVCodecParameters，包含从容器中读取的编解码器参数。
    返回值：
    成功时返回0或正数，失败时返回负的错误码（AVERROR）。*/
    int ret = avcodec_parameters_to_context(codecCtx,stream->codecpar);//复制参数
    if(ret<0){
        printError(ret);
        avcodec_free_context(&codecCtx);
        return;
    }

    //
    AVDictionary* codec_options = nullptr;//解码方式
    if(!hardDecodeFlag){//软解码
        codecCtx->thread_count = av_cpu_count();//使用多线程,线程数=cpu核心数
        av_dict_set(&codec_options,"fast","1",0);
        //用快速解码模式
    }else{
        av_dict_set(&codec_options,"low_latency","1",0);//硬解码方式,低延迟模式
    }
    //打开解码器
    /*
    int avcodec_open2(AVCodecContext *avctx, const AVCodec *codec, AVDictionary **options);
    avctx：指向已经通过avcodec_alloc_context3()分配过的AVCodecContext指针。
    codec：要打开的编解码器。通常通过avcodec_find_decoder()或avcodec_find_encoder()等函数获取。
    如果之前调用avcodec_alloc_context3时已经传入了非NULL的codec，那么这里传入的codec必须为NULL或者与之前相同的codec。
    options：一个指向AVDictionary指针的指针，用于传递编解码器私有选项。可以传入NULL。
    函数返回时，这个字典会被修改，只保留那些无法被识别的选项（即未被使用的选项）。
    */
    /*分配编解码器上下文：avcodec_alloc_context3()。
    设置必要的参数（如宽、高、像素格式等）。这些参数可以从媒体流中复制（使用avcodec_parameters_to_context）或手动设置。
    准备选项字典（可选）。
    调用avcodec_open2打开编解码器。*/
    ret = avcodec_open2(codecCtx,codec,&codec_options);
    if(ret<0){
        printError(ret);
        avcodec_free_context(&codecCtx);
        return;
    }
    av_dict_free(&codec_options);//释放选项词典
}

void FFVDecoder::flushDecoder()
{
    avcodec_flush_buffers(codecCtx);
}

int FFVDecoder::getTotalSec()//获取视频流的总时长
{
    //avq2d:将AVRational类型的时间基转换为double类型的秒数。
    //例如，如果time_base是{1, 1000}，那么av_q2d返回0.001
    //将流的时长（以时间基为单位）乘以时间基的秒数，得到流的总时长（秒）。
    return static_cast<int>(stream->duration* av_q2d(stream->time_base));
}

FFVideoPars* FFVDecoder::getVideoPars()
{
    return swsvPars;
}
AVCodecContext* FFVDecoder::getCodecCtx()
{
    return codecCtx;
}
AVStream* FFVDecoder::getStream()
{
    return stream;
}

void FFVDecoder::enqueueNull()
{
    frmQueue->enqueueNull();
}
void FFVDecoder::wakeAllThread()
{
    frmQueue->wakeAllThread();
}
void FFVDecoder::stop()
{
    m_stop.store(true,std::memory_order_release);
}
void FFVDecoder::flushQueue()
{
    frmQueue->flushQueue();
}

void FFVDecoder::close()
{
    decode(nullptr);
    stop();
    std::lock_guard<std::mutex> lock(mutex);
    if(codecCtx){
        avcodec_free_context(&codecCtx);
    }
    if(vPars){
        delete vPars;
        vPars = nullptr;
    }
    if(resampler){
        delete resampler;
        resampler = nullptr;
    }
    if(swsvPars){
        delete swsvPars;
        swsvPars = nullptr;
    }
    std::cout<<"video Decoder close!"<<std::endl;
}

void FFVDecoder::initResampler()
{
    resampler->init(vPars,swsvPars);
}

void FFVDecoder::initVideoPars(AVFrame* frame)
{
    vPars->timeBase = stream->time_base;
    vPars->pixFmtEnum = codecCtx->pix_fmt;
    vPars->frameRate = codecCtx->framerate;
    if(codecCtx->framerate.den == 0||
        codecCtx->framerate.num==0)
    {
        vPars->frameRate = stream->avg_frame_rate;
        codecCtx->framerate = stream->avg_frame_rate;//调整frameRate
    }
    vPars->width = frame->width;
    vPars->height = frame->height;
    memcpy(swsvPars,vPars,sizeof(FFVideoPars));
    swsvPars->pixFmtEnum = AV_PIX_FMT_YUV420P;
}

void FFVDecoder::decode(AVPacket *packet)
{
    std::lock_guard<std::mutex> lock(mutex);//使用互斥锁保证线程安全，防止多个线程同时访问解码器
    if(codecCtx == nullptr){
        return;//检查解码器上下文是否已初始化，未初始化则直接返回
    }
   /* 参数说明
        avctx：编解码器上下文
        avpkt：输入数据包
        典型内容：单个视频帧或多个完整音频帧
        所有权：调用者保留所有权，解码器不会写入
        引用机制：解码器可能引用或复制数据
        完全消费：与旧API不同，数据包总是被完全消费
        多帧处理：包含多帧时需要多次调用avcodec_receive_frame()
        刷新包：NULL或空包表示流结束，第一次发送返回成功，后续返回AVERROR_EOF*/
    /*0：成功
    AVERROR(EAGAIN)：当前状态不接受输入，需要先读取输出帧
    AVERROR_EOF：解码器已刷新，无法接收新包（或发送了多个刷新包）
    AVERROR(EINVAL)：编解码器未打开、是编码器或需要刷新
    AVERROR(ENOMEM)：内存分配失败
    其他负值：实际解码错误*/
    //int avcodec_send_packet(AVCodecContext *avctx, const AVPacket *avpkt);
    //用于向解码器提供原始的压缩数据包进行解码，是 FFmpeg "新API"（发送/接收模式）的一部分
    int ret = avcodec_send_packet(codecCtx,packet);
    //avcodec_send_packet()：将压缩数据包发送给解码器
    if(ret <0&&ret!=AVERROR(EAGAIN)){
        printError(ret);
        avcodec_free_context(&codecCtx);
        return;
    }
    AVFrame* frame = av_frame_alloc();
    //分配内存用于存储解码后的视频帧

    // 解码循环
    while(ret>=0){//依次解码压缩数据包packet
        if(m_stop.load(std::memory_order_acquire)){
            break;
        }
        ret = avcodec_receive_frame(codecCtx,frame);//从解码器获取解码后的帧
        if(ret<0){
            if(ret==AVERROR_EOF){
                break;
            }else if(ret == AVERROR(EAGAIN)){
                continue;
            }else{
                printError(ret);
                av_frame_free(&frame);
                avcodec_free_context(&codecCtx);
                return;
            }
        }
        else{
            if(vPars == nullptr){
                vPars = new FFVideoPars();
                swsvPars = new FFVideoPars();
                initVideoPars(frame);
                if(vPars->pixFmtEnum!=swsvPars->pixFmtEnum){//如果像素格式不统一
                    resampler = new FFVResampler();//重采样
                    initResampler();
                }
            }
            if(resampler){//格式转换路径
                AVFrame* swsFrame = nullptr;
                resampler->resample(frame,&swsFrame);
                av_frame_unref(frame);
                if(m_stop.load(std::memory_order_acquire)){
                    av_frame_unref(swsFrame);
                    av_frame_free(&swsFrame);
                    m_stop.store(false,std::memory_order_release);
                    break;
                }
                else{
                    //克隆转换后的帧加入队列
                    AVFrame* decoderFrame = av_frame_clone(swsFrame);
                    if(frmQueue!=nullptr){
                        frmQueue->enqueue(decoderFrame);
                    }
                    // av_frame_unref(decoderFrame);
                    // av_frame_free(&decoderFrame);
                    av_frame_unref(swsFrame);
                    av_frame_free(&swsFrame);//清理临时帧
                }
            }
            else{//无需格式转换
                //解码队列
                if(frmQueue){
                    AVFrame* decoderFrame = av_frame_clone(frame);
                    frmQueue->enqueue(decoderFrame);
                    av_frame_free(&decoderFrame);
                }
                av_frame_unref(frame);
            }
        }
    }
    av_frame_free(&frame);
}

void FFVDecoder::printError(int ret)
{
    char errorBuffer[AV_ERROR_MAX_STRING_SIZE];
    int res = av_strerror(ret,errorBuffer,sizeof errorBuffer);
    if(res<0){
        std::cerr<<"Unknow Error!"<<std::endl;
    }else{
        std::cerr<<"Error:"<<errorBuffer<<std::endl;
    }
}
