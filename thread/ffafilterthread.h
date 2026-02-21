#ifndef FFAFILTERTHREAD_H
#define FFAFILTERTHREAD_H

#include "ffthread.h"
#include <mutex>
#include <condition_variable>
extern "C"
{
#include <libavutil/time.h>
}
class AVFrame;
class FFAFilter;
class FFAFrameQueue;
class FFAFilterThread : public FFThread
{
public:
    FFAFilterThread();
    virtual ~FFAFilterThread() override;

    void openAudioSource(int audioType);
    void closeAudioSource(int audioType);
    void init(FFAFrameQueue* frmQueue1_,FFAFrameQueue* frmQueue2_,FFAFilter* filter_);
    void startEncoder();
    void stopEncoder();
    void pauseEncoder();

    void setAudioVolume(double value);//设置音量
    void setMicrophoneVolume(double value);

    bool peekStart();
    void wakeAllThread();
protected:
    virtual void run() override;
private:
    AVFrame* generateMuteFrame();
private:
    FFAFrameQueue* frmQueue1 = nullptr;
    FFAFrameQueue* frmQueue2 = nullptr;//camera
    FFAFilter* filter = nullptr;

    AVFrame* audioFrame = nullptr;
    AVFrame* microPhoneFrame = nullptr;
    std::atomic<bool> pauseFlag;
    std::atomic<bool> audioFlag;
    std::atomic<bool> microphoneFlag;
    std::atomic<bool> encoderFlag;

    std::mutex mutex;
    std::condition_variable cond;
    std::atomic<int64_t> pauseTime;
    std::atomic<int64_t> lastpauseTime;

};

#endif // FFAFILTERTHREAD_H
