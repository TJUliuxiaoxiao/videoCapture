#include "ffmuxer.h"

FFMuxer::FFMuxer():headerFlag(false),trailerFlag(false){}

FFMuxer::~FFMuxer()
{
    close();
}

void FFMuxer::init(const std::string &url_, const std::string &format_)
{
    std::lock_guard<std::mutex> lock(mutex);
    url = url_;
    format = format_;
    initMuxer();
}

void FFMuxer::addStream(AVCodecContext *codecCtx)
{
    std::lock_guard<std::mutex> lock(mutex);//保护流添加和状态变量
    AVStream* stream = avformat_new_stream(fmtCtx,nullptr);//在输出容器中创建一个新的媒体流
    if(!stream){
        std::cerr<<"New Stream Fail!"<<std::endl;
        return;
    }
    //复制编解码器参数，将编解码器上下文中的参数复制到流的编解码器参数中
    int ret = avcodec_parameters_from_context(stream->codecpar,codecCtx);
    if(ret < 0){
        std::cerr<<"copy Parameters From Context Fail"<<std::endl;
        printError(ret);
        return;
    }
    //设置时间基准
    stream->time_base = codecCtx->time_base;
    //stream->time_base = 1/48000
    //根据codec_type区分音频和视频流

    if(codecCtx->codec_type == AVMEDIA_TYPE_AUDIO){//音频流
        aCodecCtx = codecCtx;
        aStream = stream;
        //1/48000
        aStreamIndex = stream->index;
        hasAudio = true;
    }else if(codecCtx->codec_type==AVMEDIA_TYPE_VIDEO){//视频流
        vCodecCtx = codecCtx;
        vStream = stream;
        vStreamIndex = stream->index;
        hasVideo = true;
    }
    streamCount++;
    if(streamCount == 2&&hasAudio&&hasVideo){
        readyFlag = true;//直接赋值,由锁保证可见性
    }
}

int FFMuxer::mux(AVPacket *packet)
{
    std::lock_guard<std::mutex> lock(mutex);
    if(trailerFlag){
        return 1;
    }
    //获取时间基并转换时间戳
    //根据packet所在的流索引（音频或视频），获取对应编解码
    //器上下文中的时间基（srcTimeBase）和输出流的时间基（dstTimeBase）。如果既不是音频流也不是视频流，则返回错误。
    int streamIndex = packet->stream_index;
    AVRational srcTimeBase,dstTimeBase;
    if(streamIndex == aStreamIndex){
        srcTimeBase = aCodecCtx->time_base;
        dstTimeBase = aStream->time_base;
        std::cout<<"=======aStream========"<<std::endl;
        std::cout<<"CodeTimeBase:"<<srcTimeBase.num<<"/"<<srcTimeBase.den<<std::endl;
        std::cout<<"StreamTimeBase:"<<dstTimeBase.num<<"/"<<dstTimeBase.den<<std::endl;
    }else if(streamIndex == vStreamIndex){
        srcTimeBase = vCodecCtx->time_base;
        dstTimeBase = vStream->time_base;
        std::cout<<"=======vStream========"<<std::endl;
        std::cout<<"CodeTimeBase:"<<srcTimeBase.num<<"/"<<srcTimeBase.den<<std::endl;
        std::cout<<"StreamTimeBase:"<<dstTimeBase.num<<"/"<<dstTimeBase.den<<std::endl;
    }else{
        return -1;
    }
    //转换时间戳
    packet->pts = av_rescale_q(packet->pts,srcTimeBase,dstTimeBase);
    packet->dts = av_rescale_q(packet->dts,srcTimeBase,dstTimeBase);
    /*DTS和PTS的区别
    DTS（Decoding Time Stamp）：表示这个数据包应该被解码的时间。
    PTS（Presentation Time Stamp）：表示这个数据包解码后应该被显示（或呈现）的时间。
    编码顺序（DTS顺序）和显示顺序（PTS顺序）：
    由于B帧需要后续的帧作为参考，所以编码顺序和显示顺序可能不同。例如，显示顺序为：I B B P，但编码顺序可能为：I P B B（其中P帧在B帧之前编码，因为B帧需要P帧作为参考）。
    在FFmpeg中，可以通过AVFrame->pict_type来获取帧的类型（I、B、P等）。常见的pict_type值有：
    AV_PICTURE_TYPE_I, AV_PICTURE_TYPE_P, AV_PICTURE_TYPE_B。。*/
    packet->duration = av_rescale_q(packet->duration,srcTimeBase,dstTimeBase);//显示时间

    if(packet->pts == AV_NOPTS_VALUE||packet->dts == AV_NOPTS_VALUE||packet->pts<0){
        av_packet_unref(packet);
        av_packet_free(&packet);
        return 0;
    }//AV_NOPTS_VALUE表示无时间戳,特殊值
    if(fmtCtx == nullptr){
        return 0;
    }
    int ret = av_write_frame(fmtCtx,packet);
    //将一个packet写入
    if(ret<0){
        std::cerr<<"Mux Fail!"<<std::endl;
        printError(ret);
        av_packet_unref(packet);
        av_packet_free(&packet);
        return -1;
    }
    av_packet_unref(packet);
    av_packet_free(&packet);
    return 0;
}

void FFMuxer::writeHeader()
{
    while(!readyFlag){
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }//这段代码是一个简单的等待循环，等待某个标志位（readyFlag）变为true。
    //在等待期间，每次循环会睡眠10毫秒，以避免忙等待（busy-waiting）占用过多CPU资源。
    std::lock_guard<std::mutex> lock(mutex);//保护头文件
    if(headerFlag){
        return;
    }
    headerFlag = true;//设置状态后再执行写入
    if(fmtCtx ==nullptr){
        std::cerr<<"fmtCtx is nullptr"<<std::endl;
        return;
    }
    int ret = avformat_write_header(fmtCtx,nullptr);
    if(ret < 0){
        std::cerr<<"Write Header Fail!"<<std::endl;
        printError(ret);
        headerFlag = false;//恢复状态以防后续错误
    }
}

void FFMuxer::writeTrailer()
{
    std::lock_guard<std::mutex> lock(mutex);
    //保护尾文件写入状态
    if(trailerFlag){
        return;//直接判断bool变量
    }
    trailerFlag = true;//设置状态后再执行写入
    if(fmtCtx ==nullptr){
        return;
    }
    int ret = av_write_trailer(fmtCtx);
    if(ret<0){
        std::cerr<<"write Trailer Fail！"<<std::endl;
        printError(ret);
        trailerFlag = false;//恢复状态以防后续错误
    }
}

int FFMuxer::getAStreamIndex()
{
    std::lock_guard<std::mutex> lock(mutex);
    return aStreamIndex;
}

int FFMuxer::getVStreamIndex()
{
    std::lock_guard<std::mutex> lock(mutex);
    return vStreamIndex;
}


void FFMuxer::close()
{
    std::lock_guard<std::mutex> lock(mutex);
    // 1.如果头部已经写但是尾部未写，写尾部
    if(headerFlag&&!trailerFlag){
        writeTrailer();
    }
    // 2.关闭输出
    if(fmtCtx){
        //先关闭IO
        if(fmtCtx->pb&&!(fmtCtx->oformat->flags&AVFMT_NOFILE)){
            avio_closep(&fmtCtx->pb);
        }
        //释放上下文
        avformat_free_context(fmtCtx);
        fmtCtx = nullptr;
    }
    // 3.
    url.clear();
    format.clear();
    headerFlag = false;
    trailerFlag = false;
    readyFlag = false;
    hasAudio = false;
    hasVideo = false;
    aStreamIndex = vStreamIndex = -1;
    streamCount = 0;
    aStream = nullptr;
    vStream = nullptr;
    aCodecCtx = nullptr;
    vCodecCtx = nullptr;
}

void FFMuxer::initMuxer()
{
    std::cout<<"url = "<<url<<std::endl;
    std::cout<<"format = "<<format<<std::endl;
    //1. 创建输出格式上下文
    int ret = avformat_alloc_output_context2(&fmtCtx,nullptr,format.c_str(),url.c_str());
    if(ret < 0){
        std::cerr<<"Alloc output Context Fail!"<<std::endl;
        printError(ret);
        return;
    }
    // 2. 根据格式类型进行不同处理
    if(format == "rtsp"){
        //RTSP输出设置
        AVDictionary *opts = nullptr;
        //设置RTSP传输协议为TCP
        ret = av_opt_set(&opts,"rtsp_transport","tcp",0);
        if(ret<0){
            std::cerr<<"av_opt_set:rtsp_transport fail"<<std::endl;
        }
        //设置超时时间为5秒
        ret = av_dict_set(&opts,"stimeout","5000000",0);
        if(ret<0){
            std::cerr<<"av_dict_set:stimeout fail"<<std::endl;
        }
        //将选项应用到格式上下文
        ret = av_opt_set_dict(fmtCtx,&opts);
        if(ret<0){
            std::cerr<<"av_opt_set_fail"<<std::endl;
        }
        //释放选项字典
        av_dict_free(&opts);
    }
    else{
        //非RTSP格式(如MP4、AVI等)
        //打开文件输出
        ret = avio_open(&fmtCtx->pb,url.c_str(),AVIO_FLAG_WRITE);
        if(ret < 0){
            std::cerr<<"Open File Fail!"<<std::endl;
            printError(ret);
            return;
        }
    }
}

void FFMuxer::printError(int ret)
{
    char errorBuffer[AV_ERROR_MAX_STRING_SIZE];
    int res = av_strerror(ret,errorBuffer,sizeof (errorBuffer));
    if(res < 0){
        std::cerr<<"Unknow Error!"<<std::endl;
    }else{
        std::cerr<<"Error:"<<errorBuffer<<std::endl;
    }
}
