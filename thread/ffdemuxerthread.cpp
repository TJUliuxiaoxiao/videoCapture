#include "ffdemuxerthread.h"
#include "demuxer/ffdemuxer.h"
#include "queue/ffvpacketqueue.h"
#include "queue/ffapacketqueue.h"
#include "queue/ffeventqueue.h"
#include "player/ffplayercontext.h"
FFDemuxerThread::FFDemuxerThread() {
    stopFlag.store(true);
}

FFDemuxerThread::~FFDemuxerThread()
{
    if(demuxer){
        delete demuxer;
        demuxer = nullptr;
    }
    if(playerCtx){
        delete playerCtx;
        playerCtx = nullptr;
    }
}

void FFDemuxerThread::init(FFDemuxer* demuxer_)
{
    demuxer = demuxer_;
    playerCtx = new FFPlayerContext();
    playerCtx->demuxerThread = this;
    /*设计目的：这种双向引用允许：
    解复用器通过上下文发送全局事件
    其他组件通过上下文访问解复用器
    实现组件间的通信和协调
    需要注意的问题：
    循环引用：可能导致内存泄漏
    生命周期管理：需要仔细设计所有权
    线程安全：多线程访问需要同步*/
}

void FFDemuxerThread::wakeAllThread()
{
    std::lock_guard<std::mutex> lock(mutex);//保护demuxer访问
    if(demuxer){//检查demuxer是否有效
        demuxer->wakeAllThread();//可能唤醒解码器、音频/视频线程等
    }
    //唤醒等待在本线程上的线程
    cond.notify_all();//唤醒等待在当前线程条件变量上的线程
}

void FFDemuxerThread::seek(int64_t seekSec)
{

}

void FFDemuxerThread::close()
{
    if(demuxer){
        demuxer->close();//关闭下游解复用器
    }
    //将关闭请求传播给关联的 FFDemuxer 对象
    //（通常负责实际的 FFmpeg 解复用上下文）。
    cond.notify_all();//唤醒等待在本线程条件变量上的线程
    stopFlag.store(true,std::memory_order_release);//原子设置停止标志
}

bool FFDemuxerThread::peekStop()
{
    return stopFlag.load(std::memory_order_acquire);
}

void FFDemuxerThread::run()
{
    while(!m_stop.load(std::memory_order_acquire)){
        stopFlag.store(false,std::memory_order_release);
        int ret = demuxer->demux();
        if(ret!=0){
            m_stop = true;
        }
        if(m_stop){
            break;
        }
    }
}
