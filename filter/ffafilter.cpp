#include "ffafilter.h"
#include "queue/ffaframequeue.h"
#include "decoder/ffadecoder.h"
#include "clock/ffglobalclock.h"
#include "capture/ffcapturecontext.h"
using namespace FFCaptureContextType;

#define FF_AUDIO_TIME_BASE {1,48000}
FFAFilter::FFAFilter() {}

FFAFilter::~FFAFilter()
{
    if(filterGraph){
        avfilter_graph_free(&filterGraph);
    }
    if(singleFilterGraph[0]){
        avfilter_graph_free(&singleFilterGraph[0]);
    }
    if(singleFilterGraph[1]){
        avfilter_graph_free(&singleFilterGraph[1]);
    }
}


void FFAFilter::init(FFAFrameQueue *encoderFrmQueue_,FFADecoder* aDecoder1_,FFADecoder *aDecoder2_)
{
    encoderFrmQueue = encoderFrmQueue_;
    aDecoder1 = aDecoder1_;
    aDecoder2 = aDecoder2_;
}


AVRational FFAFilter::getTimeBase()
{
    return FF_AUDIO_TIME_BASE;
}

AVMediaType FFAFilter::getMediaType()
{
    return AVMEDIA_TYPE_AUDIO;
}

void FFAFilter::setAudioVolume(double value)
{//音频滤波器类中的音量设置函数
    std::lock_guard<std::mutex> lock(mutex);
    audioVolume = value;//设置音量
    if(singleFilterGraph[0]){
        //输出调试信息，显示正在调整的音量值
        std::cout<<"adjust audio volume:"<<audioVolume<<std::endl;
        //销毁现有的音频滤波器
        destroyFilterGraph(singleFilterGraph[0]);
        //重新初始化单个滤波器,使用新的音量值
        initSingleFilter(aDecoder1->getCodecCtx(),
                         aDecoder1->getStream(),AUDIO);
    }
}

void FFAFilter::destroyFilterGraph(AVFilterGraph* filterGraph)
{
    if(filterGraph){
        //释放滤波器图
        avfilter_graph_free(&filterGraph);
        filterGraph = nullptr;//多余
    }
}

void FFAFilter::setMicrophoneVolume(double value)
{//设置麦克风音量值
    std::lock_guard<std::mutex> lock(mutex);
    microphoneVolume = value;
    if(singleFilterGraph[1]){
        std::cout<<"adjust microphone volume:"<<microphoneVolume<<std::endl;
        //通过销毁并重新创建滤波器图来实现音量更新。
        destroyFilterGraph(singleFilterGraph[1]);
        initSingleFilter(aDecoder2->getCodecCtx(),aDecoder2->getStream(),MICROPHONE);
    }
}

void FFAFilter::setVolume(double value1,double value2)
{//同时设置两个音量的函数,统一滤波器图
    std::lock_guard<std::mutex> lock(mutex);
    if(value1 !=-1){
        audioVolume = value1;
    }
    if(value2 !=-1){
        microphoneVolume = value2;
    }
    if(filterGraph){
        std::cout<<"adjust audio volume:"<<audioVolume<<std::endl;
        std::cout<<"adjust microphone volume:"<<microphoneVolume<<std::endl;
        destroyFilterGraph(filterGraph);
        initFilter(aDecoder1->getCodecCtx(),aDecoder2->getCodecCtx(),
                   aDecoder1->getStream(),aDecoder2->getStream());
    }
}


int FFAFilter::sendFilter(AVFrame* frame1,AVFrame* frame2,int64_t start,int64_t pauseTime)
{//两个音频帧，分别代表两个音频源,start:起始时间戳，用于计算处理后的时间戳
    //参数pauseTime:暂停时间，用于调整时间戳。
    std::lock_guard<std::mutex>lock(mutex);
    if(!frame1||!frame2){//释放空帧
        if(frame1){
            av_frame_unref(frame1);
            av_frame_free(&frame1);
        }
        if(frame2){
            av_frame_unref(frame2);
            av_frame_free(&frame2);
        }
        return 0;
    }
    //初始化滤镜图
    if(!bufferCtx1||!bufferCtx2){
        initFilter(aDecoder1->getCodecCtx(),aDecoder2->getCodecCtx(),
                   aDecoder1->getStream(),aDecoder2->getStream());
    }
    //发送音频帧到滤镜图
    int ret = av_buffersrc_add_frame_flags(bufferCtx1,frame1,AV_BUFFERSRC_FLAG_KEEP_REF);
    //av_buffersrc_add_frame_flags将frame1发送到滤镜图的输入bufferCtx1，
    //并设置AV_BUFFERSRC_FLAG_KEEP_REF标志，表示保持对帧的引用，即不会复制帧数据，而是增加引用计数。这样可以避免数据拷贝
    if(ret<0){
        std::cerr<<"Failed to send frame to buffer1"<<std::endl;
        printError(ret);
        return -1;
    }
    ret = av_buffersrc_add_frame_flags(bufferCtx2,frame2,AV_BUFFERSRC_FLAG_KEEP_REF);
    if(ret<0){
        std::cerr<<"Failed to send frame to buffer2"<<std::endl;
        printError(ret);
        return -1;
    }
    //从滤镜图循环读取所有处理后的帧
    while(true){
        AVFrame* filterFrame = av_frame_alloc();
        ret = av_buffersink_get_frame(bufferSinkCtx,filterFrame);//从滤波器图的输出接收器（sink）获取一帧处理后的数据。
        if(ret == AVERROR(EAGAIN)||ret==AVERROR_EOF){
            av_frame_free(&filterFrame);
            break;
        }
        if(ret<0){
            std::cerr<<"Failed to get frame from sink"<<std::endl;
            printError(ret);
            av_frame_free(&filterFrame);
            return -1;
        }
        encoderFrmQueue->enqueue(filterFrame);//将处理后的帧放入编码队列
        //调整时间戳
        auto end = av_gettime_relative();
        int64_t duration = (end - start);
        // filterFrame->pts = av_gettime_relative()*10 +
        //                    duration - pauseTime*10;
        int64_t elapsed_us = av_gettime_relative()+duration - pauseTime;  // 假设 start 是录制开始时间（微秒）
        filterFrame->pts = av_rescale_q(elapsed_us, (AVRational){1, 1000000}, FF_AUDIO_TIME_BASE);
        // av_frame_unref(filterFrame);
        // av_frame_free(&filterFrame);
    }
    av_frame_unref(frame1);
    av_frame_free(&frame1);
    av_frame_unref(frame2);
    av_frame_free(&frame2);
    return 0;

}


void FFAFilter::sendFrame(AVFrame *frame,int64_t start,int64_t pauseTime)
{
    if(frame == nullptr){
        return;
    }
    // auto end = av_gettime_relative()*10;
    // auto duration = (end - start);
    // frame->pts = av_gettime_relative()*10-duration-pauseTime*10;
    auto end = av_gettime_relative();
    auto duration = (end - start);
    int64_t elaped_us = av_gettime_relative()-duration-pauseTime;
    frame->pts = av_rescale_q(elaped_us,(AVRational){1, 1000000}, FF_AUDIO_TIME_BASE);
    encoderFrmQueue->enqueue(frame);
    av_frame_free(&frame);
}

void FFAFilter::initFilter(AVCodecContext *codecCtx1_,
                           AVCodecContext *codecCtx2_,
                           AVStream *stream1_,
                           AVStream *stream2_)
{
    codecCtx1 = codecCtx1_;
    codecCtx2 = codecCtx2_;
    stream1 = stream1_;
    stream2 = stream2_;

    filterGraph = avfilter_graph_alloc();
    if(!filterGraph){
        throw std::runtime_error("Failed to allocate filter graph");
    }
    //创建输入缓冲区滤镜
    createBufferFilter(filterGraph,&bufferCtx1,codecCtx1,stream1,"in1");
    createBufferFilter(filterGraph,&bufferCtx2,codecCtx2,stream2,"in2");

    //创建音量滤镜
    createAmixFilter(filterGraph);

    //创建降噪滤镜
    // createAfftdnFilter(filterGraph);
    //创建输出sink滤镜
    createBufferSinkFilter(filterGraph,&bufferSinkCtx,"out");

    //构建滤镜链
    linkFilters(bufferCtx1,0,volumeCtx1,0);//输入1->音量
    linkFilters(volumeCtx1,0,amixCtx,0);//音量->混音

    linkFilters(bufferCtx2,0,volumeCtx2,0);//输入2->音量
    linkFilters(volumeCtx2,0,amixCtx,1);//音量->混音
    // linkFilters(amixCtx,0,afftdnCtx,0);//混音->降噪
    linkFilters(amixCtx,0,bufferSinkCtx,0);//降噪->输出

    //配置滤镜图
    avfilter_graph_set_auto_convert(filterGraph,AVFILTER_AUTO_CONVERT_ALL);
    //这意味着允许滤波器图在需要时自动插入转换滤波器（例如，当两个相连的滤波器格式不匹配时）。
    int ret = avfilter_graph_config(filterGraph,nullptr);
    //对图中的所有连接和格式进行检查和配置。
    if(ret<0){
        printError(ret);
        throw std::runtime_error("Filter graph config failed");
    }
}

//辅助函数:创建输入缓冲区滤镜
void FFAFilter::createBufferFilter(AVFilterGraph* filterGraph,
                                   AVFilterContext** ctx,AVCodecContext* codecCtx,
                                   AVStream* stream,const char* name)
{
    /*这个函数创建了FFmpeg滤波器图的音频输入端点（abuffer过滤器）。它配置了音频流的基本参数，
     * 使得原始音频数据能够正确进入滤波器链进行处理。在实际使用中，需要注意正确处理不同的音频格式
     * 和通道布局，避免使用硬编码的参数值。*/
    /*参数
    filterGraph: 滤波器图，将在其中创建过滤器
    ctx: 输出参数，用于返回创建的过滤器上下文
    codecCtx: 解码器上下文，提供音频参数（如通道数）
    stream: 流，提供时间基
    name: 给这个过滤器实例起的名字*/

    const AVFilter* bufferFilter = avfilter_get_by_name("abuffer");//音频缓冲滤波器
    if(!bufferFilter){
        throw std::runtime_error("abuffer filter not found");
    }
    AVRational timeBase1 = stream->time_base;//从stream中获取时间基

    //设置音频通道布局
    AVChannelLayout layout = {};
    uint64_t channel_mask = 0;


    //根据解码器的通道数创建默认通道布局
    av_channel_layout_default(&layout,codecCtx->ch_layout.nb_channels);

    //如果布局是原生类型(使用掩码),获取通道掩码
    if(layout.order == AV_CHANNEL_ORDER_NATIVE){
        channel_mask = layout.u.mask;
    }

    //构建过滤器参数字符串
    std::ostringstream args;
    args<<"time_base="<<timeBase1.num<<"/"<<timeBase1.den
         <<":sample_rate="<<FF_SAMPLE_RATE<<":sample_fmt="
         <<av_get_sample_fmt_name(FF_SAMPLE_FMT)<<":channel_layout=0x"
         <<std::hex<<channel_mask;

    //创建过滤器实例
    int ret = avfilter_graph_create_filter(ctx,bufferFilter,name,
                                           args.str().c_str(),nullptr,
                        filterGraph);
    if(ret<0){
        printError(ret);
        throw std::runtime_error("Failed to create abuffer filter");
    }
}


//通用滤镜创建函数,// 简化了原始FFmpeg API调用
AVFilterContext* FFAFilter::createFilter(AVFilterGraph* filterGraph,
                                         const AVFilter* filter,const char* name,const char* filterArgs)
{
    AVFilterContext* ctx = nullptr;
    int ret = avfilter_graph_create_filter(&ctx,filter,name,filterArgs,nullptr,filterGraph);
    if(ret < 0){
        printError(ret);
        throw std::runtime_error(std::string("Failed to create")+name);

    }
    return ctx;
}

//创建混音滤镜
void FFAFilter::createAmixFilter(AVFilterGraph* filterGraph)
{
    const AVFilter* amixFilter = avfilter_get_by_name("amix");
    amixCtx = createFilter(filterGraph,amixFilter,"amix","inputs=2:duration=longest");;
}

//创建降噪滤镜
void FFAFilter::createAfftdnFilter(AVFilterGraph* filterGraph)
{
    const AVFilter* afftdnFilter = avfilter_get_by_name("afftdn");
    afftdnCtx = createFilter(filterGraph,afftdnFilter,"afftdn",
                             "nf=-60:track_noise=1");
}

//创建输出sink滤镜
void FFAFilter::createBufferSinkFilter(AVFilterGraph* filterGraph,
                                       AVFilterContext**ctx,const char *name)
{
    const AVFilter* sinkFilter = avfilter_get_by_name("abuffersink");//（这是一个音频缓冲sink，用于从滤镜图获取音频数据）
    *ctx = createFilter(filterGraph,sinkFilter,name,nullptr);
    //设置输出格式
    enum AVSampleFormat fmts[] = {AV_SAMPLE_FMT_FLTP,AV_SAMPLE_FMT_NONE};
    av_opt_set_int_list(*ctx,"sample_fmts",fmts,AV_SAMPLE_FMT_NONE,
                        AV_OPT_SEARCH_CHILDREN);
    // av_opt_set_int_list(bufferSinkCtx, "sample_fmts", fmts,
    //                     AV_SAMPLE_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
}

//创建音量滤镜
void FFAFilter::createVolumeFilter(AVFilterGraph* filterGraph,
                                   AVFilterContext** ctx,double gain,const char* name)
{
    const AVFilter* volumeFilter = avfilter_get_by_name("volume");
    if(!volumeFilter){
        throw std::runtime_error("volume filter not found");
    }
    char args[64];
    snprintf(args,sizeof(args),"volume=%.2f:eval=frame",gain);
    int ret = avfilter_graph_create_filter(ctx,volumeFilter,name,args,nullptr,filterGraph);
    if(ret<0){
        printError(ret);
        throw std::runtime_error("Failed to create volume filter:"+std::string(name));
    }
}

void FFAFilter::initSingleFilter(AVCodecContext *codecCtx, AVStream *stream, int type)
{
    size_t index = (type == AUDIO)?0:1;
    singleFilterGraph[index] = avfilter_graph_alloc();
    if(!singleFilterGraph[index]){
        throw std::runtime_error("Failed to allocate filter graph");
    }
    //创建输入缓冲区滤镜
    const std::string bufferFilterName = "single_in" + std::to_string(index);
    createBufferFilter(singleFilterGraph[index],&singleBufferCtx[index],codecCtx,stream,
                       bufferFilterName.c_str());
    //创建音量滤镜
    const std::string volumeFilterName = "single_volume"+std::to_string(index);
    double volume = (type==AUDIO)?audioVolume:microphoneVolume;
    createVolumeFilter(singleFilterGraph[index],&singleVolumeCtx[index],volume,volumeFilterName.c_str());
    //创建输出sink滤镜
    const std::string sinkFilterName = "single_out" + std::to_string(index);
    createBufferSinkFilter(singleFilterGraph[index],&singleBufferSinkCtx[index],sinkFilterName.c_str());
    //构建滤镜链
    linkFilters(singleBufferCtx[index],0,
                singleVolumeCtx[index],0);
    linkFilters(singleVolumeCtx[index],0,
                singleBufferSinkCtx[index],0);
    //配置滤镜图
    avfilter_graph_set_auto_convert(singleFilterGraph[index],AVFILTER_AUTO_CONVERT_ALL);
    int ret = avfilter_graph_config(singleFilterGraph[index],nullptr);
    if(ret <0){
        printError(ret);
        throw std::runtime_error("Filter graph config failed");
    }
}

void FFAFilter::sendSingleFilter(AVFrame* frame,int64_t start,
                                 int64_t pauseTime,int type)
{

    if(frame == nullptr){
        return;
    }
    //使用RAII确保异常安全
    // std::unique_ptr<AVFrame,void(*)(AVFrame*)> frame_guard(frame,[](AVFrame* f){
    //     if(f){
    //         av_frame_free(&f);
    //     }
    // });
    std::lock_guard<std::mutex> lock(mutex);
    size_t index = (type==AUDIO)?0:1;
    if(!singleFilterGraph[index]){
        if(index==0){
            initSingleFilter(aDecoder1->getCodecCtx(),
                             aDecoder1->getStream(),AUDIO);
        }else{
            initSingleFilter(aDecoder2->getCodecCtx(),
                             aDecoder2->getStream(),MICROPHONE);
        }
    }
    //发送音频帧到滤镜图
    // int ret = av_buffersrc_add_frame_flags(singleBufferCtx[index],frame,
    //                                        AV_BUFFERSRC_FLAG_KEEP_REF);
    int ret = av_buffersrc_add_frame_flags(singleBufferCtx[index],frame,
                                           AV_BUFFERSRC_FLAG_KEEP_REF);

    if(ret < 0){
        std::cerr<<"Failed to send frame to singleBufferCtx"<<std::endl;
        printError(ret);
        return;
    }
    // frame_guard.release();
    //循环读取所有处理后的帧
    while(true){
        AVFrame* filterFrame = av_frame_alloc();
        ret = av_buffersink_get_frame(singleBufferSinkCtx[index],filterFrame);
        if(ret == AVERROR(EAGAIN)||ret==AVERROR_EOF){
            av_frame_free(&filterFrame);
            break;
        }
        if(ret<0){
            std::cerr<<"Failed to get frame from sink"<<std::endl;
            printError(ret);
            av_frame_free(&filterFrame);
            break;
        }
        encoderFrmQueue->enqueue(filterFrame);
        //调整时间戳
        auto end = av_gettime_relative();
        int64_t duration = 10*(end-start);
        filterFrame->pts = av_gettime_relative()+duration-pauseTime*10;
        // av_frame_unref(filterFrame);
        // av_frame_free(&filterFrame);
    }
    av_frame_unref(frame);
    av_frame_free(&frame);

}



//安全链接滤镜
void FFAFilter::linkFilters(AVFilterContext* src,int srcPad,AVFilterContext* dst,int dstPad)
{
    int ret = avfilter_link(src,srcPad,dst,dstPad);
    if(ret < 0){
        printError(ret);
        throw std::runtime_error("Filter linking Failed");
    }
}

void FFAFilter::printError(int ret)
{
    char errorBuffer[AV_ERROR_MAX_STRING_SIZE];
    int res = av_strerror(ret,errorBuffer,sizeof errorBuffer);
    if(res<0){
        std::cerr<<"Unknow Error!"<<std::endl;
    }
    else{
        std::cerr<<"Error:"<<errorBuffer<<std::endl;
    }

}
