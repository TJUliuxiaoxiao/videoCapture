#ifndef FFVENCODERTHREAD_H
#define FFVENCODERTHREAD_H

#include "ffthread.h"
#include <mutex>
extern "C"{
#include <libavformat/avformat.h>
}

class FFVEncoderPars;
class FFVEncoder;
class FFVFrameQueue;
class FFVideoPars;
class FFMuxer;
class FFVFilter;

class FFVEncoderThread : public FFThread
{
public:
    FFVEncoderThread();
    virtual ~FFVEncoderThread() override;
    void init(FFVFilter* vFilter_,FFVEncoder* vEncoder_,FFMuxer *muxer_,FFVFrameQueue* frmQueue_);
    void close();
    void wakeAllThread();
protected:
    virtual void run() override;
    //将 run() 声明为 protected，意味着只有 QThread
    //自身及其派生类可以访问它，普通外部代码无法直接调用。
    //这样就强制用户通过 start() 启动线程，保证了线程的正确行为。
private:
    void initEncoder(AVFrame* frame);

private:
    FFVEncoder* vEncoder = nullptr;
    FFVFrameQueue* frmQueue = nullptr;
    FFMuxer* muxer = nullptr;
    int streamIndex = -1;
    FFVFilter* vFilter = nullptr;
    AVRational videoTimeBase;
    AVRational frameRate;
    int64_t firstFramePts = 0;
    bool isFirstFrame = true;
    std::mutex mutex;
};

#endif // FFVENCODERTHREAD_H
