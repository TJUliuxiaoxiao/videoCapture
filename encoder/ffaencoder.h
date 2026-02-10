#ifndef FFAENCODER_H
#define FFAENCODER_H
/*使用FFmpeg编码音频数据，并将编码后的数据包放入一个队列中
 * （由FFAPacketQueue管理）。它支持多线程操作（因为有互斥锁和唤醒线程的方法），
 * 并且可以处理待编码的音频帧。*/
#include <vector>
#include <mutex>
extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}
class FFAPacketQueue;//存放编码后的数据包
struct FFAEncoderPars{
    int sampleRate;
    int nbChannel;
    int biteRate;
    enum AVSampleFormat audioFmt;
};//定义编码器参数，包括采样率、通道数、比特率和采样格式


struct PendingFrame{
    std::vector<uint8_t> data[8];//每个通道一个vector每个vector存放不定长的字节数据
    int samples = 0;
    int64_t next_pts = 0;
};//定义了一个待处理帧,包含一个数据数组(每个通道一个vector)、样本数和下一个显示时间戳


class FFAEncoder
{
public:
    FFAEncoder();
    ~FFAEncoder();
    void init(FFAPacketQueue* pktQueue_);//初始化编码器，传入一个存放编码数据包的队列
    void close();//关闭编码器,释放资源
    void wakeAllThread();//唤醒所有线程,可能用户多线程编码时。
    int encode(AVFrame* frame,int streamIndex,int64_t pts,AVRational timeBase);
    //编码一帧音频数据。参数包括带编码的帧、流索引、显示时间戳和时间基。返回编码结果（可能是成功或错误码）;
    FFAEncoderPars* getEncoderPars();//获取编码器参数
    AVCodecContext* getCodecCtx();//获取FFmpeg的编码上下文。
    void initAudio(AVFrame* frame);//初始化音频编码器,可能根据传入的帧设置编码参数。
private:
    void printError(int ret);//打印错误信息
    AVFrame *createFrameFromPending();//从待处理帧创建一个AVFrame
    void clearPendingFrame();//清空待处理帧
private:
    FFAPacketQueue* pktQueue = nullptr;//指向存放编码数据包的队列
    AVCodecContext* codecCtx = nullptr;//FFmpeg编码上下文
    FFAEncoderPars* aPars = nullptr;//编码器参数
    std::mutex mutex;//互斥锁,用于多线程同步
    PendingFrame pendingFrame;//待处理帧,可能用于缓存尚未编码的音频数据
};

#endif // FFAENCODER_H
