#include "ffapacketqueue.h"
#include "ffpacket.h"
#define MAX_PACKET_SIZE 3
FFAPacketQueue::FFAPacketQueue():serial(0),m_stop(false) {

}

FFAPacketQueue::~FFAPacketQueue(){
    close();
}

FFPacket *FFAPacketQueue::dequeue()
{
    std::unique_lock<std::mutex> lock(mutex);//unique_lock允许作用域内多次加锁和解锁
    cond.wait(lock,[this](){
        return !pktQueue.empty() || m_stop.load();//队列不为空或者停止
    });//当队列为空时
    FFPacket* ffpkt = pktQueue.front();
    pktQueue.pop();
    cond.notify_one();
    std::cout<<"dequeue apacket!"<<std::endl;
    return ffpkt;
}

FFPacket *FFAPacketQueue::peekQueue()//查询第一个包的值，但是不取出包
{
    std::lock_guard<std::mutex> lock(mutex);
    return pktQueue.empty()?nullptr:pktQueue.front();
}

void FFAPacketQueue::enqueue(AVPacket *pkt)
{
    std::unique_lock<std::mutex> lock(mutex);
    cond.wait(lock,[this](){
        return pktQueue.size()<MAX_PACKET_SIZE || m_stop.load();
    });//当队列满且m_stop为false时，不被阻塞
    if(m_stop.load()){//如果是主线程发送stop信号时
        std::cerr<<"m_stop load!"<<std::endl;
        av_packet_unref(pkt);//减少AVPacket的引用计数，释放资源
        return;
    }
    FFPacket* ffpkt = static_cast<FFPacket*> (av_mallocz(sizeof(FFPacket)));
    av_init_packet(&ffpkt->packet);
    av_packet_move_ref(&ffpkt->packet,pkt);
    /*分配内存：av_mallocz()分配并清零FFPacket结构体
    初始化：av_init_packet()初始化AVPacket结构
    转移引用：av_packet_move_ref()将pkt的数据转移到ffpkt，避免拷贝
    转移后，传入的pkt变为空，ffpkt持有数据所有权*/
    ffpkt->serial = serial;//序列号
    ffpkt->type = NORMAL;
    pktQueue.push(ffpkt);
    cond.notify_one();

}

void FFAPacketQueue::enqueueFlush()//插入一个特殊的刷新标记包
{
    std::unique_lock<std::mutex> lock(mutex);
    cond.wait(lock,[this](){
        return pktQueue.size()<MAX_PACKET_SIZE||m_stop.load();
    });
    if(m_stop.load()){
        return;
    }
    FFPacket* ffpkt = static_cast<FFPacket*> (av_mallocz(sizeof(FFPacket)));
    av_init_packet(&ffpkt->packet);
    ffpkt->serial = this->serial++;//刷新
    ffpkt->type = FLUSH;
    pktQueue.push(ffpkt);
    cond.notify_one();//唤醒消费者进程
}

void FFAPacketQueue::enqueueNull()
{
    std::unique_lock<std::mutex> lock(mutex);
    cond.wait(lock,[this](){
        return pktQueue.size()<MAX_PACKET_SIZE || m_stop.load();
    });//相当于wait until()...
    if(m_stop){
        return;
    }

    FFPacket* ffpkt = static_cast<FFPacket*> (av_mallocz((sizeof(FFPacket))));
    av_init_packet(&ffpkt->packet);
    ffpkt->serial = serial;
    ffpkt->type = NULLP;
    ffpkt->packet.data = nullptr;
    pktQueue.push(ffpkt);
    cond.notify_one();
}

void FFAPacketQueue::flushQueue()//清除了队列头部连续的一些旧序列号的包
{
    while(1){
        FFPacket* pkt = peekQueue();
        if(pkt == nullptr || pkt->serial == this->serial){
            break;
        }else{
            std::lock_guard<std::mutex> lock(mutex);
            pktQueue.pop();
            av_packet_unref(&pkt->packet);
            av_freep(&pkt);
        }
    }
    cond.notify_one();
}

size_t FFAPacketQueue::getSerial()
{
    return serial.load();
}

void FFAPacketQueue::clearQueue()//清空队列
{
    std::lock_guard<std::mutex> lock(mutex);
    while(!pktQueue.empty()){
        FFPacket* ffpkt = pktQueue.front();
        pktQueue.pop();
        if(ffpkt!=nullptr){//先释放ffpkt的packet的空间，再释放结构体ffpkt的空间
            av_packet_unref(&ffpkt->packet);//取地址packet
            av_freep(&ffpkt);//取指向ffpkt指针的地址
        }
    }
}

void FFAPacketQueue::wakeAllThread()
{
    m_stop = true;
    cond.notify_all();
}

void FFAPacketQueue::close()
{
    wakeAllThread();
    clearQueue();
}

void FFAPacketQueue::start()
{
    m_stop = false;
}



