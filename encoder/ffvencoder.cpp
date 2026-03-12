#include "ffvencoder.h"
#include "queue/ffvpacketqueue.h"
FFVEncoder::FFVEncoder() {}

FFVEncoder::~FFVEncoder()
{
    close();
}

void FFVEncoder::init(FFVPacketQueue *pktQueue_)
{
    pktQueue = pktQueue_;
}

void FFVEncoder::close()
{
    std::lock_guard<std::mutex> lock(mutex);
    if(codecCtx){
        avcodec_free_context(&codecCtx);
        codecCtx = nullptr;
    }
    if(vPars){
        delete vPars;
        vPars = nullptr;
    }
    lastPts = -1;
}

void FFVEncoder::wakeAllThread()
{
    if(pktQueue)
        pktQueue->wakeAllThread();
}

int FFVEncoder::encode(AVFrame *frame, int streamIndex, int64_t pts, AVRational timeBase)
{
    std::lock_guard<std::mutex>lock(mutex);
    if(frame == nullptr||codecCtx ==nullptr){
        std::cout<<"nullptr"<<std::endl;
        return 0;
    }
    //时间戳处理
    //将输入的时间戳从输入时间基准转换为编码器的时间基准
    pts = av_rescale_q(pts,timeBase,codecCtx->time_base);

    //防止时间戳重复或不递增,解决时间戳回退问题
    if(pts<=lastPts){//修正时间戳,确保单调递增
        pts = lastPts + 1;
    }
    lastPts = pts;//更新最后的时间戳

    //设置帧的时间戳
    frame->pts = pts;

    //发送帧给编码器
    int ret = avcodec_send_frame(codecCtx,frame);
    if(ret<0){
        printError(ret);//打印错误信息
        return -1;
    }
    //循环接收编码后的数据包
    while(1){
        AVPacket* pkt = av_packet_alloc();
        ret = avcodec_receive_packet(codecCtx,pkt);
        //情况1:编码器需要更多输入数据
        if(ret==AVERROR(EAGAIN)){
            av_packet_free(&pkt);//释放数据包
            break;//退出循环等待下一帧
        }
        //情况2 编码器已经完成所有帧的编码
        else if(ret == AVERROR_EOF){
            std::cout<<"Encode Video EOF!"<<std::endl;
            av_packet_free(&pkt);//数据包
            break;//退出循环
        }
        //情况3:编码器发生错误
        else if(ret < 0){
            printError(ret);//打印错误信息
            //清理资源
            av_packet_free(&pkt);
            av_frame_unref(frame);
            av_frame_free(&frame);
            return -1;
        }
        //情况4 成功接收到编码的数据包
        else{
            //设置数据包的流索引
            pkt->stream_index = streamIndex;
            pktQueue->enqueue(pkt);
            av_packet_free(&pkt);
        }

    }
    return 0;//成功返回
}

void FFVEncoder::initVideo(AVFrame *frame, AVRational fps)
{
    std::lock_guard<std::mutex> lock(mutex);

    // 1. 创建并配置编码器参数结构体
    vPars = new FFVEncoderPars();
    vPars->bitRate = 2 * 1024 *1024;//2Mbps;
    vPars->height = frame->height;
    vPars->width = frame->width;
    vPars->videoFmt = AV_PIX_FMT_YUV420P;// 固定使用YUV420P像素格式（最常用）
    vPars->frameRate = fps; // 设置帧率

// 2. 查找编码器
#if 1
    // 使用软件编码器（默认）
    const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);
#elif 0
    // 使用AMD显卡的硬件编码器（仅限AMD显卡）
    const AVCodec* codec = avcodec_find_encoder_by_name("h264_amf");//只有amd显卡
#elif 0
    const AVCodec* codec = avcodec_find_encoder_by_name("h264_nvenc");//nvida显卡

#endif
     // 检查编码器是否找到
    if(codec==nullptr){
        std::cerr<<"Find H264 Codec Fail!"<<std::endl;
        delete vPars;
        vPars =nullptr;
        return;
    }

// 3. 分配编码器上下文
    codecCtx = avcodec_alloc_context3(codec);
    if(codecCtx == nullptr){
        std::cerr<<"Alloc CodecCtx Fail!"<<std::endl;
        return;// 返回但不清理vPars - 内存泄漏风险！
    }
    // 4. 配置编码器上下文的基本参数
    codecCtx->width = vPars->width;
    codecCtx->height = vPars->height;
    codecCtx->framerate = vPars->frameRate;
    // 时间基：时间戳的单位，这里是帧率的倒数
    // 例如帧率30fps时，时间基为1/30秒
    codecCtx->time_base = AVRational{vPars->frameRate.den,vPars->frameRate.num};
    std::cout<<"frame rate"<<vPars->frameRate.num<<"/"<<vPars->frameRate.den<<std::endl;
    std::cout<<"time_base"<<codecCtx->time_base.num<<"/"<<codecCtx->time_base.den<<std::endl;
    codecCtx->pix_fmt = vPars->videoFmt;

    // 设置全局头标志，某些封装格式（如MP4）需要
    // 如果不设置，每个关键帧都会包含编码参数信息
    codecCtx->flags|=AV_CODEC_FLAG_GLOBAL_HEADER;


    // 5. 高级编码设置（针对实时/低延迟场景优化）
    codecCtx->max_b_frames = 0;//禁用b帧,减少编码延迟
    codecCtx->gop_size = 12;//合理关键帧间隔,每12帧一个关键帧
    codecCtx->keyint_min = 12;//最小关键帧间隔
    codecCtx->flags|=AV_CODEC_FLAG_LOW_DELAY;//低延迟模式


// 6. 多线程编码配置（当前被禁用）
// 启用后可以加速编码，但可能增加延迟
#if 0
    codecCtx->thread_type = FF_THREAD_FRAME;
    codecCtx->thread_count = 8;
#endif

     // 7. 配置编码器选项
    AVDictionary* codec_options = nullptr;
#if 0
    av_dict_set(&codec_options,"usage","3",0);
    av_dict_set(&codec_options,"max_b_frames","0",0);
    av_dict_set(&codec_options,"latency","true",0);
#else
    //通用x264编码器选项（默认）
    av_dict_set(&codec_options,"tune","zerolatency",0);
    av_dict_set(&codec_options,"preset","ultrafast",0);
#endif
     // 8. 打开编码器
    int ret = avcodec_open2(codecCtx,codec,&codec_options);
    if(ret <0){
        printError(ret);
        return;
    }
    // 9. 清理选项字典
    av_dict_free(&codec_options);
}

AVCodecContext *FFVEncoder::getCodecCtx()
{
    //    std::lock_guard<std::mutex>lock(mutex);
    return codecCtx;
}

FFVEncoderPars *FFVEncoder::getEncoderPars()
{
    //    std::lock_guard<std::mutex>lock(mutex);
    return vPars;
}

void FFVEncoder::printError(int ret)
{
    char errorBuffer[AV_ERROR_MAX_STRING_SIZE];
    int res = av_strerror(ret,errorBuffer,sizeof errorBuffer);
    if(res < 0){
        std::cerr << "Unknow Error!" << std::endl;
    }
    else{
        std::cerr << "Error:" << errorBuffer << std::endl;
    }
}


