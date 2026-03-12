#include "ffvencoderthread.h"
#include "encoder/ffvencoder.h"
#include "queue/ffvframequeue.h"
#include "filter/ffvfilter.h"
#include "muxer/ffmuxer.h"
FFVEncoderThread::FFVEncoderThread() {

}

FFVEncoderThread::~FFVEncoderThread()
{

}

void FFVEncoderThread::init(FFVFilter *vFilter_, FFVEncoder *vEncoder_, FFMuxer *muxer_, FFVFrameQueue *frmQueue_)
{
    vFilter = vFilter_;
    vEncoder = vEncoder_;
    muxer = muxer_;
    frmQueue = frmQueue_;
}

void FFVEncoderThread::close()
{
    if(vEncoder){
        vEncoder->close();
    }
    streamIndex = -1;
    isFirstFrame = true;//表示从第一帧开始传输
    firstFramePts = 0;
}

void FFVEncoderThread::wakeAllThread()
{
    if(frmQueue){
        std::cout<<"In FFVEncoderThread::wakeAllThread frmQueue"<<std::endl;
        frmQueue->wakeAllThread();
    }
    if(vEncoder){
        std::cout<<"In FFVEncoderThread::wakeAllThread vEncoder"<<std::endl;
        vEncoder->wakeAllThread();
    }
}

void FFVEncoderThread::run()
{   while(!m_stop){
        AVFrame* frame = frmQueue->dequeue();
        if(frame==nullptr){
            m_stop = true;
            break;
        }
        if(streamIndex == -1){
            initEncoder(frame);
        }
        if(isFirstFrame){
            firstFramePts = frame->pts;
            isFirstFrame = false;
            vEncoder->encode(frame,streamIndex,0,videoTimeBase);
        }else{
            int64_t relativePts = frame->pts - firstFramePts;
            vEncoder->encode(frame,streamIndex,relativePts,videoTimeBase);
        }
        av_frame_unref(frame);
        av_frame_free(&frame);
    }
}

void FFVEncoderThread::initEncoder(AVFrame *frame)
{
    videoTimeBase = vFilter->getTimeBase();
    frameRate = vFilter->getFrameRate();
    vEncoder->initVideo(frame,frameRate);
    muxer->addStream(vEncoder->getCodecCtx());
    streamIndex = muxer->getVStreamIndex();
}


