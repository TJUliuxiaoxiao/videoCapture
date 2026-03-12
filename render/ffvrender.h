#ifndef FFVRENDER_H
#define FFVRENDER_H

#include <chrono>//用于时间测量和延迟控制,可能实现精确的帧间隔
#include <atomic>
extern "C"{
#include "libavformat/avformat.h"
}
//ffmpeg

class FFTimer;
class FFVFrameQueue;
class FFPlayerContext;
class FFCapWindow;
/*在实际实现中，FFVRender 可能会在一个循环中：
从 frmQueue 中获取下一帧（可能阻塞直到有帧）。
获取当前时间和帧的 PTS，计算需要等待的时间。
使用 timer 进行精确休眠。
将帧数据通过 capWindow->setYUVData(frame) 显示。
处理暂停、停止等控制信号。*/

class FFVRender
{
public:
    FFVRender();
    ~FFVRender();
    void init(FFVFrameQueue* frmQueue_,FFCapWindow* capWindow_);
    void start();
    void stop();
    void pause();
    void wait();
    void close();
    void setSpeed(float speed);
    void wakeAllThread();
    bool peekReadyFlag();
private:
    void initTimer();

private:
    FFVFrameQueue* frmQueue = nullptr;//视频帧队列
    FFTimer* timer = nullptr;//定时器用于控制渲染帧率，
    //可能基于系统时钟或者视频帧的PTS进行精确休眠,实现同步
    FFCapWindow* capWindow = nullptr;
    //视频显示窗口
};

#endif // FFVRENDER_H
