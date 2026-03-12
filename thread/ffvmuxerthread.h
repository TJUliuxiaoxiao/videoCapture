#ifndef FFVMUXERTHREAD_H
#define FFVMUXERTHREAD_H

#include "ffthread.h"
class FFVPacketQueue;
class FFMuxer;

/*
 * FFVMuxerThread 是一个专门处理视频复用任务的工作线程。它的工作流程通常如下：
外部模块（如视频编码线程）将编码后的视频包放入 FFVPacketQueue。
FFVMuxerThread 启动后，在 run() 中循环从队列中取出包。
对于每个包，通过 muxer 将其写入输出文件，并可能需要根据
videoPts 和 vFrameDuration 调整或校验包的时间戳。
当收到停止信号（由基类 FFThread 提供机制）时，线程退出循环，
并可能调用 muxer 的收尾函数（如写入文件尾）。
该类与之前看到的音频复用线程 FFAMuxerThread 类似，但专注于视频流，
且没有音频编码器成员，因为视频包通常已经是编码后的格式，无需再次编码。
*/


//前向声明两个类,避免在头文件中包含它们的完整定义,
//从而减少编译依赖,加快编译速度。这两个类的具体定义将在源文件中包含
class FFVMuxerThread : public FFThread
{
public:
    FFVMuxerThread();
    virtual ~FFVMuxerThread() override;
    //这样当通过基类指针删除派生类对象时，
    //能够正确调用派生类的析构函数，释放资源。
    void init(FFVPacketQueue* pktQueue_,FFMuxer* muxer_);
protected:
    virtual void run() override;
private:
    FFVPacketQueue *pktQueue = nullptr;
    FFMuxer* muxer = nullptr;
    int64_t videoPts = 0;
    //当前帧的显示时间戳,用于记录下一个包的PTS,确保连续递增,

    int64_t vFrameDuration = 0;//视频帧的持续时间
    //通常以时间基为单位,在恒定帧率的情况下，
    //每一帧的时长固定,可用于计算后续帧的PTS


};

#endif // FFVMUXERTHREAD_H
