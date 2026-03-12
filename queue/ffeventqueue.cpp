#include "ffeventqueue.h"
#include "event/ffevent.h"
FFEventQueue::~FFEventQueue(){
    std::lock_guard<std::mutex> lock(mutex);//加锁保护，防止并发修改
    m_stop.store(true);//通知所有正在deque中等待的线程可以退出
    cond.notify_all();//唤醒所有等待的线程
    while(!evQueue.empty()){
        FFEvent* event = evQueue.front();
        evQueue.pop();
        delete event;
    }//删除队列中的所有event
}

FFEventQueue &FFEventQueue::getInstance(){
    static FFEventQueue instance;
    return instance;
}//返回全局唯一下的FFEventQueue对象的引用

void FFEventQueue::enqueue(FFEvent* event){
    std::lock_guard<std::mutex> lock(mutex);
    evQueue.emplace(event);
    //使用emplace将事件添加到队列
    cond.notify_one();
}//将事件指针放入队列尾部

FFEvent *FFEventQueue::dequeue()
{
    std::unique_lock<std::mutex> lock(mutex);
    //使用unique_lock加锁
    cond.wait(lock,[this](){
        return !evQueue.empty()||m_stop.load();
    });//当队列为空并且没有停止时，线程进入等待状态并释放锁
    //当被唤醒且条件满足时,重新获得锁
    if(m_stop.load()){
        return nullptr;
    }//如果是因为停止标志被唤醒,则返回nullptr,表示队列已经关闭，消费者可以退出
    FFEvent* event = evQueue.front();
    evQueue.pop();
    return event;//从队列头部取出事件,弹出并返回指针
}

void FFEventQueue::clearQueue()
{
    std::lock_guard<std::mutex> lock(mutex);
    while(!evQueue.empty()){
        FFEvent* event = evQueue.front();
        evQueue.pop();
        delete event;
    }//立即清空队列并删除所有对象
    //例如重置队列或准备重新开始时,可以主动清理尚未处理的事件
}

void FFEventQueue::wakeAllThread()
{
    std::lock_guard<std::mutex> lock(mutex);
    m_stop.store(true);
    cond.notify_all();
    //设置停止标志并通知所有可能阻塞在dequeue中的线程
    //在程序退出时调用,让消费者线程及时退出循环,避免死锁或无法终止
}




