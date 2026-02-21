#ifndef FFVDECODERTHREAD_H
#define FFVDECODERTHREAD_H

#include "ffthread.h"
extern "C"
{
#include <libavformat/avformat.h>
#include <libavutil/time.h>
}

class FFVPacketQueue;
class FFVDecoder;
class FFPlayerContext;
class FFVDecoderThread: public FFThread
{

//继承自FFThread,表明这是一个可独立运行的线程类,FFThread
//很可能封装了std::thread或平台相关线程接口
public:

    FFVDecoderThread();
    virtual ~FFVDecoderThread() override;

    void init(FFVDecoder* vDecoder_,
              FFVPacketQueue* vPktQueue_);

    void wakeAllThread();//唤醒线程，用于解除队列的阻塞等
    void close();//关闭线程

    bool peekStop();//检查停止标志,返回stopFlag的当前值。
    //用于外部查询线程是否即将退出。

protected:
    virtual void run() override;//重写run函数
private:
    void sendEndEvent();
    //私有辅助函数:当解码循环正常结束时调用,
    //向播放器或外部发送解码结束事件,已触发后续动作
    //(如切换音轨、显示结束画面),实现可能通过playerCtx回调或直接信号方式

private:
    FFVPacketQueue* vPktQueue = nullptr;
    FFVDecoder* vDecoder = nullptr;
    FFPlayerContext* playerCtx = nullptr;
    std::atomic<bool> stopFlag;
};

#endif // FFVDECODERTHREAD_H
