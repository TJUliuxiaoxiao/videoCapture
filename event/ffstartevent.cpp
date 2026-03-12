#include "ffstartevent.h"
#include "muxer/ffmuxer.h"
#include "encoder/ffaencoder.h"
#include "encoder/ffvencoder.h"
FFStartEvent::FFStartEvent(FFCaptureContext*captureCtx,std::string const&url_,std::string const&format_)
    :FFEvent(captureCtx){
    url = url_;
    format = format_;
}

void FFStartEvent::work()
{
    initEncoder();
    startQueue();
    startEncoder();
}

void FFStartEvent::initEncoder()
{
    //初始化复用线程
    muxer->init(url,format);
    muxerThread->init(aEncoderPktQueue,vEncoderPktQueue,muxer,aEncoder,vEncoder,captureContext);
    //初始化编码线程
    aEncoder->init(aEncoderPktQueue);
    aEncoderThread->init(aFilter,aEncoder,muxer,aFilterEncoderFrmQueue);

    vEncoder->init(vEncoderPktQueue);
    vEncoderThread->init(vFilter,vEncoder,muxer,vFilterEncoderFrmQueue);
}

void FFStartEvent::startQueue()
{
    //开启队列
    aEncoderPktQueue->start();
    vEncoderPktQueue->start();

    aFilterEncoderFrmQueue->start();
    vFilterEncoderFrmQueue->start();
}

void FFStartEvent::startEncoder()
{
    std::cout<<"startEncoder"<<std::endl;
    //开启编码标志
    aFilterThread->startEncoder();
    vFilterThread->startEncoder();

    //开启编码线程
    aEncoderThread->start();
    std::cout<<"aEncoderThread start"<<std::endl;

    vEncoderThread->start();
    std::cout<<"vEncoderThread start"<<std::endl;

    //开启复用线程
    muxerThread->start();
    std::cout<<"muxerThread start"<<std::endl;
}
