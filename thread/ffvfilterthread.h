#ifndef FFVFILTERTHREAD_H
#define FFVFILTERTHREAD_H

#include "ffthread.h"
#include "opencv/ffoverlayprocessor.h"
extern "C"{
#include <libavutil/time.h>
}
#include <condition_variable>
/*该类负责从多个视频源（屏幕、摄像头、视频文件）获取原始帧，
 * 应用滤镜、人脸检测等处理，然后输出处理后的帧到渲染队列，
 * 供后续编码或显示使用。*/
class FFVFrameQueue;
class FFVFilter;
class FFFaceDetector;
class FFCapWindow;
class AVFrame;
class FFVFilterThread : public FFThread
{
public:
    FFVFilterThread();
    virtual ~FFVFilterThread() override;
public:
    void init(FFVFrameQueue* frmQueue1_,FFVFrameQueue* frmQueue2_,FFVFrameQueue* frmQueue3_,FFVFrameQueue* renderFrmQueue_,FFVFilter* filter_,FFCapWindow* capWindow_);
    void startEncoder();
    void stopEncoder();

    void openVideoSource(int sourceType);
    void closeVideoSource(int sourceType);

    void setWhiteValue(int value);
    void setSmoothValue(int value);

    void pauseEncoder();
    bool peekStart();
    void wakeAllThread();
protected:
    virtual void run() override;
private:
    void overlayFrame(AVFrame * frame,int type);
private:
    FFVFrameQueue* frmQueue1 = nullptr;//屏幕源队列
    FFVFrameQueue* frmQueue2 = nullptr;//摄像头源队列
    FFVFrameQueue* frmQueue3 = nullptr;//视频文件源队列
    FFVFrameQueue* renderFrmQueue = nullptr;//输出到渲染的队列

    FFVFilter* filter = nullptr;//视频滤镜队列

    FFFaceDetector* faceDetector = nullptr;//人脸检测器
    FFCapWindow* capWindow = nullptr;//采集窗口

    FFOverlayProcessor* overlayProcessor = nullptr;//叠加处理器(绘制人脸框)
    int overlayX,overlayY;//叠加区域的起始坐标
    int overlayWidth,overlayHeight;//叠加区域的宽高
    int64_t overlayPts;//叠加帧的时间戳(可能用于同步)
    int64_t overlayDts;//叠加编码帧
    std::vector<int> overlayNumbers;//要叠加的数字(检测到的人脸数)
    std::atomic<bool> encoderFlag;//编码器线程运行标志
    std::atomic<bool> cameraFlag;//摄像头源启用标志
    std::atomic<bool> videoFlag;//视频文件源启用标志
    std::atomic<bool> screenFlag;//屏幕源启用标志

    AVFrame* screenFrame = nullptr;//屏幕源当前帧
    AVFrame* cameraFrame = nullptr;//摄像头当前帧
    AVFrame* cameraFrame2 = nullptr;//摄像头的另一帧
    AVFrame* lastVideoFrame = nullptr;//上一帧视频
    AVFrame* videoFrame = nullptr;//视频文件当前帧
    // AVFrame* videoFrame2 = nullptr;//视频文件另一帧


    size_t cameraCount = 0;//摄像头计数
    std::mutex mutex;//互斥锁
    bool eofFlag = false;//是否到达文件末尾
    std::condition_variable cond;//条件变量,用于线程等待
    int64_t lastVideoTime = 0;//上次处理视频帧的时间，用于控制帧率
    std::atomic<bool> pauseFlag;
    std::atomic<int64_t> pauseTime;//暂停时间
    std::atomic<int64_t> lastPauseTime;
};

#endif // FFVFILTERTHREAD_H
