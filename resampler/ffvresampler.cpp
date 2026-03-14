#include "ffvresampler.h"
#include "decoder/ffvdecoder.h"
#include <iostream>
FFVResampler::FFVResampler() {}

FFVResampler::~FFVResampler(){
    if(swsCtx){
        sws_freeContext(swsCtx);
    }
    if(srcPars){
        delete srcPars;
        srcPars = nullptr;
    }
    if(dstPars){
        delete dstPars;
        dstPars = nullptr;
    }
    if(vBuffer){
        av_freep(&vBuffer);
        vBuffer = nullptr;
        // 总结区别:
        //av_free只释放内存，不改变指针的值。
        //av_freep释放内存，并将指针设置为NULL。

    }
}
void FFVResampler::init(FFVideoPars *srcPars_,FFVideoPars *dstPars_)
{
    srcPars = new FFVideoPars();
    memcpy(srcPars,srcPars_,sizeof(FFVideoPars));
    dstPars = new FFVideoPars();
    memcpy(dstPars,dstPars_,sizeof(FFVideoPars));
    std::cout<<"=======src video format ==============="<<std::endl;
    std::cout<<"width:"<<srcPars->width<<std::endl;
    std::cout<<"height:"<<srcPars->height<<std::endl;
    std::cout<<"framerate"<<srcPars->frameRate.num<<"/"
              <<srcPars->frameRate.den<<std::endl;
    std::cout<<"pixFmt"<<av_get_pix_fmt_name(srcPars->pixFmtEnum)<<std::endl;


    std::cout<<"========std video format==============="<<std::endl;
    std::cout<<"width:"<<dstPars->width<<std::endl;
    std::cout<<"height"<<dstPars->height<<std::endl;
    std::cout<<"framerate"<<dstPars->frameRate.num<<"/"
              <<dstPars->frameRate.den<<std::endl;
    std::cout<<"pixFmt"<<av_get_pix_fmt_name(dstPars->pixFmtEnum)<<std::endl;

    initSws();
}

void FFVResampler::resample(AVFrame *srcFrame, AVFrame **dstFrame)
{
    *dstFrame = allocFrame(dstPars,srcFrame);
    if(*dstFrame == nullptr){
        return;
    }
    sws_scale(swsCtx,srcFrame->data,srcFrame->linesize
              ,0,srcFrame->height,(*dstFrame)->data,(*dstFrame)->linesize);
 //    /**
 // * 缩放源图像切片并将结果切片放入目标图像中。
 // * 切片是图像中连续行的序列。需要一个先前通过 sws_init_context() 初始化的上下文。
 // *
 // * 切片必须按顺序提供，可以是自上而下或自下而上的顺序。
 // * 如果以非顺序方式提供切片，函数的行为是未定义的。
 // *
 // * @param c         通过 sws_getContext() 先前创建的缩放上下文
 // * @param srcSlice  包含源切片各平面指针的数组
 // * @param srcStride 包含源图像每个平面跨度的数组
 // * @param srcSliceY 源图像中要处理的切片位置，
 // *                  即切片第一行在图像中的行号（从0开始计数）
 // * @param srcSliceH 源切片的高度，即切片中的行数
 // * @param dst       包含目标图像各平面指针的数组
 // * @param dstStride 包含目标图像每个平面跨度的数组
 // * @return          输出切片的高度
 // */

}

void FFVResampler::initSws()
{
    swsCtx = sws_getContext(srcPars->width,
                            srcPars->height,
                            srcPars->pixFmtEnum,
                            dstPars->width,
                            dstPars->height,
                            dstPars->pixFmtEnum,
                            SWS_FAST_BILINEAR,nullptr,
                            nullptr,nullptr);
    if(!swsCtx){
        std::cerr<<"sws_getContext error!"<<std::endl;
        return;
    }
}

AVFrame *FFVResampler::allocFrame(FFVideoPars *vPars, AVFrame *srcFrame)
{
    //分配缓冲区
    AVFrame* frame = av_frame_alloc();
    int bufSize = av_image_get_buffer_size(vPars->pixFmtEnum,
                                           vPars->width,vPars->height,1);
    //av_image_get_buffer_size 函数用于计算指定像素格式、图像宽度和高度以及对齐方式下，存储一帧图像所需缓冲区的大小。
    if(bufSize>maxbufSize){//超出最大缓冲区，重新分配下vBuffer(uint_8*)
        maxbufSize = bufSize;
        if(vBuffer){
            av_freep(&vBuffer);
        }
        vBuffer = static_cast<uint8_t*>(av_mallocz(bufSize));
        if(!vBuffer){
            av_frame_free(&frame);
            std::cerr<<"malloc vBuffer error!"<<std::endl;
            return nullptr;//缓冲区分配失败
        }
    }
    //填充缓冲区
    int ret = av_image_fill_arrays(frame->data,frame->linesize,vBuffer,
                                    vPars->pixFmtEnum,vPars->width,
                                   vPars->height,1);
    if(ret<0){
        printError(ret);
        av_frame_free(&frame);
        return nullptr;
    }
    frame->width = vPars->width;
    frame->height = vPars->height;
    frame->format = vPars->pixFmtEnum;
    frame->pts = srcFrame->pts;
    return frame;
}

void FFVResampler::printError(int ret)
{
    char errorBuffer[AV_ERROR_MAX_STRING_SIZE];
    int res = av_strerror(ret,errorBuffer,sizeof errorBuffer);
    if(res<0){
        std::cerr<<"Unknow Error"<<std::endl;
    }
    else{
        std::cerr<<"Error:"<<errorBuffer<<std::endl;
    }

}
