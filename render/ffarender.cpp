#include "ffarender.h"
#include "player/ffplayercontext.h"
#include "decoder/ffadecoder.h"
#include "event/ffreadyevent.h"
#include "queue/ffeventqueue.h"
#include "clock/ffglobalclock.h"
// #include "event/ffendevent.h"
#include "event/ffstopevent.h"
#include "event/ffprocessevent.h"
extern "C"{
#include "sonic/sonic.h"
}
FFARender::FFARender():readyFlag(false),
    seekFlag(false),pauseFlag(false),
    speedFlag(false),speed(1)
{

}

FFARender::~FFARender()
{
    stop();
    close();
    if(playerCtx){
        delete playerCtx;
        playerCtx = nullptr;
    }
    if(abufOut){
        av_freep(&abufOut);
        abufOut = nullptr;
    }
}

void FFARender::init(FFAFrameQueue *frmQueue_, FFADecoder *aDecoder_, FFPlayerWindow *playerWindow_)
{
    frmQueue = frmQueue_;
    aDecoder = aDecoder_;

    playerCtx = new FFPlayerContext();
    playerCtx->playerWindow = playerWindow_;
    playerCtx->aRender = this;
    playerCtx->aFrmQueue = frmQueue_;
}

void FFARender::stop()
{
    m_stop = true;
    pauseFlag = false;
    pauseCond.notify_all();
}

void FFARender::pause()
{
    bool flag = pauseFlag.load(std::memory_order_acquire);
    pauseFlag.store(!flag,std::memory_order_release);
    if(flag){
        pauseCond.notify_one();
    }
}

bool FFARender::peekReady()
{
    return readyFlag.load(std::memory_order_acquire);
}

void FFARender::wakeAllThread()
{
    pauseCond.notify_all();

}

void FFARender::seek()
{
    seekFlag = false;
}

void FFARender::close()
{
    maxBufSize = -1;
    if(aSink){
        aSink->deleteLater();
        aSink = nullptr;
    }
    if(aPars){
        delete aPars;
        aPars = nullptr;
    }
    if(abuf){
        av_freep(&abuf);
        abuf = nullptr;
    }
    if(sonicCtx){
        sonicDestroyStream(sonicCtx);
        sonicCtx = nullptr;
    }
    readyFlag = false;
    seekFlag = false;
    pauseFlag = false;
    speedFlag = false;
    speed = 1;

}

void FFARender::setSpeed(float speed_)
{
    speed = speed_;
    speedFlag.store(true,std::memory_order_release);
}

void FFARender::setVolume(double volume)
{
    if(aSink){
        aSink->setVolume(volume);
    }

}

void FFARender::run()
{
    while(!m_stop){
        AVFrame* frame = frmQueue->dequeue();
        if(frame == nullptr){
            m_stop = true;
            break;
        }
        if(aPars == nullptr){
            initAudio();
        }
        if(!frame->data[0]&&!frame->data[1]&&!frame->data[2]){
            av_frame_free(&frame);
            continue;
        }
        if(pauseFlag.load(std::memory_order_acquire)){
            std::unique_lock<std::mutex> lock(mutex);
            pauseFlag.store(false,std::memory_order_release);
        }
        playAudio(frame);
        av_frame_unref(frame);
        av_frame_free(&frame);
    }
}

void FFARender::initAudio()
{
    initAudioPars();
    clockBase = aPars->sampleSize*aPars->nbChannels*aPars->sampleRate;

    //设置音频格式
    aFormat.setSampleRate(aPars->sampleRate);
    aFormat.setChannelCount(aPars->nbChannels);
    aFormat.setSampleFormat(QAudioFormat::Int16);

    outputDevice = QMediaDevices::defaultAudioOutput();
    //检查格式是否支持
    if(!outputDevice.isFormatSupported(aFormat)){
        aFormat = outputDevice.preferredFormat();
        qWarning()<<"Requested audio format not supported,using device preferred format";
    }
    aSink = new QAudioSink(outputDevice,aFormat);
    aSink->setVolume(1.0);
    aDevice = aSink->start();

    //准备事件
    if(!readyFlag.load(std::memory_order_relaxed)){
        totalSec = aDecoder->getTotalsec();
        FFEvent* event = new FFReadyEvent(playerCtx,totalSec,-1);
        FFEventQueue::getInstance().enqueue(event);
        readyFlag.store(true,std::memory_order_release);
    }
}

void FFARender::initAudioPars()
{
    FFAudioPars* tmpPars = aDecoder->getAudioPars();
    if(tmpPars == nullptr){
        return;
    }
    aPars = new FFAudioPars();
    memcpy(aPars,tmpPars,sizeof(FFAudioPars));

}

void FFARender::playAudio(AVFrame *frame)
{   //将帧中的音频数据输出到音频设备，并处理变速（通过sonic库），同时更新全局时钟和发送进度事件。
    uint8_t* playerBuffer = nullptr;
    int64_t bufSize = av_samples_get_buffer_size(nullptr,
                                                 aPars->nbChannels,
                                                 frame->nb_samples,
                                                 aPars->aFormatEnum,1);
    /*使用FFmpeg函数av_samples_get_buffer_size计算存储该帧音频样本所需的总字节数。
     * 参数：通道数、样本数、样本格式（枚举）、对齐方式（1表示不对齐）*/
    if(bufSize > maxBufSize){//当前bufSize超过最大值
        maxBufSize = bufSize;//扩大最大容量
        if(abuf){
            av_freep(&abuf);//释放abuf缓冲区
        }
        abuf = static_cast<uint8_t*>(av_mallocz(maxBufSize));
        //av_mallocz 是 FFmpeg 中专门用于分配对齐且清零的内存块的函数
        //重新分配缓冲区
        if(!abuf){
            std::cerr<<"malloc abuf!"<<std::endl;
            return;
        }
    }
    memcpy(abuf,frame->data[0],bufSize);//拷贝音频帧,这里假定音频格式为交错模式AV_SAMPLE_FMT_S16,所有通道数据连续存放在 data[0]
    playerBuffer = abuf;//指向原始缓冲区
    if(speedFlag.load(std::memory_order_acquire)){//速度改变需要重新创建
        if(sonicCtx!=nullptr){
            sonicDestroyStream(sonicCtx);
        }
        //动态创建sonic上下文
        sonicCtx = sonicCreateStream(aPars->sampleRate,aPars->nbChannels);
        //设置速度
        sonicSetSpeed(sonicCtx,speed);
        //设置音高
        sonicSetPitch(sonicCtx,1.0f);
        //设置速率
        sonicSetRate(sonicCtx,1.0f);
        //仅速度被改变，音高和速率保持 1.0，确保变速不变调
        speedFlag.store(false,std::memory_order_release);
    }
    if(fabs(speed -1)>=0.1){//若速度偏离足够大，执行实际变速
        int actual_out_samples = bufSize/(aPars->nbChannels*av_get_bytes_per_sample(aPars->aFormatEnum));
        //计算样本数:总字节数除以（每样本字节数 × 通道数）,得到原始 PCM 的样本个数。

        int out_ret = sonicWriteShortToStream(sonicCtx,(int16_t*)abuf,actual_out_samples);
        //写入 sonic 流：sonicWriteShortToStream 将原始样本（强制转换为 int16_t*）写入 sonic 处理引擎

        int num_samples = sonicSamplesAvailable(sonicCtx);
        //获取可用样本：sonicSamplesAvailable 返回 sonic 内部缓冲区中已处理完成的样本数。
        int out_size = num_samples*av_get_bytes_per_sample(aPars->aFormatEnum)*aPars->nbChannels;

        av_fast_malloc(&abufOut,&abufOutSize,out_size);//复用已经有的内存
        //使用 av_fast_malloc 动态调整 abufOut 的大小，确保能容纳处理后的数据。
        int sonic_samples;
        if(out_ret){
            //读取处理结果：若写入成功（out_ret 非零），则从 sonic 流读取处理后的样本，
            //更新 playerBuffer 指向新缓冲区，并调整 bufSize 为处理后的实际字节数。
            sonic_samples = sonicReadShortFromStream(sonicCtx,(int16_t*)abufOut,num_samples);
            playerBuffer = abufOut;
            bufSize = sonic_samples*av_get_bytes_per_sample(aPars->aFormatEnum)*aPars->nbChannels;
        }
    }
    //更新全局时钟
    int64_t globalTime = frame->pts * av_q2d(aPars->timeBase)*1e3;
    FFGlobalClock* gClock = FFGlobalClock::getInstance();
    gClock->setClock(globalTime);
    //将帧的 PTS（以时间基为单位）转换为毫秒，并设置到全局时钟。该时钟用于音视频同步，其他模块（如视频渲染）可据此调整播放节奏。

    //发送处理事件,计算当前播放的秒数
    sendProcessEvent((globalTime + 500)/1000);
    //计算当前秒数（四舍五入：加500毫秒除以1000）发送给播放窗口以更新进度条等

    //将音频数据写入音频设备
    int bytesWrite = 0;
    while(bytesWrite < bufSize){//写入的字节数累积直到达到 bufSize
        //aDevice 是从 QAudioSink 获取的 QIODevice，用于向声卡写入 PCM 数据。
        int64_t bytes = aDevice->write((const char*)playerBuffer + bytesWrite,bufSize - bytesWrite);
        if(bytes<=0){
            std::this_thread::sleep_for(std::chrono::microseconds(1000));
            continue;
        }
        bytesWrite += bytes;
        //循环写入直到所有数据发送完毕。如果 write 返回 ≤0（可能设备缓冲区满），则短暂休眠（1000 微秒）后重试，避免忙等待。
    }
}

void FFARender::sendEndEvent()
{
    FFEvent* stopEv = new FFStopEvent(playerCtx);
    FFEventQueue::getInstance().enqueue(stopEv);
}

void FFARender::sendProcessEvent(int curSeconds)
{
    if(curSeconds == lastSec){
        return;
    }
    lastSec = curSeconds;
    FFEvent* processEv = new FFProcessEvent(playerCtx,curSeconds);
    FFEventQueue::getInstance().enqueue(processEv);
}

