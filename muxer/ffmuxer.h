#ifndef FFMUXER_H
#define FFMUXER_H
#include <iostream>
#include <mutex>
#include <thread>
#include <chrono>
extern "C"{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
}
/*
 * @brief FFmpeg复用器（Muxer）类
 * @details 用于将编码后的音频和视频流混合封装到容器文件中
 *          支持添加音频流和视频流，将编码数据包写入容器
 */
class FFMuxer
{
public:
    FFMuxer();
    ~FFMuxer();
    /*
     * @brief 初始化复用器
     * @param url_ 输出文件路径或网络URL
     * @param format_ 容器格式，默认为"mp4"
     *
     * @example
     *   muxer.init("output.mp4", "mp4");
     *   muxer.init("rtmp://localhost/live/stream", "flv");
     */
    void init(const std::string& url_,
              std::string const& format_ = "mp4");
    /*
     * @brief 添加媒体流到复用器
     * @param codecCtx 编码器上下文，包含流的编码参数
     *
     * @note 这个函数的设计有问题：根据streamCount判断是音频还是视频，
     *       但不会检查传入的codecCtx的类型。
     *       建议改进：根据codecCtx->codec_type自动判断类型
     */
    void addStream(AVCodecContext* codecCtx);

    /*
     * @brief 将编码后的数据包写入容器
     * @param packet 编码后的AVPacket
     * @return 成功返回0，失败返回负数错误码
     */
    int mux(AVPacket* packet);

    /*
     * @brief 写入文件头部
     * @note 必须在所有流添加完成后，在写入任何数据包前调用
     */
    void writeHeader();

    /*
     * @brief 写入文件尾部
     * @note 必须在所有数据包写入完成后调用
     */
    void writeTrailer();

    /*
     * @brief 获取音频流索引
     * @return 音频流在容器中的索引，如果没有音频流返回-1
     */
    int getAStreamIndex();

    /*
     * @brief 获取视频流索引
     * @return 视频流在容器中的索引，如果没有视频流返回-1
     *
     * @note 函数名不明确，建议改为getVStreamIndex()
     */
    int getVStreamIndex();

    /*
     * @brief 关闭复用器并释放资源
     */
    void close();

private:
    /*
     * @brief 内部初始化函数
     * @details 创建输出格式上下文和IO上下文
     */
    void initMuxer();

    /*
     * @brief 打印FFmpeg错误信息
     * @param ret FFmpeg函数返回的错误码
     */
    void printError(int ret);
private:
    //基础配置
    std::string url;    //输出文件路径或url
    std::string format; //容器格式(如"mp4","flv","avi"等等);

    //FFmpeg核心对象
    AVFormatContext* fmtCtx = nullptr;//输出格式上下文
    AVCodecContext* aCodecCtx = nullptr;//音频编解码器上下文
    AVCodecContext* vCodecCtx = nullptr;//视频编解码器上下文

    //流对象
    AVStream* aStream = nullptr;//音频流对象
    AVStream* vStream = nullptr;//视频流对象
    //流索引
    int aStreamIndex = -1;//音频流索引
    int vStreamIndex = -1;//视频流索引

    //状态标志
    bool headerFlag = false;//文件头是否已经写入
    bool trailerFlag = false;//文件尾是否已经写入
    bool readyFlag = false;//复用器是否已经初始化
    bool hasAudio = false;
    bool hasVideo = false;
    int streamCount = 0;//已添加的流数量
    std::mutex mutex;
};

#endif // FFMUXER_H
