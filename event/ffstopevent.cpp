#include "ffstopevent.h"

FFStopEvent::FFStopEvent(FFCaptureContext* captureCtx)
    :FFEvent (captureCtx)
{

}

// void FFStopEvent::work()
// {
//     close();
//     clearQueue();
// }
void FFStopEvent::work()
{

        close();
        clearQueue();

}
void FFStopEvent::close()
{
    std::cout<<"3333333333333"<<std::endl;
    if(muxerThread){
        //关闭复用线程
        muxerThread->stop();
        muxerThread->wakeAllThread();
        muxerThread->wait();
        muxerThread->close();
    }
    std::cout<<"6666666666666666666"<<std::endl;
    //关闭编码线程
    std::cout << "Before stop: vEncoderThread = " << vEncoderThread << std::endl;
    if(vEncoderThread){
        std::cout << "vEncoderThread111111111"<< std::endl;
        vEncoderThread->stop();
        std::cout << "vEncoderThread222222222"<< std::endl;
        vEncoderThread->wakeAllThread();
        std::cout << "vEncoderThread333333333"<< std::endl;
        vEncoderThread->wait();
        std::cout << "vEncoderThread444444444"<< std::endl;
        vEncoderThread->close();
    }
    std::cout<<"44444444444444444"<<std::endl;
    if(aEncoderThread){
        aEncoderThread->stop();
        aEncoderThread->wakeAllThread();
        aEncoderThread->wait();
        aEncoderThread->close();
    }
    std::cout<<"55555555555555555555555"<<std::endl;
    std::cout<<"111111111111"<<std::endl;
    //设置编码标志
    // if(vFilterThread){
    //     vFilterThread->stop();          // 假设有统一的 stop() 接口
    //     vFilterThread->wakeAllThread();
    //     vFilterThread->wait();
    //     vFilterThread->stopEncoder();
    // }
    vFilterThread->stopEncoder();
    std::cout<<"2222222222222"<<std::endl;
    // if(aFilterThread){
    //     aFilterThread->stop();          // 假设有统一的 stop() 接口
    //     aFilterThread->wakeAllThread();
    //     aFilterThread->wait();
    //     aFilterThread->stopEncoder();
    // }
    aFilterThread->stopEncoder();

}
void FFStopEvent::clearQueue()
{

    // 关闭滤镜输出队列
    std::cout<<"777777777777777"<<std::endl;
    if (aFilterEncoderFrmQueue) aFilterEncoderFrmQueue->close();
    if (vFilterEncoderFrmQueue) vFilterEncoderFrmQueue->close();

    // 关闭编码器输出包队列
    std::cout<<"88888888888888888"<<std::endl;
    if (aEncoderPktQueue) aEncoderPktQueue->close();
    if (vEncoderPktQueue) vEncoderPktQueue->close();
    std::cout<<"99999999999999999"<<std::endl;
    // 关闭渲染帧队列
    if (vRenderFrmQueue) vRenderFrmQueue->close();
}
