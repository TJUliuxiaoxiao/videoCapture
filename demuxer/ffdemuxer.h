#ifndef FFDEMUXER_H
#define FFDEMUXER_H
#include <string>
#include <iostream>
#include <atomic>
#include <mutex>


extern "C"{
#include <libavformat/avformat.h>
#include<libavutil/cpu.h>
#include<libavdevice/avdevice.h>
#include<libavutil/time.h>
}
class FFVPacketQueue;
class FFAPacketQueue;
using std::string;
class FFDemuxer
{
public:
    explicit FFDemuxer(); // 显式构造函数，防止隐式转换
    FFDemuxer(const FFDemuxer&) = delete; // 禁止拷贝构造
    FFDemuxer& operator=(const FFDemuxer) = delete;//编译器将此函数彻底删除，确保不会生成默认的拷贝构造函数
    ~FFDemuxer();
    void init(string const& url_,string const&format_,
              FFAPacketQueue* aPktQueue_,FFVPacketQueue* vPktQueue_,int type_);//初始化函数，设置媒体文件路径、格式、音视频包队列和类型。
    int demux(); // 执行解封装，即从媒体文件中读取音视频包，并放入队列
    AVStream* getAStream();//获取音频流
    AVStream* getVStream();//获取视频流
    void wakeAllThread();//唤醒所有线程
    void close();//关闭解封装器
    void initDemuxer();// 初始化解封装器（打开文件，查找流信息等）
    int getType();//获取类型
private:
    void printError(int ret);
private:
    string url;//媒体文件路径
    string format;//指定的格式，例如RTSP 文件等
    AVFormatContext* fmtCtx =  nullptr;//FFmpeg格式的上下文，核心结构
    AVDictionary* opts = nullptr;//打开媒体文件的选项
    AVStream* aStream = nullptr;//音频流
    AVStream* vStream = nullptr;//视频流
    FFAPacketQueue *aPktQueue = nullptr;
    FFVPacketQueue *vPktQueue = nullptr;
    AVRational aTimeBase;
    AVRational vTimeBase;

    const AVInputFormat* inputFmt = nullptr;
    int type;
    bool offsetFlag = false;

    std::mutex mutex;
    bool stopFlag = false;//控制解封装循环

};

#endif // FFDEMUXER_H
