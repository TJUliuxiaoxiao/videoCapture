#ifndef FFVIDEOADAPTER_H
#define FFVIDEOADAPTER_H
#include <opencv2/opencv.hpp>
extern "C"{
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}
/*头文件定义了一个名为 FFVideoAdapter 的类，
 * 用于在 OpenCV 的 cv::Mat 和 FFmpeg 的
 *  AVFrame 之间进行双向转换。它结合了两个库的数据结构，
 *  常用于视频处理或编解码场景，
 *  例如从视频流中获取帧（FFmpeg）转换为 OpenCV
 *  可处理的图像，或将 OpenCV 处理后的帧转换回
 *   FFmpeg 进行编码。*/
class FFVideoAdapter
{
public:
    FFVideoAdapter();
    ~FFVideoAdapter();
    //将openCV的mat格式(BGR格式)转换成FFmpeg的AVFrame
    AVFrame* convertMatToFrame(const cv::Mat& bgrMat);

    //将FFmpeg的AVFrame转换为Opencv的Mat(BGR)
    cv::Mat convertFrameToMat(AVFrame* frame);
private:
    //初始化或重新初始化像素格式转换上下文(SwsContext)
    void initSws(int width,int height);

    SwsContext* sws_ctx = nullptr;//FFmpeg的像素格式转换上下文,用于颜色空间转换(如YUV<->BGR)
    int lastW = 0,lastH = 0;//记录上一次转换的图像尺寸
    //以便在尺寸变化时重新创建sws_ctx

};

#endif // FFVIDEOADAPTER_H
