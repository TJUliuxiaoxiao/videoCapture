#include "ffaencoder.h"
#include "queue/ffaframequeue.h"
#include "queue/ffapacketqueue.h"
#include <QDebug>
FFAEncoder::FFAEncoder() {

}

FFAEncoder::~FFAEncoder()
{
    close();
}

void FFAEncoder::init(FFAPacketQueue *pktQueue_)
{
    pktQueue = pktQueue_;
}

void FFAEncoder::close()
{
    std::lock_guard<std::mutex> lock(mutex);
    if(codecCtx){
        avcodec_free_context(&codecCtx);
        codecCtx = nullptr;
    }
    if(aPars){
        delete aPars;
        aPars = nullptr;
    }
    clearPendingFrame();
}

void FFAEncoder::wakeAllThread()
{
    if(pktQueue){
        pktQueue->wakeAllThread();
    }
}

void FFAEncoder::initAudio(AVFrame *frame)
{
    std::lock_guard<std::mutex> lock(mutex);
    //使用互斥锁、

    //1.创建参数结构体并设置参数
    aPars = new FFAEncoderPars();
    aPars->biteRate = 64*1024;
    aPars->nbChannel = frame->ch_layout.nb_channels;
    aPars->sampleRate = frame->sample_rate;
    aPars->audioFmt = AV_SAMPLE_FMT_FLTP;

    //2.查找AAC编码器
    const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
    if(codec == nullptr){
        std::cerr<<"Find AAC Codec Fail!"<<std::endl;
        return;
    }

    //编解码器分配上下文codecCtx
    codecCtx = avcodec_alloc_context3(codec);//为特定编解码器初始化最佳默认设置
    if(codecCtx == nullptr){
        std::cerr<<"Alloc CodecCtx Fail!"<<std::endl;
        return;
    }

    // 3.分配编码器上下文参数
    codecCtx->bit_rate = aPars->biteRate;
    codecCtx->ch_layout.nb_channels = aPars->nbChannel;
    //4.设置默认的声道布局(根据声道数),通道布局结构体指针,通道数
    av_channel_layout_default(&codecCtx->ch_layout,aPars->nbChannel);

    codecCtx->sample_rate = aPars->sampleRate;//采样率
    codecCtx->sample_fmt = aPars->audioFmt;//采样格式
    //时间基:每个采样点的时间间隔，即1/采样率
    codecCtx->time_base = AVRational{1,aPars->sampleRate};
    //设置全局头标志，这样生成的流包含全局头信息(如用于MP4容器)
    codecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;//控制编解码器是否将配置信息（如 SPS/PPS 等）存储在容器级别，而不是每个关键帧中。
    // 当输出到这些容器时，通常需要设置,使用时机在打开编码器之前
    // formatContext->oformat->flags & AVFMT_GLOBALHEADER

    //     // 常见的需要全局头的容器
    //     - MP4 (.mp4, .m4v)
    //     - MOV (.mov)
    //     - MKV (.mkv)
    //     - FLV (.flv)  // 部分情况
    // 5.打开编码器
    int ret = avcodec_open2(codecCtx,codec,nullptr);
    if(ret<0){
        printError(ret);
        return;
    }
}

AVFrame* FFAEncoder::createFrameFromPending(){//用于从缓存中创建一个AVFrame
    AVFrame* frame = av_frame_alloc();
    frame->format = codecCtx->sample_fmt;//采样格式
    frame->ch_layout = codecCtx->ch_layout;//通道布局
    frame->ch_layout.nb_channels = codecCtx->ch_layout.nb_channels;//通道数
    frame->sample_rate = codecCtx->sample_rate;//采样率
    frame->nb_samples = pendingFrame.samples;//采样点数
    frame->pts = pendingFrame.next_pts;//时间戳

    av_frame_get_buffer(frame,0);
    for(int ch = 0;ch<frame->ch_layout.nb_channels;++ch)
    {
        memcpy(frame->data[ch],pendingFrame.data[ch].data(),
               pendingFrame.samples* av_get_bytes_per_sample(codecCtx->sample_fmt));

    }
    return frame;
}


void FFAEncoder::clearPendingFrame()
{
    pendingFrame.next_pts = 0;
    pendingFrame.samples = 0;
    for(auto& channel:pendingFrame.data){
        channel.clear();
    }
}

int FFAEncoder::encode(AVFrame* frame,int streamIndex,int64_t pts,AVRational timeBase)
{   /*
    frame: 输入的音频帧，可能包含多个采样点。
    streamIndex: 流索引，用于标记编码后的包属于哪个流。
    pts: 显示时间戳，表示该帧的显示时间。
    timeBase: 时间基，用于时间戳的计算。
    */
    Q_UNUSED(pts);
    Q_UNUSED(timeBase);
    std::lock_guard<std::mutex> lock(mutex);
    if(frame == nullptr || codecCtx == nullptr){
        std::cout<<"nullptr"<<std::endl;
        return 0;
    }
    int frame_size = codecCtx->frame_size;//编码器要求的每帧样本数
    int input_samples = frame->nb_samples;//输入帧的样本数
    int bytes_per_sample = av_get_bytes_per_sample(codecCtx->sample_fmt);//用于获取音频样本格式的字节大小。
    //合并缓存数据和新数据
    std::vector<uint8_t> merged_data[8];
    int total_samples = pendingFrame.samples + input_samples;
    // 1.将缓存数据复制到合并缓冲区
    for(int ch = 0;ch<codecCtx->ch_layout.nb_channels;++ch){
        merged_data[ch].resize(total_samples*bytes_per_sample);
        //先复制缓冲数据
        if(pendingFrame.samples > 0){
            memcpy(merged_data[ch].data(),pendingFrame.data[ch].data(),
                   pendingFrame.samples*bytes_per_sample);
        }
    // 2.追加新数据
        memcpy(merged_data[ch].data() + pendingFrame.samples*bytes_per_sample,
               frame->data[ch],input_samples*bytes_per_sample);
    }
    // 计算能生成多少完整帧
    int total_full_frames = total_samples / frame_size;
    int remaining_samples = total_samples % frame_size;
    // 3.处理完整帧
    for(int i = 0;i<total_full_frames;++i){//循环对每个帧进行处理
        AVFrame* sub_frame = av_frame_alloc();//分配子帧
        sub_frame->format = codecCtx->sample_fmt;
        sub_frame->ch_layout = codecCtx->ch_layout;
        // sub_frame->ch_layout.nb_channels = codecCtx->ch_layout.nb_channels;
        sub_frame->sample_rate = codecCtx->sample_rate;
        sub_frame->nb_samples = frame_size;
        sub_frame->pts = pendingFrame.next_pts + i * frame_size;//以采样点为单位计算的时间戳
        av_frame_get_buffer(sub_frame,0);
        // 逐通道复制数据到子帧
        for(int ch = 0;ch<sub_frame->ch_layout.nb_channels;++ch){
            uint8_t* src = merged_data[ch].data() +
                           i * frame_size * bytes_per_sample;
            memcpy(sub_frame->data[ch],src,frame_size * bytes_per_sample);
        }
        // 发送到编码器
        avcodec_send_frame(codecCtx,sub_frame);
        av_frame_free(&sub_frame);
        // 接收编码后的包
        while(true){
            AVPacket* pkt = av_packet_alloc();
            int ret = avcodec_receive_packet(codecCtx,pkt);
            if(ret == AVERROR(EAGAIN)){
                av_packet_free(&pkt);
                break;
            }else if(ret<0){
                av_packet_free(&pkt);
                return -1;
            }
            pkt->stream_index = streamIndex;
            pktQueue->enqueue(pkt);
            // av_packet_free(&pkt);
        }
    }
    // 4.更新缓存
    pendingFrame.next_pts = pendingFrame.next_pts +
                            total_full_frames * frame_size;
    pendingFrame.samples = remaining_samples;
    for(int ch = 0;ch < codecCtx->ch_layout.nb_channels;++ch){
        pendingFrame.data[ch].resize(remaining_samples * bytes_per_sample);
        if(remaining_samples > 0){
            uint8_t* src =
                merged_data[ch].data() + total_full_frames * frame_size * bytes_per_sample;
            memcpy(pendingFrame.data[ch].data(),src,remaining_samples * bytes_per_sample);
        }
    }
    return 0;
}

FFAEncoderPars * FFAEncoder::getEncoderPars()
{
    std::lock_guard<std::mutex> lock(mutex);
    return aPars;
}

AVCodecContext* FFAEncoder::getCodecCtx()
{
    //这里加锁会死锁
    return codecCtx;
}

void FFAEncoder::printError(int ret)
{
    char errorBuffer[AV_ERROR_MAX_STRING_SIZE];
    int res = av_strerror(ret,errorBuffer,sizeof errorBuffer);
    if(res<0){
        std::cerr<<"Unknow Error!"<<std::endl;
    }else{
        std::cerr<<"Error"<<errorBuffer<<std::endl;
    }
}
