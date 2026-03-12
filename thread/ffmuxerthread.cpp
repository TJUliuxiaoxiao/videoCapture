#include "ffmuxerthread.h"

#define CAPTURE_TIME 60
#include "muxer/ffmuxer.h"
#include "queue/ffvpacketqueue.h"
#include "queue/ffapacketqueue.h"
#include "queue/ffpacket.h"
#include "encoder/ffaencoder.h"
#include "encoder/ffvencoder.h"
#include "event/ffcaptureprocessevent.h"
#include "queue/ffeventqueue.h"
FFMuxerThread::FFMuxerThread() {
    vTimeBase = {-1,-1};
    aTimeBase = {-1,-1};
}//构造函数初始化音视频时间基为{-1,-1},
//表示尚未获取到有效的事件基

FFMuxerThread::~FFMuxerThread()
{

}

void FFMuxerThread::init(FFAPacketQueue *aPktQueue_,
                         FFVPacketQueue *vPktQueue_,
                         FFMuxer *muxer_,
                         FFAEncoder *aEncoder_,
                         FFVEncoder *vEncoder_,
                         FFCaptureContext *captureCtx_)
{
    aPktQueue = aPktQueue_;
    vPktQueue = vPktQueue_;

    aEncoder = aEncoder_;
    vEncoder = vEncoder_;

    muxer = muxer_;
    captureCtx = captureCtx_;
}//保存外部传入的队列、编码器、复用器和捕获上下文指针
//这些依赖对象由外部创建和管理、线程仅使用他们不负责销毁

void FFMuxerThread::close()
{
    if(muxer){
        muxer->close();
    }

    aTimeBase = {-1,-1};
    vTimeBase = {-1,-1};

    aEncoder = nullptr;
    vEncoder = nullptr;

    lastProcessTime = 0;
}//关闭

void FFMuxerThread::wakeAllThread()
{
    if(vPktQueue){
        vPktQueue->wakeAllThread();
    }
    if(aPktQueue){
        aPktQueue->wakeAllThread();
    }
}//唤醒音频和视频队列中可能阻塞的等待线程

void FFMuxerThread::run()//实现音视频包的同步复用
{
    bool audioFinish = true;
    bool videoFinish = true;

    //audioFinish和videoFinish标志指示当前是否已经准备好一个音频包或者视频包
    muxer->writeHeader();//首先写入文件头
    FFPacket *audioPkt = nullptr;
    FFPacket *videoPkt = nullptr;
    AVPacket *aPacket = nullptr;
    AVPacket *vPacket = nullptr;

    double audioPtsSec = 0;
    double videoPtsSec = 0;

    int ret = 0;
    while(!m_stop){
        if(audioFinish){
            //从音频队列取一个包
            std::lock_guard<std::mutex> lock(mutex);
            audioPkt = aPktQueue->dequeue();//音频包取出队列
            if(audioPkt ==nullptr){
                m_stop = true;//线程停止
                std::cerr<<"audioPkt is nullptr";
                continue;
            }
            aPacket = &(audioPkt->packet);//得到AVPacket
            if(aTimeBase.den == -1 &&aTimeBase.num == -1){
                aTimeBase = aEncoder->getCodecCtx()->time_base;
            }//音频时间基

            //转换为秒
            // if(aEncoder==nullptr){
            //     break;
            // }
            audioPtsSec = aPacket->pts*av_q2d(captureCtx->aEncoder->getCodecCtx()->time_base)*1e3;
            // audioPtsSec = aPacket->pts*av_q2d(aEncoder->getCodecCtx()->time_base);
            if(audioPtsSec < 0){//PTS无效,释放包
                audioFinish = true;
                av_packet_unref(aPacket);
                av_packet_free(&aPacket);
                continue;
            }
        }
        //从视频队列中取包
        if(videoFinish){
            std::lock_guard<std::mutex> lock(mutex);
            videoPkt = vPktQueue->dequeue();
            if(videoPkt == nullptr){
                std::cerr<<"videoPkt is nullptr"<<std::endl;
                m_stop = true;
                continue;
            }
            vPacket = &(videoPkt->packet);
            if(vTimeBase.den==-1&&vTimeBase.num==-1){
                vTimeBase = vEncoder->getCodecCtx()->time_base;
            }
            // if(vEncoder==nullptr){
            //     break;
            // }
            videoPtsSec = vPacket->pts*av_q2d(vEncoder->getCodecCtx()->time_base);
            if(videoPtsSec <0){
                videoFinish = true;
                av_packet_unref(vPacket);
                av_packet_free(&vPacket);
                continue;
            }
        }
        /*比较当前音频包和视频包的PTS值,选择时间戳较小的包先复用,以保证音视频同步
         谁先发生先复用*/
        if(audioPtsSec < videoPtsSec){
            ret = muxer->mux(aPacket);
            if(ret < 0){
                std::cerr<<"Mux Audio Fail!"<<std::endl;
                m_stop = true;
                return;
            }
            audioFinish = true;
            videoFinish = false;
            sendCaptureProcessEvent(audioPtsSec);
        }
        else{
            ret = muxer->mux(vPacket);
            if(ret < 0){
                std::cerr<<"Mux Video Fail!"<<std::endl;
                m_stop = true;
                return;
            }
            videoFinish = true;
            audioFinish = false;
            sendCaptureProcessEvent(videoPtsSec);
        }
    }
    muxer->writeTrailer();//写入文件尾
}

void FFMuxerThread::sendCaptureProcessEvent(double seconds)
{
    /*sendCaptureProcessEvent是FFMuxerThread类中的一个私有成员函数,用于定期向全局事件
     队列发送录制进度事件,主要目的是让外部模块能够获取当前音视频录制的进度,从而更新进度条或显示
    录制进度条*/
    if(seconds - lastProcessTime > 1.0){
    //seconds表示当前已经处理到的音视频时间戳,通常来自于音频或视频包的PTS转换成的秒数
    //成员变量lastProcessTime记录上一次发送进度事件的时间。用于控制发送频率,避免频繁地发送事件
    //这里设定为至少间隔一秒才发送一次
        FFEvent* captureProcessEv =
            new FFCaptureProcessEvent(captureCtx,static_cast<int64_t>(seconds));
        FFEventQueue::getInstance().enqueue(captureProcessEv);
        //获取全局事件队列的单例,并将事件对象加入队列,由其他线程处理
        lastProcessTime = seconds;
    }
}
/*在音视频录制过程中，外部界面通常需要实时
 * 显示当前的录制时长。如果每次复用包都直接
 * 发送事件，事件数量会非常密集（例如视频
 * 30fps 时每秒 30 个事件），造成不必要的
 * 性能损耗。通过限制为每秒一次，既满足了界
 * 面更新需求，又避免了事件泛滥。*/


