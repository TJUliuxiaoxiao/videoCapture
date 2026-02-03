#ifndef FFARESAMPLER_H
#define FFARESAMPLER_H
extern "C"{
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
#include "libavutil/channel_layout.h"
}
#include <iostream>
class FFAudioPars;
class FFAResampler
{
public:
    explicit FFAResampler();
    ~FFAResampler();
    void init(FFAudioPars* src,FFAudioPars *dst);
    void resample(AVFrame* srcFrame,AVFrame** dstFrame);
private:
    void initSwr();
    AVFrame* allocFrame(FFAudioPars *aPars,int nbSamples,AVFrame* srcFrame);
    void printError(int ret);
    /*struct FFAudioPars{
    int sampleRate;//采样率
    int nbChannels;//声道数
    AVRational timeBase;//时间基准,定义时间单位，用于时间戳计算
    enum AVSampleFormat aFormatEnum;//采样格式枚举
    int sampleSize;//样本大小,每个音频样本占用的字节数,sampleSize = 2声道 x 2字节/样本 = 4字节/样本
};*/
private:
    SwrContext* swrCtx = nullptr;//重采样上下文指针
    FFAudioPars* srcPars = nullptr;// 源音频参数
    FFAudioPars* dstPars = nullptr;// 目标音频参数
    AVChannelLayout srcLayout;
    AVChannelLayout dstLayout;
};

#endif // FFARESAMPLER_H
