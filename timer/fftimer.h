#ifndef FFTIMER_H
#define FFTIMER_H

#include <functional>
#include <thread>
#include <chrono>
#include <iostream>
#include <atomic>
#include <condition_variable>
#include <QMetaObject>
#include <iomanip>

extern "C"{
#include <libavformat/avformat.h>
#include <libavutil/time.h>
}
class FFVFrameQueue;
class FFVRender;
class FFVideoPars;
class FFCapWindow;
class FFTimer
{
public:
    FFTimer();
    ~FFTimer();
    void init(FFVFrameQueue* frmQueue_,
              FFVRender* vRender_,FFCapWindow* capWindow_);
    void start();
    void wait();
    void stop();
    void pause();
    void close();
    void wakeAllThread();
    void setSpeed(float speed_);
    bool peekReadyFlag();
/*
函数	作用
init(...)	初始化，传入帧队列、渲染器、显示窗口，建立协作关系。
start()	启动定时器线程，开始循环取帧并渲染。
wait()	阻塞直到定时器线程结束（通常与 stop 配合）。
stop()	请求停止定时器线程，并等待其退出。
pause()	暂停渲染，保留当前画面，线程进入等待状态。
close()	关闭定时器，释放资源。
wakeAllThread()	唤醒所有因条件变量等待的线程（例如退出时强制唤醒）。
setSpeed(float)	设置播放速度（如 1.0 正常，2.0 倍速）。
peekReadyFlag()	查询是否已准备好显示下一帧（可能用于外部状态检查）。*/

private:
    void work();
    void copyYUV(AVFrame* frame);
    void setTimerInterval(std::chrono::milliseconds
                              interval_);
    void setTimerInterval(double interval_);
    void setTimerInterval(int64_t interval_);

    void playVideo();
private:
    std::chrono::microseconds interval;//当前帧间隔,根据PTS和速度得出
    std::thread timerThread;//定时器线程
    bool m_stop;//停止标志,控制线程退出

    FFVFrameQueue* frmQueue = nullptr;
    FFVideoPars* vPars = nullptr;
    FFVRender* vRender = nullptr;

    std::mutex mutex;
    std::condition_variable cond;

    std::condition_variable pauseCond;
    std::mutex pauseMutex;//暂停/恢复

    std::atomic<bool> seekFlag;
    std::atomic<bool> pauseFlag;
    uint8_t *yBuf  = nullptr;
    uint8_t *uBuf = nullptr;
    uint8_t *vBuf = nullptr;//YUV数据缓冲区,避免每次显示时都重新分配内存

    std::atomic<bool> speedFlag;
    float speed;//播放速度和缩放因子
    float speedFactor;

    FFCapWindow* capWindow = nullptr;
    std::atomic<bool> readyFlag;
};

#endif // FFTIMER_H
