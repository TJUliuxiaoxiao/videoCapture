#include "ffthread.h"

FFThread::FFThread():m_stop(true){

}


FFThread::~FFThread()
{
    if(m_thread.joinable()){
        m_thread.join();
    }
}


void FFThread::stop()
{
    m_stop.store(true,std::memory_order_relaxed);//只是更改了m_stop变量,线程可能还在运行
}


void FFThread::wait()
{
    bool flag = m_thread.joinable();
    if(flag){
        m_thread.join();
        std::cerr<<"thread id"
                  <<std::this_thread::get_id()<<"join"<<std::endl;
    }
}


void FFThread::start()
{
    if(m_thread.joinable()){
        //选择抛出异常
        throw std::runtime_error("该线程可连接了");
    }
    m_stop.store(false,std::memory_order_relaxed);
    //确保启动新线程之前,m_thread是不可连接的，即没有其他线程正在运行,否则会挤占资源
    m_thread = std::thread(&FFThread::run,this);//此时在启动线程后m_thread.joinable()设置为true
}
