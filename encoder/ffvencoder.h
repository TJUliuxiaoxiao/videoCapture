#ifndef FFVENCODER_H
#define FFVENCODER_H

#include <mutex>
extern "C"
{
#include <libavformat/avformat.h>
#include <libavutil/time.h>
#include <libavcodec/avcodec.h>
}
class FFVPacketQueue;

struct FFVEncoderPars{
    int width;
    int height;
    int bitRate;
    enum AVPixelFormat videoFmt;
    AVRational frameRate;
};

class FFVEncoder
{
public:
    FFVEncoder();
    ~FFVEncoder();

    void init(FFVPacketQueue* pktQueue_);
    void close();
    void wakeAllThread();
    int encode(AVFrame* frame,int streamIndex,int64_t pts,AVRational timeBase);
    AVCodecContext * getCodecCtx();
    FFVEncoderPars* getEncoderPars();
    void initVideo(AVFrame* frame,AVRational fps);
private:
    void printError(int ret);
private:
    FFVPacketQueue* pktQueue = nullptr;
    AVCodecContext* codecCtx = nullptr;
    FFVEncoderPars* vPars = nullptr;
    int64_t lastPts = -1;//上一帧的PTS，用于PTS生成和防止重复
    std::mutex mutex;
};

#endif // FFVENCODER_H
