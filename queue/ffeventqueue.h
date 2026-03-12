#ifndef FFEVENTQUEUE_H
#define FFEVENTQUEUE_H

#include <iostream>
#include<queue>
#include <atomic>
#include <condition_variable>
#include <mutex>
class FFEvent;
class FFEventQueue final//final关键字表示虚函数不能被重写或者类继承
{
public:
    static FFEventQueue& getInstance();

    FFEventQueue(const FFEventQueue&) = delete;//禁用拷贝构造函数
    FFEventQueue& operator=(const FFEventQueue&) = delete;

    void enqueue(FFEvent* event);
    FFEvent* dequeue();
    void clearQueue();
    void wakeAllThread();
    ~FFEventQueue();

private:
    FFEventQueue():m_stop(false){}
    std::queue<FFEvent*> evQueue;
    std::condition_variable cond;
    std::mutex mutex;
    std::atomic<bool> m_stop;
};

#endif // FFEVENTQUEUE_H
