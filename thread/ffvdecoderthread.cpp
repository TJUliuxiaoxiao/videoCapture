#include "ffvdecoderthread.h"
#include "player/ffplayercontext.h"
#include "queue/ffvpacketqueue.h"
#include "decoder/ffvdecoder.h"
#include "queue/ffpacket.h"
#include "queue/ffeventqueue.h"
#include "player/ffplayercontext.h"

FFVDecoderThread::FFVDecoderThread() {

    stopFlag = true;
}

FFVDecoderThread::~FFVDecoderThread(){
    if(vPktQueue){
        delete vPktQueue;
        vPktQueue = nullptr;
    }
    if(vDecoder){
        delete vDecoder;
        vDecoder = nullptr;
    }
    if(playerCtx){
        delete playerCtx;
        playerCtx = nullptr;
    }

}

void FFVDecoderThread::init(FFVDecoder *vDecoder_, FFVPacketQueue *vPktQueue_)
{
    vDecoder = vDecoder_;//视频解码器
    vPktQueue = vPktQueue_;//视频packet队列

    playerCtx = new FFPlayerContext();//
    playerCtx->vDecoderThread = this;
    playerCtx->vPktQueue = vPktQueue_;
}

void FFVDecoderThread::wakeAllThread()
{
    if(vPktQueue){
        vPktQueue->wakeAllThread();
    }
    if(vDecoder){
        vDecoder->wakeAllThread();
    }
}

void FFVDecoderThread::close()
{
    if(vDecoder){
        vDecoder->close();
    }
    stopFlag.store(true,std::memory_order_release);
}

bool FFVDecoderThread::peekStop()
{
    return stopFlag.load(std::memory_order_acquire);
}

void FFVDecoderThread::run()
{
    while(!m_stop){
        stopFlag.store(false,std::memory_order_release);
        FFPacket* pkt = vPktQueue->dequeue();
        if(pkt == nullptr){
            continue;
        }
        if(pkt->serial != vPktQueue->getSerial()){//seek
            vPktQueue->flushQueue();//清空vpcket队列
            vDecoder->flushQueue();//清空queue
            vDecoder->flushDecoder();//清空codecCtx的ctx
            std::cerr<<"flush vDecoder"<<std::endl;
        }
        else{
            if(pkt->type == NULLP&&pkt->packet.data==nullptr){
                vDecoder->decode(nullptr);
                std::cout<<"null vpacket"<<std::endl;
                vDecoder->enqueueNull();
            }
        }
    }
}

