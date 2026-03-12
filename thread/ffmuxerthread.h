#ifndef FFMUXERTHREAD_H
#define FFMUXERTHREAD_H

#include "ffthread.h"
#include <mutex>
extern "C"{
#include <libavformat/avformat.h>
}

class FFAPacketQueue;
class FFVPacketQueue;
class FFMuxer;
class FFAEncoder;
class FFVEncoder;
class FFPacket;
class FFCaptureContext;
//音频包队列、视频包队列、复用器、
//音频编码器、视频编码器、数据包、捕获上下文。
class FFMuxerThread : public FFThread
{
public:
    FFMuxerThread();
    virtual ~FFMuxerThread() override;
    /*初始化*/
    void init(FFAPacketQueue *aPktQueue_,FFVPacketQueue* vPktQueue_,FFMuxer* muxer_,FFAEncoder* aEncoder_,FFVEncoder* vEncoder_,FFCaptureContext*captureCtx_);
    void close();//关闭线程
    void wakeAllThread();//可能唤醒等待的线程
protected:
    virtual void run() override;
private:
    void sendCaptureProcessEvent(double seconds);
private:
    FFAPacketQueue* aPktQueue = nullptr;
    FFVPacketQueue* vPktQueue = nullptr;

    FFMuxer* muxer = nullptr;
    FFVEncoder* vEncoder = nullptr;
    FFAEncoder* aEncoder = nullptr;
    AVRational vTimeBase;
    AVRational aTimeBase;

    std::mutex mutex;
    double lastProcessTime = 0;
    FFCaptureContext* captureCtx = nullptr;
};

#endif // FFMUXERTHREAD_H
