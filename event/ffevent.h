#ifndef FFEVENT_H
#define FFEVENT_H

#include <QMetaObject>
#include "capture/ffcapturecontext.h"
#include"thread/ffdemuxerthread.h"
#include "thread/ffafilterthread.h"
#include "thread/ffvfilterthread.h"
#include"thread/ffadecoderthread.h"
#include"thread/ffvdecoderthread.h"
#include "thread/ffaencoderthread.h"
#include "thread/ffvencoderthread.h"
#include"queue/ffapacketqueue.h"
#include"queue/ffvpacketqueue.h"
#include"queue/ffaframequeue.h"
#include"queue/ffvframequeue.h"
#include"decoder/ffadecoder.h"
#include"decoder/ffvdecoder.h"
#include"demuxer/ffdemuxer.h"
using namespace FFCaptureContextType;
class FFEvent
/*FFEvent是一个超集上下文基类,它封装了整个音视频处理框架所需的所有核心组件
 使派生事件能够方便地操作从解复用到渲染的整个流程。*/
/*集中管理:FFEvent类将整个音视频处理流水线中的所有关键组件集中在一个地方,使得派生类可以方便地访问任何需要的模块而无需通过全局变量或层层传递
支持多流:通过使用数组如aPktQueue[A_DEMUXER_SIZE],框架可以同理处理多个音频源或视频源,每个源有自己独立的队列和线程
事件驱动:所有事件都通过继承FFEvent实现,work()函数中根据具体事件类型操作相应的组件。事件被放入全局事件队列
FFEventQueue中,由事件处理线程依次执行*/
{
public:
    FFEvent(FFCaptureContext* captureContext_);
    virtual ~FFEvent();
    virtual void work() = 0;
    //纯虚函数work,所有具体事件(所有开始录制、停止录制、进度更新等)
    //都必须实现该函数,在该函数中执行相应的业务逻辑.
protected:
    //捕获上下文，指向捕获上下文的指针，用于标识当前事件属于哪个录制会话(如屏幕录制，摄像头录制)
    //1、全局上下文
    FFCaptureContext* captureContext = nullptr;

    //2、队列(线程间数据传递)
    //音视频包队列数组、每个元素对应一个音频源的编码前数据包队列
    //demuxer->【packetQueue】->decoder
    FFAPacketQueue* aPktQueue[A_DEMUXER_SIZE];
    FFVPacketQueue* vPktQueue[V_DEMUXER_SIZE];

    //音视频帧队列数组,解码后的原始音视频帧
    //decoder->frmQueue->filter
    FFAFrameQueue* aFrmQueue[A_DECODER_SIZE];
    FFVFrameQueue* vFrmQueue[V_DECODER_SIZE];

    //视频渲染帧队列，用于将最终要显示的帧传递给渲染器
    FFVFrameQueue* vRenderFrmQueue;

    //过滤器后的音视频帧队列,准备送入音视频编码器
    // filter->frmQueue->encoder
    FFAFrameQueue* aFilterEncoderFrmQueue;
    FFVFrameQueue* vFilterEncoderFrmQueue;
    //编码器数据包队列
    //encoder->pktQueue
    FFAPacketQueue* aEncoderPktQueue;
    FFVPacketQueue* vEncoderPktQueue;

    //过滤器
    FFAFilter* aFilter;
    FFVFilter* vFilter;

    //过滤线程
    FFAFilterThread* aFilterThread;
    FFVFilterThread* vFilterThread;
    //解码器
    FFADecoder* aDecoder[A_DECODER_SIZE];
    FFVDecoder* vDecoder[V_DECODER_SIZE];

    //编码器
    FFAEncoder* aEncoder;
    FFVEncoder* vEncoder;
    //编码线程
    FFAEncoderThread* aEncoderThread;
    FFVEncoderThread* vEncoderThread;

    //复用器
    FFMuxer* muxer;
    //复用线程
    FFMuxerThread* muxerThread;

    //解码线程
    FFADecoderThread* aDecoderThread[A_DECODER_SIZE];
    FFVDecoderThread* vDecoderThread[V_DECODER_SIZE];
    //解复用器
    FFDemuxer* aDemuxer[A_DEMUXER_SIZE];
    FFDemuxer* vDemuxer[V_DEMUXER_SIZE];
    //解复用器
    FFDemuxerThread* aDemuxerThread[A_DEMUXER_SIZE];
    FFDemuxerThread* vDemuxerThread[V_DEMUXER_SIZE];

    //视频渲染器
    FFVRender* vRender = nullptr;
    //UI窗口
    FFCapWindow* capWindow;
};

#endif // FFEVENT_H
