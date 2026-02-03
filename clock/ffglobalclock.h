#ifndef FFGLOBALCLOCK_H
#define FFGLOBALCLOCK_H
#include <mutex>
#include <chrono>
#include <atomic>
// 用于音视频播放器中的时钟同步，正确实现后可以保证线程安全和高性能。
class FFGlobalClock
{
public:
    FFGlobalClock();
    static FFGlobalClock *getInstance();
    int64_t getAudioClock();
    int64_t getVideoClock();
    void setAudioClock(int64_t clock);
    void setVideoClock(int64_t clock);
    void initClock();
    void setReadyFlag();
    bool getReadyFlag();

    void setClock(int64_t clock);
    int64_t getClock();
private:
    std::atomic<int64_t> audioClock;
    std::atomic<int64_t> videoClock;
    std::atomic<bool> readyFlag;
    std::mutex mutex;
    int64_t clock = 0;
};

#endif // FFGLOBALCLOCK_H
