#include "ffopensourceevent.h"
#include "demuxer/ffdemuxer.h"
#include "decoder/ffadecoder.h"
#include "decoder/ffvdecoder.h"
using namespace FFCaptureContextType;

FFOpenSourceEvent::FFOpenSourceEvent(FFCaptureContext *captureCtx, FFCaptureContextType::demuxerType sourceType_,
                                     std::string const& url_,std::string const& format_)
    :FFEvent (captureCtx)
{

    sourceType = sourceType_;
    url = url_;
    format = format_;
    index = demuxerIndex[sourceType];
}

void FFOpenSourceEvent::work()
{
    init();
    start();

    std::cout << "source type :" << sourceType <<std::endl;
}

void FFOpenSourceEvent::init()
{

    if(sourceType == AUDIO || sourceType == MICROPHONE){
        //音频源处理
        aDemuxerThread[index]->close();
        aDemuxer[index]->init(url,format,aPktQueue[index],nullptr,sourceType);
        aDemuxerThread[index]->init(aDemuxer[index]);

        aDecoderThread[index]->close();
        aDecoder[index]->init(aDemuxer[index]->getAStream(),aFrmQueue[index]);
        aDecoderThread[index]->init(aDecoder[index],aPktQueue[index]);
    }
    else{
        //视频源处理
        vDemuxerThread[index]->close();//关闭下游线程
        vDemuxer[index]->init(url,format,nullptr,vPktQueue[index],sourceType);
        vDemuxerThread[index]->init(vDemuxer[index]);

        vDecoderThread[index]->close();
        vDecoder[index]->init(vDemuxer[index]->getVStream(),vFrmQueue[index]);
        vDecoderThread[index]->init(vDecoder[index],vPktQueue[index]);
    }

    std::cout << "init opensource" << std::endl;
}

void FFOpenSourceEvent::start()
{

    if(sourceType == AUDIO || sourceType == MICROPHONE){
        aDemuxerThread[index]->wakeAllThread();
        aDemuxerThread[index]->start();

        aDecoderThread[index]->wakeAllThread();
        aDecoderThread[index]->start();

        aPktQueue[index]->start();
        aFrmQueue[index]->start();

        aFilterThread->openAudioSource(sourceType);
    }
    else{
        vDemuxerThread[index]->wakeAllThread();
        vDemuxerThread[index]->start();

        vDecoderThread[index]->wakeAllThread();
        vDecoderThread[index]->start();

        vPktQueue[index]->start();
        vFrmQueue[index]->start();

        vFilterThread->openVideoSource(sourceType);
    }

    std::cout << "start opensource "<< std::endl;
}
