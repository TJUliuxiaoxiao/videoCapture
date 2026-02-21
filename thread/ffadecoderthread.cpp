#include "ffadecoderthread.h"
#include "queue/ffapacketqueue.h"
#include "decoder/ffadecoder.h"
#include "queue/ffpacket.h"
#include "player/ffplayercontext.h"
#include "event/ffevent.h"
#include "queue/ffeventqueue.h"

FFADecoderThread::FFADecoderThread() {
    stopFlag = true;
}

FFADecoderThread::~FFADecoderThread()
{
    if(aPktQueue){
        delete aPktQueue;
        aPktQueue = nullptr;
    }
    if(aDecoder){
        delete aDecoder;
        aDecoder = nullptr;
    }
    if(playerCtx){
        delete playerCtx;
        playerCtx = nullptr;
    }
}

void FFADecoderThread::init(FFADecoder *aDecoder_,
                            FFAPacketQueue* aPktQueue_){
    aDecoder = aDecoder_;
    aPktQueue = aPktQueue_;
    playerCtx = new FFPlayerContext();
    playerCtx->aDecoderThread = this;
    playerCtx->aPktQueue = aPktQueue_;
}

void FFADecoderThread::wakeAllThread()
{
    if(aPktQueue){
        aPktQueue->wakeAllThread();
    }
    if(aDecoder){
        aDecoder->wakeAllThread();
    }
}

void FFADecoderThread::close()
{
    if(aDecoder){
        aDecoder->close();
    }
    stopFlag.store(true,std::memory_order_release);
}

bool FFADecoderThread::peekStop()
{
    return stopFlag.load(std::memory_order_acquire);
}

void FFADecoderThread::run()
{
    while(!m_stop){
        stopFlag.store(false,std::memory_order_release);
        FFPacket* pkt = aPktQueue->dequeue();
        if(pkt == nullptr){
            continue;
        }
        if(pkt->serial!=aPktQueue->getSerial()){
            aPktQueue->flushQueue();//清空pktQueue队列
            aDecoder->flushQueue();//清空frmQueue队列
            aDecoder->flushDecoder();//清空codeCtx缓冲区

            std::cerr<<"flush aDecoder"<<std::endl;
        }
        else{
            if(pkt->type == NULLP&& pkt->packet.data==nullptr){
                //读取完毕，冲刷解码器,pkt为空
                aDecoder->decode(nullptr);
                std::cerr<<"null apacket !"<<std::endl;
                aDecoder->enqueueNull();
            }else{
                aDecoder->decode(&pkt->packet);
            }
            av_packet_unref(&pkt->packet);//
            av_freep(&pkt);
        }
    }
}
