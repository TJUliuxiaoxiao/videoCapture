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
        return !pktQueue.empty() || m_stop.load();// 等待条件：队列不为空或者停止
    });//当队列为空时
    FFPacket* ffpkt = pktQueue.front();
    pktQueue.pop();
    cond.notify_one();
    std::cout<<"dequeue apacket!"<<std::endl;
    return ffpkt;
}

FFPacket *FFAPacketQueue::peekQueue()
{
    std::lock_guard<std::mutex> lock(mutex);
    return pktQueue.empty()?nullptr:pktQueue.front();
}

void FFAPacketQueue::enqueue(AVPacket *pkt)
{
    std::unique_lock<std::mutex> lock(mutex);
    cond.wait(lock,[this](){
        return pktQueue.size()<MAX_PACKET_SIZE || m_stop.load();
    });
    if(m_stop.load()){
        std::cerr<<"m_stop load!"<<std::endl;
        av_packet_unref(pkt);
        return;
    }
    FFPacket* ffpkt = static_cast<FFPacket*>(av_mallocz(sizeof(FFPacket)));
    av_init_packet(&ffpkt->packet);
    av_packet_move_ref(&ffpkt->packet,pkt);
    ffpkt->serial = serial;//序列号
    ffpkt->type = NORMAL;
    pktQueue.push(ffpkt);
    std::cout<<"enqueue apcket:"<<pktQueue.size()<<std::endl;
    cond.notify_one();
}





void FFAPacketQueue::clearQueue()
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



