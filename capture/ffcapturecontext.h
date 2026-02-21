#ifndef FFCAPTURECONTEXT_H
#define FFCAPTURECONTEXT_H
//这是一个用于管理音视频捕获、处理、编码和渲染的整个流程的上下文管理类

#include "ui/ffcapwindow.h"
#include "thread/ffadecoderthread.h"

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
    FFADecoderThread aDecoderThread[FFCaptureContextType::A_DECODER_SIZE];
};

#endif // FFCAPTURECONTEXT_H
