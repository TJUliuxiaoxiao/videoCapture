#ifndef FFVPACKETQUEUE_H
#define FFVPACKETQUEUE_H

#include <mutex>
#include <atomic>
#include <condition_variable>
#include <queue>
#include <iostream>
extern "C"
{
    #include <libavformat/avformat.h>
}

class FFPacket;

class FFVPacketQueue
{
public:
    explicit FFVPacketQueue();
    ~FFVPacketQueue();
    FFPacket* dequeue();
    FFPacket* peekQueue();
    void enqueue(AVPacket* pkt);
    void enqueueFlush();
    void enqueueNull();
    void flushQueue();

    size_t getSerial();

    void wakeAllThread();
    void clearQueue();
    void close();
    void start();
    void setMaxSize(size_t maxSize_);
    size_t size();

private:
    std::mutex mutex;
    std::condition_variable cond;
    std::atomic<size_t> serial;
    std::queue<FFPacket*> pktQueue;
    std::atomic<bool> m_stop;
    size_t maxSize = 2;

};

#endif // FFVPACKETQUEUE_H
