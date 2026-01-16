#include "ffdemuxer.h"
#include <iostream>

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
    // av_init_packet(packet);
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
            }
        }
    }


}


