#ifndef FFAPACKETQUEUE_H
#define FFAPACKETQUEUE_H
#include <condition_variable>
#include <mutex>
#include <queue>
#include <iostream>
#include <atomic>
extern "C"{
    #include <libavformat/avformat.h>
}

class FFPacket;


class FFAPacketQueue
{
public:
    explicit FFAPacketQueue();
    ~FFAPacketQueue();
    FFPacket* dequeue();
    FFPacket* peekQueue();

    void enqueue(AVPacket *pkt);
    void enqueueFlush();
    void enqueueNull();
    void flushQueue();

    size_t getSerial();
    void clearQueue();
    void wakeAllThread();
    void close();
    void start();
private:
    std::atomic<size_t> serial;//序列号
    std::condition_variable cond;
    std::mutex mutex;
    std::queue<FFPacket*> pktQueue;
    std::atomic<bool> m_stop;//用于当需要停止时，唤醒所有的相关线程，防止死锁
};

#endif // FFAPACKETQUEUE_H
