#ifndef FFADECODERTHREAD_H
#define FFADECODERTHREAD_H
#include "ffthread.h"
extern "C"{
#include <libavformat/avformat.h>
}
class FFAPacketQueue;
class FFADecoder;
class FFPlayContext;

class FFADecoderThread:public FFThread
{
public:
    FFADecoderThread();
    virtual ~FFADecoderThread() override;
    void init(FFADecoder* aDecoder_,FFAPacketQueue* aPktQueue_);
    void wakeAllThread();
    void close();

    bool peekStop();
protected:
    virtual void run() override;
private:
    FFAPacketQueue* aPktQueue = nullptr;
    FFADecoder* aDecoder = nullptr;

};

#endif // FFADECODERTHREAD_H
