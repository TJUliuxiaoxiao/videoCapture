#include "ffvideoadapter.h"

FFVideoAdapter::FFVideoAdapter() {

}
FFVideoAdapter::~FFVideoAdapter()
{
    if(sws_ctx){
        sws_free_context(&sws_ctx);
    }
}

AVFrame *FFVideoAdapter::convertMatToFrame(const cv::Mat &bgrMat)
{/*函数定义，接受一个常量引用 cv::Mat（应为 BGR 三通道 8 位图像），
返回一个指向新分配的 AVFrame 的指针。调用者需负责释放返回的
AVFrame（使用 av_frame_free）*/


    //分配AVFrame结构体
    AVFrame* frame = av_frame_alloc();
    if(!frame){
        std::cerr<<"Error Could not alloc frame"<<std::endl;
        return nullptr;
    }
    //获取图像尺寸
    int width = bgrMat.cols;
    int height = bgrMat.rows;
    frame->format = AV_PIX_FMT_YUV420P;
    frame->width = width;
    frame->height = height;
    //分配数据缓存区
    int ret = av_frame_get_buffer(frame,0);
    if(ret<0){
        std::cerr<<"Error:Could not aloccate the video frame data"<<std::endl;
        av_frame_free(&frame);
        return nullptr;
    }
    //bgrMat.step是Mat中一行数据的字节数，通常等于width*channel
    //bgrLinesize 是一个整数数组，存放每个平面的行字节数（即步长），
    //同样只有一个元素。
    int bgrLinesize[1] = {static_cast<int>(bgrMat.step)};

    const uint8_t* bgrData[1] = {bgrMat.data};
    //bgrData 是一个指针数组，包含一个指向 Mat 数据起始地址的指针。
    //sws_scale 的输入参数要求是一个指针数组，
    //因为某些格式可能有多个平面（如 YUV），
    //但对于 BGR 这种 packed 格式，只有一个平面，所以数组大小为 1。

    if(sws_ctx==nullptr||lastH!=height||lastW!=width){
        initSws(width,height);
    }
    sws_scale(sws_ctx,bgrData,bgrLinesize,0,height,frame->data,frame->linesize);
    return frame;
}

cv::Mat FFVideoAdapter::convertFrameToMat(AVFrame *frame)
{
    int width = frame->width;
    int height = frame->height;//图像尺寸
    cv::Mat yuvMat(height*3/2,width,CV_8UC1);
    /*创建一个 OpenCV 矩阵 yuvMat，用于存放 YUV420P 格式的原始数据（打包为连续的平面）。
    尺寸计算：YUV420P 格式中，Y 平面大小为 width * height 字节，U 和 V 平面大小均为
    (width/2) * (height/2) = width * height / 4 字节。因此总数据量为 width*height
    + 2*(width*height/4) = width*height*3/2 字节。
    这里将矩阵的行数设为 height * 3 / 2，列数设为 width，数据类型为 CV_8UC1
    （单通道 8 位无符号整数）。因此矩阵的总元素个数为 (height*3/2) * width =
    width*height*3/2，恰好容纳全部 YUV 数据。
    这种布局通常称为 I420 格式（YUV420P 的平面按 Y、U、V 顺序连续排列），
    OpenCV 的 COLOR_YUV2BGR_I420 转换常量专门处理这种内存布局。*/

    memcpy(yuvMat.data,frame->data[0],width*height);//复制Y平面
    memcpy(yuvMat.data+width*height,frame->data[1],width*height/4);//复制U平面
    memcpy(yuvMat.data+width*height*5/4,
           frame->data[2],width*height/4);//复制V平面
    //经过这三步复制，yuvMat 的内存中形成了一个连续的 I420 数据流。

    //将YUV转换为BGR
    cv::Mat bgrMat;
    cv::cvtColor(yuvMat,bgrMat,cv::COLOR_YUV2BGR_I420);
    //调用OpenCV的颜色转换函数cvtColor，将I420格式的YUV数据转换
    //为BGR格式
    return bgrMat;
}


void FFVideoAdapter::initSws(int width,int height)
{
    if(sws_ctx){
        sws_free_context(&sws_ctx);
    }
    //创建新的上下文
    sws_ctx = sws_getContext(
        width,height,AV_PIX_FMT_BGR24,
        //源图像:宽高像素格式BGR24
        width,height,AV_PIX_FMT_YUV420P,
        //目标图像：宽、高、像素格式 YUV420P
        SWS_FAST_BILINEAR,//缩放算法:快速双线性插值
        nullptr,nullptr,nullptr
        );
    lastW = width;
    lastH = height;
    //将本次使用的宽高保存到成员变量中，
    //供下次调用 initSws 时判断尺寸是否变化，从而决定是否需要重建上下文
    if(!sws_ctx){
        std::cerr<<"Error: Could not initialize "
                     "SwsContext!"<<std::endl;
        return;
    }
}
