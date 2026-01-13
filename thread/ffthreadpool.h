#ifndef FFTHREADPOOL_H
#define FFTHREADPOOL_H
#include <queue>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
class FFThreadPool
{
public:
    explicit FFThreadPool();
    ~FFThreadPool();
    void init(size_t threadCount_);
    void stop();
    void wait();

    template<typename Func>
    void submit(Func func){
        std::function<void()> task = func;
        enqueueTask(task);//将任务放入队列
    }

    template<typename Class,typename Func>
    void submit(Class *obj,Func memberFunc){
        std::function<void()> task = [obj,memberFunc](){
            (obj->*memberFunc)();
            delete obj;
        };
        enqueueTask(task);
    }

    template<typename Class,typename Func,typename...Args>
    void submit(Class*obj,Func memberFunc,Args&&... args){
        std::function<void()> task = [obj,memberFunc,args...]()mutable{
            (obj->*memberFunc)(std::forward<Args>(args)...);
            delete obj;
        };
        enqueueTask(task);
    }

private:
    void work();
    std::function<void()> getTask();
    void enqueueTask(std::function<void()>task);
private:
    std::vector<std::thread> threadVec;
    std::queue<std::function<void()>> taskQueue;
    std::mutex mutex;
    std::condition_variable cond;
    bool m_stop;
    size_t threadCount;
};

#endif // FFTHREADPOOL_H
