#include "ffaencoderthread.h"
#include "encoder/ffaencoder.h"
#include "filter/ffafilter.h"
#include "muxer/ffmuxer.h"
#include "queue/ffaframequeue.h"
FFAEncoderThread::FFAEncoderThread() {}

FFAEncoderThread::~FFAEncoderThread()
{

}

void FFAEncoderThread::init(FFAFilter *aFilter_, FFAEncoder *aEncoder_, FFMuxer *muxer_, FFAFrameQueue *frmQueue_)
{
    aFilter = aFilter_;
    aEncoder = aEncoder_;
    muxer = muxer_;
    frmQueue  = frmQueue_;
}

void FFAEncoderThread::close()
{
    if(aEncoder){
        aEncoder->close();
    }
    firstFrame = true;//标记是否为第一帧
    firstFramePts = 0;//用于存储第一帧的绝对PTS
    streamIndex = -1;//流编号
}

void FFAEncoderThread::wakeAllThread()
{
    if(frmQueue){
        frmQueue->wakeAllThread();
    }
    if(aEncoder){
        aEncoder->wakeAllThread();
    }
}

void FFAEncoderThread::run()
{

    while(!m_stop){
        std::lock_guard<std::mutex> lock(mutex);
        AVFrame* frame = frmQueue->dequeue();//取帧
        if(frame == nullptr){
            m_stop = true;
            break;//帧空，中断
        }
        if(streamIndex == -1){
            initEncoder(frame);
        }//流编号
        if(firstFrame){//第一帧
            firstFramePts = frame->pts;
            //firstFramPts = 168406498292,非常大的一个数
            firstFrame = false;
            //第一帧的相对PTS为0
            aEncoder->encode(frame,streamIndex,0,audioTimeBase);

        }else{
            //frame->pts = 168302857860
            //audioTimeBase = 1/48000
            int64_t RelativePts = frame->pts - firstFramePts;
            if(RelativePts<0){
                std::cerr<<"RelativePts<0!"<<std::endl;
            }
            aEncoder->encode(frame,streamIndex,RelativePts,audioTimeBase);
            //相对时间戳
        }
        av_frame_unref(frame);
        av_frame_free(&frame);
    }
}

void FFAEncoderThread::initEncoder(AVFrame *frame)
{
    audioTimeBase = aFilter->getTimeBase();
    aEncoder->initAudio(frame);
    //
    muxer->addStream(aEncoder->getCodecCtx());
    streamIndex = muxer->getAStreamIndex();
}

