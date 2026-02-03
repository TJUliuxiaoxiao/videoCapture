#include "ffaresampler.h"
#include "decoder/ffadecoder.h"
FFAResampler::FFAResampler() {}
FFAResampler::~FFAResampler()
{
    if(swrCtx){
        swr_free(&swrCtx);
    }
    if(srcPars){
        delete srcPars;
        srcPars = nullptr;

    }
    if(dstPars){
        delete dstPars;
        dstPars = nullptr;
    }
}

void FFAResampler::init(FFAudioPars *src_,FFAudioPars *dst_)
{
    srcPars = new FFAudioPars();
    memcpy(srcPars,src_,sizeof(FFAudioPars));
    dstPars = new FFAudioPars();
    memcpy(dstPars,dst_,sizeof(FFAudioPars));
    initSwr();
}

//重采样
void FFAResampler::resample(AVFrame* srcFrame,AVFrame** dstFrame){
    //获取重采样延迟
    int64_t delaySamples = swr_get_delay(swrCtx,srcPars->sampleRate);
    //计算最大输出样本数
    // 假设：输入1024样本，延迟50样本，采样率从44100→16000
    // 输出样本数 ≈ (1024 + 50) × (16000/44100) ≈ 390样本
    int maxNbSamples = swr_get_out_samples(swrCtx,srcFrame->nb_samples + delaySamples);
    //分配输出帧,调用 allocFrame 方法分配内存
    *dstFrame = allocFrame(dstPars,maxNbSamples,srcFrame);
    //如果分配失败
    if(!*dstFrame){
        std::cerr<<"av_frame_alloc error!"<<std::endl;
        return;
    }
    //调整输出帧的PTS,显示时间戳
    //输出PTS = (输入PTS + 延迟样本) × (目标采样率 / 源采样率)
    (*dstFrame)->pts = av_rescale_q(
        srcFrame->pts + delaySamples,
        (AVRational){1,srcPars->sampleRate},
        (AVRational){1,dstPars->sampleRate});

    //返回时间转换的样本数
    //这是一个指向数组
    /*int swr_convert(struct SwrContext *s, uint8_t * const *out, int out_count,
                                const uint8_t * const *in , int in_count);*/
    //out 是一个指针，指向一个常量指针，这个常量指针又指向 uint8_t 数据。
    //在swr_convert函数中，out参数用于输出音频数据。它通常是一个指针数组，
    //每个指针指向一个声道的数据（对于平面格式）或者只有一个指针指向所有声道交错的数据（对于打包格式）。
    int samples = swr_convert(
        swrCtx,
        (*dstFrame)->data,//输出数据缓冲区
        maxNbSamples,//输出缓冲区容量
        (const uint8_t **) srcFrame->data,//输入数据
        srcFrame->nb_samples//输入样本数
        );
    if(samples < 0){
        printError(samples);
        swr_free(&swrCtx);
        return ;

    }
    //更新输出帧的实际样本数
    (*dstFrame)->nb_samples = samples;
}


void FFAResampler::initSwr()
{
#if LIBAVUTIL_VERSION_INT < AV_VERSION_INT(57,100,0)
    swrCtx = swr_alloc_set_opts(swrCtx,
                                av_get_default_layout(dstPars->nbChannels),
                                dstPars->aFormatEnum,dstPars->samplerate,
                                srcPars->aFormatEnum,srcPars->samplerate,
                                0,nullptr);
#else
    //新版本使用AVChannelLayout 和swr_alloc_set_opts2
    //初始化默认通道布局
    av_channel_layout_default(&srcLayout,srcPars->nbChannels);
    av_channel_layout_default(&dstLayout,dstPars->nbChannels);
//创建并配置SwrContext
    int ret = swr_alloc_set_opts2(&swrCtx,
                                 &dstLayout,dstPars->aFormatEnum,dstPars->sampleRate,
                                 &srcLayout,srcPars->aFormatEnum,srcPars->sampleRate,0,nullptr);
    if(ret<0){
        std::cerr<<"Swr Alloc Set Opts Fail !"<<std::endl;
        printError(ret);
        swrCtx = nullptr;
    }
    //清理通道布局资源
    av_channel_layout_uninit(&srcLayout);
    av_channel_layout_uninit(&dstLayout);
#endif
    if(!swrCtx){
        std::cerr<<"initSwr error!"<<std::endl;
        return;
    }
    ret = swr_init(swrCtx);
    if(ret<0){
        printError(ret);
        swr_free(&swrCtx);
        return;
    }
}

AVFrame *FFAResampler::allocFrame(FFAudioPars* aPars, int nbSamples, AVFrame *srcFrame)
{

    AVFrame* frame = av_frame_alloc();
    if(!frame){
        return nullptr;
    }
    frame->format = aPars->aFormatEnum;
    frame->sample_rate = aPars->sampleRate;
    frame->nb_samples = nbSamples;
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(57,100,0)
    av_channel_layout_default(&frame->ch_layout,aPars->nbChannels);
#else
    //旧版本兼容
    frame->channels = aPars->nbChannels;
    frame->channels_layout = av_get_default_channel_layout(aPars->nbChannels);
#endif
    if(srcFrame){
        frame->pts = srcFrame->pts;
    }
    int ret = av_frame_get_buffer(frame,0);
    if(ret<0){
        printError(ret);
        av_frame_unref(frame);
        av_frame_free(&frame);
        return nullptr;
    }
    return frame;
}

void FFAResampler::printError(int ret)
{
    char errorBuffer[AV_ERROR_MAX_STRING_SIZE];
    int res = av_strerror(ret,errorBuffer,sizeof(errorBuffer));
    if(res<0){
        std::cerr<<"Unknow Error!"<<std::endl;
    }else{
        std::cerr<<"Error:"<<errorBuffer;
    }
}
