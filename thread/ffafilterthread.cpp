#include "ffafilterthread.h"
#include "queue/ffaframequeue.h"
#include "filter/ffafilter.h"
#include "capture/ffcapturecontext.h"
using namespace FFCaptureContextType;
FFAFilterThread::FFAFilterThread() {
    encoderFlag.store(false);
    pauseFlag.store(false);
    audioFlag.store(false);
    microphoneFlag.store(false);
    pauseTime = 0;
    lastpauseTime = 0;
}

FFAFilterThread::~FFAFilterThread()
{
    if(audioFrame){
        av_frame_free(&audioFrame);
    }
    if(microPhoneFrame){
        av_frame_free(&microPhoneFrame);
    }
}

void FFAFilterThread::openAudioSource(int audioType)
{
    cond.notify_all();
    enum demuxerType type = static_cast<demuxerType>(audioType);
    if(type==AUDIO){
        audioFlag.store(true,std::memory_order_seq_cst);
    }else if(type==MICROPHONE){
        microphoneFlag.store(true,std::memory_order_seq_cst);
    }
}

void FFAFilterThread::closeAudioSource(int audioType)
{
    cond.notify_all();
    enum demuxerType type = static_cast<demuxerType>(audioType);
    if(type==AUDIO){
        audioFlag.store(false,std::memory_order_seq_cst);
    }else if(type==MICROPHONE){
        microphoneFlag.store(false,std::memory_order_seq_cst);
        //在 acquire-release 的基础上，进一步要求所有线程看到相同的全局顺序，是最强的约束。
        //换句话说，就好像所有核上的原子操作被一个虚构的全局时钟排成一条单一的时间线，
        //每个操作在这个时间线上都有一个明确的位置，且所有核都认同这个时间线。
    }
}

void FFAFilterThread::init(FFAFrameQueue *frmQueue1_, FFAFrameQueue *frmQueue2_, FFAFilter *filter_)
{
    frmQueue1 = frmQueue1_;
    frmQueue2 = frmQueue2_;
    filter = filter_;
}

void FFAFilterThread::startEncoder()
{
    cond.notify_all();
    encoderFlag.store(true,std::memory_order_seq_cst);
}

void FFAFilterThread::stopEncoder()
{
    cond.notify_all();
    encoderFlag.store(false,std::memory_order_seq_cst);
    pauseFlag.store(false);
    pauseTime = 0;
    lastpauseTime = 0;
}

void FFAFilterThread::pauseEncoder()//一个编码器暂停/恢复的切换功能，并记录累计暂停时间。
{
    std::lock_guard<std::mutex> lock(mutex);
    if(pauseFlag.load()){
        pauseTime += av_gettime_relative() - lastpauseTime;//暂停时间累加
        pauseFlag.store(false);//
    }else{
        pauseFlag.store(true);
        lastpauseTime = av_gettime_relative();
    }
    //使用 av_gettime_relative() 记录暂停开始时刻（lastpauseTime）和
    //计算暂停时长（pauseTime += now - lastpauseTime），
    //这正是该函数的典型用途——测量相对时间间隔。
    //因此，必须借助状态变量来记录暂停的起始时刻（lastpauseTime）
    //并累加每次暂停的时长（pauseTime）。这两个变量配合 pauseFlag 一起，
    //共同维护了暂停/恢复的历史信息。
    //计算总暂停时间pauseTime的作用:同步多路流
    //如果同时处理多个编码器（如音视频分别编码），各路的暂停时间需保持一致，避免不同步。记录暂停时长可用于对齐或补偿。
}

void FFAFilterThread::setAudioVolume(double value)
{
    if(filter){
        if(audioFlag.load()&&microphoneFlag.load()){
            filter->setVolume(value,-1);
        }else{
            filter->setAudioVolume(value);
        }
    }
}

void FFAFilterThread::setMicrophoneVolume(double value)
{
    if(filter){
        if(audioFlag.load()&&microphoneFlag.load()){
            filter->setVolume(-1,value);
        }else{
            filter->setMicrophoneVolume(value);
        }
    }
}

bool FFAFilterThread::peekStart()
{
    return encoderFlag.load();
}

void FFAFilterThread::wakeAllThread()
{
    if(frmQueue1){
        frmQueue1->wakeAllThread();
    }
    if(frmQueue2){
        frmQueue2->wakeAllThread();
    }
}

void FFAFilterThread::run()
{
    while(!m_stop){
        bool microphoneActive = microphoneFlag.load(std::memory_order_seq_cst);
        bool audioActive = audioFlag.load(std::memory_order_seq_cst);
        bool encoderActive = encoderFlag.load(std::memory_order_seq_cst);
        bool pauseActive = pauseFlag.load(std::memory_order_seq_cst);

        if(!microphoneActive && !audioActive&& !encoderActive){
            std::unique_lock<std::mutex> lock(mutex);
            cond.wait_for(lock,std::chrono::milliseconds(100));
            continue;
        }
        int64_t start = av_gettime_relative();

        //混音:[声卡] + [麦克风]
        if(audioActive&&microphoneActive){
            audioFrame = frmQueue1->dequeue();
            microPhoneFrame = frmQueue2->dequeue();
            if(encoderActive&&!pauseActive){
                int ret = filter->sendFilter(audioFrame,microPhoneFrame,start,pauseTime);
                if(ret < 0){
                    std::cerr<<"FilterThread Fail!"<<std::endl;
                    m_stop = true;
                }

            }
            else{
                if(audioFrame){
                    av_frame_unref(audioFrame);
                    av_frame_free(&audioFrame);
                }
                if(microPhoneFrame){
                    av_frame_unref(audioFrame);
                    av_frame_free(&microPhoneFrame);
                }
            }
        }
        //[声卡]
        else if(audioActive&&!microphoneActive){
            audioFrame = frmQueue1->dequeue();
            if(encoderActive&&!pauseActive){
                filter->sendSingleFilter(audioFrame,start,pauseTime,AUDIO);
            }else{
                if(audioFrame){
                    av_frame_unref(audioFrame);
                    av_frame_free(&audioFrame);
                }
            }
        }
        //[麦克风]
        else if(microphoneActive){
            microPhoneFrame = frmQueue2->dequeue();
            if(encoderActive&&!pauseActive){
                filter->sendSingleFilter(microPhoneFrame,start,pauseTime,MICROPHONE);
            }else{
                if(microPhoneFrame){
                    av_frame_unref(microPhoneFrame);
                    av_frame_free(&microPhoneFrame);
                }
            }
        }
        //[静音包]
        else{
            if(encoderActive && !pauseActive){
                AVFrame* muteFrame = generateMuteFrame();
                filter->sendFrame(muteFrame,start,pauseTime);
            }
        }
    }
}

AVFrame *FFAFilterThread::generateMuteFrame()
{
    //分配AVFrame结构
    AVFrame* frame = av_frame_alloc();

    //设置帧属性
    frame->format = FF_SAMPLE_FMT;
    frame->sample_rate = FF_SAMPLE_RATE;
    frame->nb_samples = 1024;
    //分配声道布局
    av_channel_layout_default(&frame->ch_layout,2);
    //分配数据缓冲区
    int ret = av_frame_get_buffer(frame,0);
    if(ret < 0){
        std::cerr<<"get mute Frame buffer error"<<std::endl;
        av_frame_free(&frame);
        return nullptr;
    }
    //将缓冲区设置置为静音
    ret = av_samples_set_silence(frame->extended_data,0,frame->nb_samples,frame->ch_layout.nb_channels,FF_SAMPLE_FMT);
    if(ret < 0){
        std::cerr<<"Set Samples Silence Fail"<<std::endl;
        av_frame_free(&frame);
        return nullptr;
    }
    return frame;
}











