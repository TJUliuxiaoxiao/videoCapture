#ifndef FFAMUXERTHREAD_H
#define FFAMUXERTHREAD_H

#include "ffthread.h"
//音频复用线程
class FFAPacketQueue;
class FFMuxer;
class FFAEncoder;

class FFAMuxerThread : public FFThread
{
public:
    FFAMuxerThread();
    virtual ~FFAMuxerThread() override;//virtual可以省略
    void init(FFAPacketQueue*pktQueue_,FFMuxer* muxer_,FFAEncoder* aEncoder_);
protected:
    virtual void run() override;
private:
    FFAPacketQueue* pktQueue = nullptr;
    FFMuxer* muxer = nullptr;
    FFAEncoder* aEncoder = nullptr;

};

#endif // FFAMUXERTHREAD_H
