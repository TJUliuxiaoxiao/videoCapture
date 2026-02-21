#ifndef FFDEMUXERTHREAD_H
#define FFDEMUXERTHREAD_H

#include "ffthread.h"
#include <string>
#include <mutex>
#include <condition_variable>

extern "C"
{
#include "libavformat/avformat.h"
}

class FFDemuxer;//解复用器主类
class FFAPacketQueue;
class FFVPacketQueue;
class FFPlayerContext;//播放器上下文
class FFDemuxerThread : public FFThread
{
public:
    //构造函数和析构函数
    FFDemuxerThread();
    virtual ~FFDemuxerThread() override;
    //初始化
    void init(FFDemuxer* demuxer_);
    //线程控制
    void wakeAllThread();
    void seek(int64_t seekSec);
    void close();
    //状态检查
    bool peekStop();

protected:
    virtual void run() override;

private:
    void sendStopEvent();//暂停

private:
    FFDemuxer* demuxer = nullptr;
    FFPlayerContext* playerCtx = nullptr;
    std::condition_variable cond;
    std::mutex mutex;
    std::atomic<bool> stopFlag;
};

#endif // FFDEMUXERTHREAD_H
