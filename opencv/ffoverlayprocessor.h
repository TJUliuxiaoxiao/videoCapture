#ifndef FFOVERLAYPROCESSOR_H
#define FFOVERLAYPROCESSOR_H

#include <opencv2/opencv.hpp>
#include "ffvideoadapter.h"
#include <mutex>
//定义了一个用于在视频帧上叠加图像的类
//结合了 OpenCV 的图像处理能力和
//FFmpeg 的帧数据结构，并通过互斥锁保证多线程环境下的安全性。
class FFOverlayProcessor
{
public:
    FFOverlayProcessor();
    void init();
    void sendOverlayImage(AVFrame* frame,
                          int x,int y,int w, int h);
    void sendOverlayImage(cv::Mat &mat,int x, int y,int w,int h);
    void resetBackground();//重置背景对象
    AVFrame* getOverlayFrame();//获取合成后的帧
private:
    void overlayImage(cv::Mat &foreground,int x,int y,int w,int h);//核心叠加算法

private:
    cv::Mat background;//存储背景图像，原始视频帧
    FFVideoAdapter* adapter = nullptr;
    //用于在FFmpeg的AVFrame和Opencv的cv::mat之间进行格式转换
    std::mutex mutex;//互斥锁，保护background和其他内部状态，防止同时调用修改背景的函数
};

#endif // FFOVERLAYPROCESSOR_H
