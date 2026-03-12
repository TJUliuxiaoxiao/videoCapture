#include "ffvpacketqueue.h"
#include "queue/ffpacket.h"

#define MAX_PACKET_SIZE 2

FFVPacketQueue::FFVPacketQueue():serial(0),m_stop(false){}

FFVPacketQueue::~FFVPacketQueue()
{
    close();
}

FFPacket *FFVPacketQueue::dequeue()
{
    std::unique_lock<std::mutex> lock(mutex);
    cond.wait(lock,[this](){
        return (!pktQueue.empty())||m_stop.load()==true;//向下执行的条件
    });//wait阻塞直到满足某个条件
    FFPacket* ffpkt = pktQueue.front();
    pktQueue.pop();
    //然后呢?
    cond.notify_one();
    return ffpkt;
}

FFPacket *FFVPacketQueue::peekQueue()
{
    std::lock_guard<std::mutex> lock(mutex);
    return pktQueue.empty()?nullptr:pktQueue.front();
}

void FFVPacketQueue::enqueue(AVPacket *pkt)
{
    std::unique_lock<std::mutex> lock(mutex);
    cond.wait(lock,[this](){
        return pktQueue.size()<MAX_PACKET_SIZE||m_stop.load();
    });
    if(m_stop.load()){
        std::cout<<"stop load!"<<std::endl;
        av_packet_unref(pkt);
        m_stop.store(false);
        return;
    }
    FFPacket* ffpkt = static_cast<FFPacket*>(av_mallocz(sizeof(FFPacket)));
    av_init_packet(&ffpkt->packet);
    // ffpkt->packet = av_packet_alloc();
    av_packet_move_ref(&ffpkt->packet,pkt);
    ffpkt->serial = serial;
    ffpkt->type = NORMAL;
    pktQueue.push(ffpkt);
    cond.notify_one();
}

void FFVPacketQueue::enqueueFlush()
{
    std::unique_lock<std::mutex> lock(mutex);
    cond.wait(lock,[this](){
        return pktQueue.size()<MAX_PACKET_SIZE||m_stop.load();
    });
    if(m_stop.load()){
        // m_stop.store(false);
        return;
    }
    FFPacket* ffpkt = static_cast<FFPacket*>(av_mallocz(sizeof(FFPacket)));
    av_init_packet(&ffpkt->packet);
    ffpkt->serial = this->serial++;
    ffpkt->type = FLUSH;
    pktQueue.push(ffpkt);
    cond.notify_one();
}

void FFVPacketQueue::enqueueNull()
{
    std::unique_lock<std::mutex> lock(mutex);
    cond.wait(lock,[this](){
        return pktQueue.size()<MAX_PACKET_SIZE||m_stop.load();
    });
    if(m_stop){
        // m_stop.store(false);
        return;
    }
    FFPacket* ffpkt = static_cast<FFPacket*>(av_mallocz(sizeof(FFPacket)));
    av_init_packet(&ffpkt->packet);
    ffpkt->serial = this->serial;
    ffpkt->type = NULLP;
    pktQueue.push(ffpkt);
    cond.notify_one();
}

void FFVPacketQueue::flushQueue()
{//刷新队列
    std::lock_guard<std::mutex> lock(mutex);
    std::cout << "[flushQueue] before flush, size=" << pktQueue.size() << std::endl;
    while(true){
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
    std::cout << "[flushQueue] after flush, size=" << pktQueue.size() << std::endl;
    cond.notify_one();
}

size_t FFVPacketQueue::getSerial()
{
    return serial.load();
}

void FFVPacketQueue::wakeAllThread()
{
    m_stop = true;
    cond.notify_all();
}

void FFVPacketQueue::clearQueue()
{
    std::lock_guard<std::mutex> lock(mutex);
    while(!pktQueue.empty()){
        FFPacket* pkt = pktQueue.front();
        pktQueue.pop();
        if(pkt!=nullptr){
            av_packet_unref(&pkt->packet);
            av_freep(&pkt);
        }
    }
}

void FFVPacketQueue::close()
{
    wakeAllThread();
    clearQueue();
}

void FFVPacketQueue::start()
{
    m_stop = false;
}

void FFVPacketQueue::setMaxSize(size_t maxSize_)
{
    maxSize = maxSize_;
}
size_t FFVPacketQueue::size() {
    std::lock_guard<std::mutex> lock(mutex);
    return pktQueue.size();
}
