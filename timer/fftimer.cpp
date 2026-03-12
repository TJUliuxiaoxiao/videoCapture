#include "fftimer.h"
#include "queue/ffvframequeue.h"
#include "ui/ffcapwindow.h"
#include "decoder/ffvdecoder.h"
#include "queue/ffvframequeue.h"
#include "player/ffplayercontext.h"
#define FRAME_RATE 30
FFTimer::FFTimer() :m_stop(false),seekFlag(false),pauseFlag(false),speedFlag(false),speed(1.0f),speedFactor(1.0f){
    readyFlag = true;
}

FFTimer::~FFTimer()
{
    stop();
    close();
}

void FFTimer::init(FFVFrameQueue *frmQueue_, FFVRender *vRender_, FFCapWindow *capWindow_)
{
    frmQueue = frmQueue_;
    vRender = vRender_;
    capWindow = capWindow_;
}

void FFTimer::start()
{
    m_stop = false;
    timerThread = std::thread(&FFTimer::work,this);
}


void FFTimer::wait()
{   // 检查 timerThread 是否处于可 join 状态（即线程正在运行且尚未被 join 或 detach）
    if(timerThread.joinable()){
        // 阻塞当前线程，直到 timerThread 执行完毕
        timerThread.join();//join()表示阻塞调用线程
        // 输出调试信息，表示线程已成功 join
        std::cerr<<"Timer thread has joined!"<<std::endl;
    }
}

void FFTimer::stop()
{
    m_stop = true;
    pauseFlag  = false;
    cond.notify_all();
    pauseCond.notify_all();
}

void FFTimer::pause()
{
    bool flag = pauseFlag.load(std::memory_order_acquire);
    pauseFlag.store(!flag,std::memory_order_release);
    if(flag){
        pauseCond.notify_one();
    }
}

void FFTimer::close()
{
    seekFlag = false;
    pauseFlag = false;
    speedFlag = false;
    speed = 1.0f;
    speedFactor = 1.0f;

}

void FFTimer::wakeAllThread()
{
    pauseCond.notify_all();
    cond.notify_all();
    readyFlag.store(true,std::memory_order_release);
}

void FFTimer::setSpeed(float speed_)
{
    speedFlag.store(true,std::memory_order_release);
    speed = speed_;

}

bool FFTimer::peekReadyFlag()
{
    return readyFlag.load(std::memory_order_acquire);
}

void FFTimer::work()
{
    while(!m_stop){
        playVideo();
        //调用该函数从帧队列中取出一帧，
        //并将其数据通过 capWindow 显示出来。这是实际渲染动作。
        readyFlag.store(false,std::memory_order_release);
        //将原子布尔变量 readyFlag 设置为 false，
        //表示当前帧已渲染，可能正在等待下一帧（或正在休眠）

        //30fps
        double duration = 1e6/FRAME_RATE;//固定帧间隔
        av_usleep(static_cast<uint>(duration));//调用微秒级休眠程序,维持目标帧率
        readyFlag.store(true,std::memory_order_release);
        //休眠结束后,将readyFlag置为true,表示能够处理下一帧渲染
    }
}


void FFTimer::copyYUV(AVFrame *frame)
{
    yBuf = new uint8_t[frame->width*frame->height];
    uBuf = new uint8_t[frame->width*frame->height/4];
    vBuf = new uint8_t[frame->width*frame->height/4];//分配缓冲区
    for(int i = 0;i<frame->height;i++){
        memcpy(yBuf + i*frame->width,frame->data[0]+i*frame->linesize[0],frame->width);
    }//拷贝Y平面,由于frame->linesize[0]可能大于frame->width，无法整体拷贝
    /*yBuf += i*frame->width
     * fram->data[0] +=i*frame->linesize[0]
     */
    /*width (宽度)：这是图像的逻辑尺寸，表示一帧图像中一行有多少个有效的像素点。
     * 比如一个 1920x1080 的视频，width 就是 1920。这是你应该实际使用的数据宽度。
    linesize (行跨度/步长)：这是图像的存储尺寸，表示在内存中，
    存储一行图像数据实际占用的字节数。出于性能考虑，它通常会大于 width 对应的字节数*/
    for(int i = 0;i<frame->height;i++){
        memcpy(uBuf + i*frame->width/2,frame->data[1] + i*frame->linesize[1],frame->width/2);

        memcpy(vBuf + i*frame->width/2,frame->data[2] + i*frame->linesize[2],frame->width/2);
    }
}

void FFTimer::setTimerInterval(std::chrono::milliseconds interval_)
{
    interval = interval_;
}

void FFTimer::setTimerInterval(double interval_)
{
    interval = std::chrono::microseconds(static_cast<int64_t>(interval_));
    //这行代码的作用是将一个以某种单位（可能是秒或毫秒）表示的浮点数时间间隔
    //interval_ 转换为微秒，并存储到类型为 std::chrono::microseconds 的
    //成员变量 interval 中
}

void FFTimer::setTimerInterval(int64_t interval_)
{
    interval = std::chrono::microseconds(interval_);
}

void FFTimer::playVideo()
{
    AVFrame *frame = frmQueue->dequeue();
    if(frame==nullptr||frame->data[0]==nullptr){
        stop();
        return;
    }
    QMetaObject::invokeMethod(capWindow,
                              "sendVideoFrame",
                              Qt::QueuedConnection,
                              Q_ARG(AVFrame*,frame));
}
