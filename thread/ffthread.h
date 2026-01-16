#ifndef FFTHREAD_H
#define FFTHREAD_H

#include <QObject>
#include <thread>
#include <memory>
#include <iostream>
#include <atomic>


class FFThread
{
    // Q_OBJECT
public:
   explicit FFThread();
   virtual void run() = 0;
   virtual ~FFThread();
   void stop();
   void wait();
   void start();
protected:
   std::atomic<bool>m_stop;//原子操作，用于多线程环境下安全地操作bool值
private:
   std::thread m_thread;
};
#endif // FFTHREAD_H
