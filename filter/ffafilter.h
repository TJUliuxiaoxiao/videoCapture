#ifndef FFAFILTER_H
#define FFAFILTER_H

#include <iostream>
#include <thread>
#include <sstream>
#include <mutex>

extern "C"{
#include "libavformat/avformat.h"
#include "libavfilter/avfilter.h"
#include <libavutil/opt.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/time.h>
#include <libavcodec/avcodec.h>
}

// 混音保持一致48kHz FLTP
#define FF_SAMPLE_RATE 48000
#define FF_SAMPLE_FMT AV_SAMPLE_FMT_FLTP
class FFAFrameQueue;
class FFADecoder;

class FFAFilter
{
public:
    FFAFilter();
    ~FFAFilter();
    void init(FFAFrameQueue *encoderFrmQueue_,//传入编码器帧队列和两个解码器
              FFADecoder* aDecoder1_,FFADecoder* aDecoder2_);
    int sendFilter(AVFrame* frame1,AVFrame* frame2,int64_t start,int64_t pauseTime);//发送两个音频帧进行滤波处理
    AVRational getTimeBase();
    enum AVMediaType getMediaType();//返回媒体类型
    //设置音量的函数
    void setAudioVolume(double value);
    void setMicrophoneVolume(double value);
    void setVolume(double value1,double value2);

    //发送单个音频帧到滤波器
    void sendFrame(AVFrame* frame,int64_t start,
                   int64_t pauseTime);
    //发送单个滤波器的帧
    void sendSingleFilter(AVFrame* frame,int64_t start,int64_t pauseTime,int type);
private:
    //初始化滤波器图,需要两个音频流的解码器上下文和流信息
    void initFilter(AVCodecContext* codecCtx1_,
                    AVCodecContext* codecCtx2_,
                    AVStream* stream1_,
                    AVStream* stream2_);
    //打印错误信息
    void printError(int ret);
    //连接两个滤波器
    void linkFilters(AVFilterContext* src,int srcPad,
                     AVFilterContext *dst,int dstPad);
    //创建滤波器示例
    AVFilterContext *createFilter(AVFilterGraph* filterGraph,const AVFilter* filter,
                                  const char *name,const char *filterArg);

    //创建缓冲区接收滤波器
    void createBufferSinkFilter(AVFilterGraph* filterGraph,AVFilterContext **ctx,const char *name);

    //创建降噪滤波器
    void createAfftdnFilter(AVFilterGraph* filterGraph);
    //创建混音滤波器
    void createAmixFilter(AVFilterGraph* filterGraph);

    //创建缓冲区源滤波器
    void createBufferFilter(AVFilterGraph* filterGraph,AVFilterContext **ctx,
                            AVCodecContext *codecCtx,AVStream *stream,const char *name);
    //创建音量调整滤波器
    void createVolumeFilter(AVFilterGraph* filterGraph,AVFilterContext **ctx,
                            double gain,const char *name);
    //初始化单个滤波器图
    void initSingleFilter(AVCodecContext* codecCtx,
                          AVStream* stream,int type);
    //销毁滤波器图
    void destroyFilterGraph(AVFilterGraph* filterGraph);
private:
    FFAFrameQueue* encoderFrmQueue = nullptr;
    FFADecoder* aDecoder1 = nullptr;
    FFADecoder* aDecoder2 = nullptr;

    AVCodecContext* codecCtx1 = nullptr;
    AVCodecContext* codecCtx2 = nullptr;
    AVStream* stream1 = nullptr;
    AVStream* stream2 = nullptr;
    /*这是一个结构体 AVFilterGraph 的定义，它代表一个滤波器图（filter graph）。
     * 滤波器图是FFmpeg中用于处理多媒体数据（如音频、视频）的滤波器网络。
     * 它由多个滤波器（AVFilterContext）以及它们之间的连接（AVFilterLink）组成。*/
    //主滤波器系统(混音)
    AVFilterGraph* filterGraph = nullptr;
    // bufferCtx1 (音频1) → volumeCtx1 → amixCtx → afftdnCtx → bufferSinkCtx
    //                  ↑
    //     bufferCtx2 (音频2) → volumeCtx2
    /*AVFilterContext 是 FFmpeg 滤波器系统的核心数据结构：
    表示单个滤波器实例：包含状态、连接和私有数据
    构建滤波器图的基础单元：通过链接形成处理链
    支持多种高级特性：多线程、条件执行、硬件加速
    生命周期由滤波器图管理：通常不需要手动释放*/
    AVFilterContext* bufferCtx1 = nullptr;
    AVFilterContext* bufferCtx2 = nullptr;
    AVFilterContext* bufferSinkCtx = nullptr;
    AVFilterContext* scaleCtx = nullptr;
    AVFilterContext* amixCtx = nullptr;
    AVFilterContext* afftdnCtx = nullptr;
    AVFilterContext* volumeCtx1 = nullptr;
    AVFilterContext* volumeCtx2 = nullptr;
    std::mutex mutex;//线程安全
    double audioVolume = 1.0;//背景音乐音量
    double microphoneVolume = 1.0;//麦克风音量
    //AUDIO = 0,MICROPHONE = 1;
    //单通道滤波器图(单独处理）
    AVFilterGraph* singleFilterGraph[2] = {nullptr,nullptr};
    //每个元素都是一个独立的滤波器图，用于单独处理一个音频流。
    //索引0对应背景音乐（AUDIO），索引1对应麦克风（MICROPHONE）。
    AVFilterContext* singleBufferCtx[2] = {nullptr,nullptr};
    //AVFilterContext指针的数组。
    //每个元素是一个输入缓冲滤波器（例如，使用“abuffer”滤波器）的上下文，
    //用于将音频帧送入对应的单通道滤波器图。
    AVFilterContext* singleBufferSinkCtx[2] = {nullptr,nullptr};
    //是一个包含两个AVFilterContext指针的数组。
    //每个元素是一个输出缓冲接收器（例如，使用“abuffersink”滤波器）的上下文，
    //用于从对应的单通道滤波器图中接收处理后的音频帧。
    AVFilterContext* singleVolumeCtx[2] = {nullptr,nullptr};
    //每个元素是一个音量控制滤波器（例如，使用“volume”滤波器）的上下文，
    //用于调整对应音频流的音量
    //两个不同的音频流，分别是背景音乐和麦克风输入
};

#endif // FFAFILTER_H
