#include "ffdemuxer.h"
#include <iostream>
#include "queue/ffapacketqueue.h"
#include "queue/ffvpacketqueue.h"
#include "capture/ffcapturecontext.h"
#include "clock/ffglobalclock.h"
using namespace FFCaptureContextType;

FFDemuxer::FFDemuxer() {}
FFDemuxer::~FFDemuxer(){
    close();
}

void FFDemuxer::init(const string &url_, const string &format_, FFAPacketQueue *aPktQueue_, FFVPacketQueue *vPktQueue_, int type_)
{
    std::lock_guard<std::mutex> lock(mutex);// 会在构造时锁定互斥量，在析构时（即离开作用域时）自动解锁，从而保证了在作用域内的操作是线程安全的。
    url = url_;
    format = format_;
    aPktQueue = aPktQueue_;
    vPktQueue = vPktQueue_;
    type = type_;
    stopFlag = false;
    initDemuxer();

}

int FFDemuxer::demux()
{
    std::lock_guard<std::mutex> lock(mutex);
#if LIBAVCODEC_VERSION_MAJOR>=58
    //新版本代码
    AVPacket* packet = av_packet_alloc();
    av_init_packet(packet);
#else
    //旧版本代码
    AVPacket* packet = av_packet_alloc();
    av_init_packet(packet);
#endif
    if(fmtCtx == nullptr)
    {
        return -1;
    }
    int ret = av_read_frame(fmtCtx,packet);//从打开的媒体文件中读取下一个数据包

    if(ret < 0)
    {
        if(ret == AVERROR_EOF){//到达文件末尾
            if(aPktQueue){
                aPktQueue->enqueueNull();
                av_packet_unref(packet);
                av_packet_free(&packet);
            }
            if(vPktQueue){
                vPktQueue->enqueueNull();
                av_packet_unref(packet);
                av_packet_free(&packet);
            }
            std::cout<<"AVERROR_EOF"<<std::endl;
            return 1;
        }
        else{
            printError(ret);
            avformat_close_input(&fmtCtx);
            av_packet_free(&packet);
            return -1;
        }
    }
    if(stopFlag){
        return 0;
    }
    if(aStream&&packet->stream_index==aStream->index){
        if(aPktQueue){
            aPktQueue->enqueue(packet);
            av_packet_free(&packet);
        }
        else{
            av_packet_unref(packet);
            av_packet_free(&packet);
        }
    }else if(vStream&&packet->stream_index==vStream->index){
        if(vPktQueue){
            if(type == VIDEO){
                std::cout<<"pts:"<<packet->pts<<std::endl;
            }
            vPktQueue->enqueue(packet);
            av_packet_free(&packet);
        }
        else{
            av_packet_unref(packet);
            av_packet_free(&packet);
        }
    }
    return 0;
}

AVStream *FFDemuxer::getAStream()
{
    std::lock_guard<std::mutex> lock(mutex);
    return aStream;
}

AVStream *FFDemuxer::getVStream()
{
    std::lock_guard<std::mutex> lock(mutex);
    return vStream;
}

void FFDemuxer::wakeAllThread()
{
    if(vPktQueue)
        vPktQueue->wakeAllThread();
    if(aPktQueue)
        aPktQueue->wakeAllThread();
}

void FFDemuxer::close()
{
    std::lock_guard<std::mutex> lock(mutex);
    stopFlag = true;
    if(fmtCtx){
        avformat_close_input(&fmtCtx);
        fmtCtx = nullptr;
    }
    if(opts){
        av_dict_free(&opts);
    }
    std::cout<<"demuxer close!"<<std::endl;
}

void FFDemuxer::printError(int ret)
{
    char errorBuffer[AV_ERROR_MAX_STRING_SIZE];
    int res = av_strerror(ret,errorBuffer,sizeof errorBuffer);
    if(res<0){
        std::cerr<<"UnKnow Error!"<<std::endl;
    }else{
        std::cerr<<"Error:"<<errorBuffer<<std::endl;
    }
}

void FFDemuxer::initDemuxer()
{
    std::cout<<"url = "<<url<<std::endl;
    avformat_network_init();
    avdevice_register_all();

    if(type == CAMERA){
        av_dict_set(&opts,"rtbufsize","1024",0);
        av_dict_set(&opts,"threads","8",0);
        av_dict_set(&opts,"video_size","1280x720",0);
        av_dict_set(&opts,"framerate","30",0);
    }else if(type == AUDIO){
        av_dict_set(&opts,"audio_buffer_size","10",0);
    }else if(type == SCREEN){
        av_dict_set(&opts,"rtbufsize","1024",0);
        av_dict_set(&opts,"threads","8",0);
        av_dict_set(&opts,"framerate","30",0);
    }
    else if(type == MICROPHONE){
        av_dict_set(&opts,"audio_buffer_size","10",0);
    }
    inputFmt = av_find_input_format(format.c_str());
    int ret = avformat_open_input(&fmtCtx,url.c_str(),inputFmt,&opts);
    if(ret<0){
        avformat_close_input(&fmtCtx);
        printError(ret);
        return;
    }
    if(fmtCtx == nullptr){
        std::cerr<<"nullptr in fmtCtx"<<std::endl;
        return;
    }
    ret = avformat_find_stream_info(fmtCtx,nullptr);
    if(ret<0){
        avformat_close_input(&fmtCtx);
        printError(ret);
        return;
    }
}

int FFDemuxer::getType()
{
    return type;
}
