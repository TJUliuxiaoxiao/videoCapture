#ifndef FFOPENVSOURCEEVENT_H
#define FFOPENVSOURCEEVENT_H

#include"ffevent.h"
//用于打开一个音视频源(如摄像头、屏幕、视频文件、音频文件等)
//当用户通过 UI 选择开启某个源时，
//会创建该事件并放入全局事件队列 FFEventQueue，由后台线程（通常是事件处理线程）执行其 work() 方法。
class FFOpenSourceEvent : public FFEvent
{
public:
    FFOpenSourceEvent(FFCaptureContext* captureCtx,enum FFCaptureContextType::demuxerType sourceType_,
                      std::string const& url_,std::string const& format_);
    /*sourceType_：枚举类型，表示源的种类，如 VIDEO、CAMERA、SCREEN、AUDIO、MICROPHONE 等（来自命名空间 FFCaptureContextType）。
    url_：源的路径或设备标识符，例如摄像头设备名、文件路径、RTMP 地址等。
    format_：封装格式，如 "mp4"、"flv"、"dshow"（DirectShow 设备）等。*/
    virtual void work() override;
    void init();
    void start();

private:
    std::string url;
    std::string format;

    int sourceType;
    int index;
};

#endif // FFOPENVSOURCEEVENT_H
