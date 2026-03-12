#ifndef FFCAPTURECONTEXT_H
#define FFCAPTURECONTEXT_H
//这是一个用于管理音视频捕获、处理、编码和渲染的整个流程的上下文管理类

#include "ui/ffcapwindow.h"
#include "thread/ffadecoderthread.h"
#include "thread/ffvdecoderthread.h"
#include "thread/ffamuxerthread.h"
#include "thread/ffvmuxerthread.h"
#include "thread/ffdemuxerthread.h"
#include "queue/ffapacketqueue.h"
#include "queue/ffvpacketqueue.h"
#include "queue/ffaframequeue.h"
#include "queue/ffvframequeue.h"
#include "thread/ffvmuxerthread.h"
#include "thread/ffamuxerthread.h"
#include "thread/ffmuxerthread.h"
#include "thread/ffaencoderthread.h"
#include "thread/ffvencoderthread.h"
#include "thread/ffvfilterthread.h"
#include "thread/ffafilterthread.h"
namespace FFCaptureContextType{
    constexpr size_t A_DECODER_SIZE = 2;//音频解码器2个
    constexpr size_t A_DEMUXER_SIZE = 2;//
    constexpr size_t V_DECODER_SIZE = 3;//视频解码器3个
    constexpr size_t V_DEMUXER_SIZE = 3;
    enum demuxerType{
        SCREEN,CAMERA,VIDEO,AUDIO,MICROPHONE,NOTYPE
    };
    constexpr int demuxerIndex[A_DEMUXER_SIZE+V_DEMUXER_SIZE+1]{
        0,1,2,
        0,1,-1
    };//映射不同的demuxer到数组索引
};

class FFCaptureContext
{
public:
    FFCaptureContext();
    ~FFCaptureContext();
public:
    //音频解码线程:[声卡]、[麦克风]
    FFADecoderThread* aDecoderThread[FFCaptureContextType::A_DECODER_SIZE];

    //视频解码线程:【屏幕】、【摄像头】
    FFVDecoderThread* vDecoderThread[FFCaptureContextType::V_DECODER_SIZE];

    //音频解复用线程：【音频】、【麦克风】
    FFDemuxerThread* aDemuxerThread[FFCaptureContextType::A_DEMUXER_SIZE];

    //视频解复用线程：【屏幕】、【摄像头】、【视频】
    FFDemuxerThread* vDemuxerThread[FFCaptureContextType::V_DEMUXER_SIZE];

    //音频解码packet包队列
    FFAPacketQueue* aDecoderPktQueue
        [FFCaptureContextType::A_DECODER_SIZE];

    //视频解码packet包队列
    FFVPacketQueue* vDecoderPktQueue[FFCaptureContextType::V_DECODER_SIZE];

    //视频编码packet包队列
    FFVPacketQueue* vEncoderPktQueue;

    //音频编码packet包队列
    FFAPacketQueue* aEncoderPktQueue;

    //音频编码帧队列
    FFAFrameQueue* aDecoderFrmQueue[FFCaptureContextType::A_DECODER_SIZE];

    //视频编码队列
    FFVFrameQueue* vDecoderFrmQueue[FFCaptureContextType::V_DECODER_SIZE];

    //视频filter编码Frame队列
    FFVFrameQueue* vFilterEncoderFrmQueue;

    //音频filter编码Frame队列
    FFAFrameQueue* aFilterEncoderFrmQueue;

    //视频渲染帧队列；【视频文件】
    FFVFrameQueue* vRenderFrmQueue;

    //音频解复用器
    FFDemuxer* aDemuxer[FFCaptureContextType::A_DEMUXER_SIZE];

    //视频解复用器
    FFDemuxer* vDemuxer[FFCaptureContextType::V_DEMUXER_SIZE];

    //音频解码器
    FFADecoder* aDecoder[FFCaptureContextType::A_DECODER_SIZE];

    //视频解码器
    FFVDecoder* vDecoder[FFCaptureContextType::V_DECODER_SIZE];

    //视频渲染器
    FFVRender* vRender;

    //复用器
    FFMuxer* muxer;

    //复用线程
    FFMuxerThread* muxerThread;

    //音频编码器
    FFAEncoder* aEncoder;

    //音频编码线程
    FFAEncoderThread * aEncoderThread;

    //视频编码器
    FFVEncoder* vEncoder;

    //视频编码器线程
    FFVEncoderThread * vEncoderThread;

    //视频过滤器
    FFVFilter* vFilter;

    //视频过滤线程
    FFVFilterThread* vFilterThread;

    //音频过滤器
    FFAFilter* aFilter;

    //音频过滤器线程
    FFAFilterThread* aFilterThread;

    //UI窗口
    FFCapWindow* capWindow;

};

#endif // FFCAPTURECONTEXT_H
