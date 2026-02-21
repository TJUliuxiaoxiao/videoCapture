#include "ffvfilterthread.h"
#include "capture/ffcapturecontext.h"
#include "opencv/fffacedetector.h"
#include "qobjectdefs.h"
#include "queue/ffvframequeue.h"

using namespace FFCaptureContextType;
FFVFilterThread::FFVFilterThread() {
    //设置叠加图层在背景上的默认位置和大小
    overlayX = overlayY = 0;
    overlayWidth = overlayHeight = 400;//400像素
    overlayPts = 0;
    overlayDts = 0;
    encoderFlag.store(false);

    //for test
    cameraFlag.store(false);
    videoFlag.store(false);
    screenFlag.store(false);
    pauseFlag.store(false);

    pauseTime = 0;//暂停时长
    lastPauseTime = 0;//上一次暂停的时间戳
}

FFVFilterThread::~FFVFilterThread()
{
    if(faceDetector){
        delete faceDetector;
        faceDetector = nullptr;
    }
    if(overlayProcessor){
        delete overlayProcessor;
        overlayProcessor = nullptr;
    }
    if(cameraFrame){
        av_frame_unref(cameraFrame);
        av_frame_free(&cameraFrame);
    }
    if(cameraFrame2){
        av_frame_unref(cameraFrame2);
        av_frame_free(&cameraFrame2);
    }
    if(lastVideoFrame){
        av_frame_unref(lastVideoFrame);
        av_frame_free(&lastVideoFrame);
    }
    if(videoFrame){
        av_frame_unref(videoFrame);
        av_frame_free(&videoFrame);
    }
    if(screenFrame){
        av_frame_unref(screenFrame);
        av_frame_free(&screenFrame);
    }
}

void FFVFilterThread::init(FFVFrameQueue *frmQueue1_, FFVFrameQueue *frmQueue2_, FFVFrameQueue *frmQueue3_, FFVFrameQueue *renderFrmQueue_, FFVFilter *filter_, FFCapWindow *capWindow_)
{
    frmQueue1 = frmQueue1_;//屏幕捕获帧队列
    frmQueue2 = frmQueue2_;//摄像头帧队列
    frmQueue3 = frmQueue3_;//视频文件帧队列
    renderFrmQueue = renderFrmQueue_;//用于渲染的队列
    filter = filter_;//滤镜对象指针,未使用，用于扩展
    capWindow = capWindow_;//主窗口
    //创建人脸检测器并初始化
    faceDetector = new FFFaceDetector();
    faceDetector->init();//加载人脸检测的级联分类器模型
    //创建视频叠加处理器并初始化
    overlayProcessor = new FFOverlayProcessor();
    overlayProcessor->init();//初始化视频叠加类
}

void FFVFilterThread::startEncoder()
{
    std::lock_guard<std::mutex> lock(mutex);
    encoderFlag.store(true);
    cond.notify_one();
}

void FFVFilterThread::stopEncoder()
{
    std::lock_guard<std::mutex> lock(mutex);
    encoderFlag.store(false);
    pauseFlag.store(false);
    pauseTime = 0;
    lastPauseTime = 0;
    cond.notify_one();
}

void FFVFilterThread::openVideoSource(int sourceType)
{
    std::lock_guard<std::mutex> lock(mutex);
    enum demuxerType type = static_cast<demuxerType>(sourceType);
    switch (type){
    case SCREEN:
        screenFlag.store(true);
        break;
    case CAMERA:
        cameraFlag.store(true);
        break;
    case VIDEO:
        videoFlag.store(true);
        eofFlag =false;
        lastVideoTime = 0;
        break;
    default:
        break;
    }
    cond.notify_one();
}

void FFVFilterThread::closeVideoSource(int sourceType)
{
    std::lock_guard<std::mutex> lock(mutex);
    enum demuxerType type = static_cast<demuxerType>(sourceType);
    switch (type){
    case SCREEN:
        screenFlag.store(false);
        break;
    case CAMERA:
        cameraFlag.store(false);
        break;
    case VIDEO:
        videoFlag.store(false);
        eofFlag = false;
        lastVideoTime = 0;
        break;
    default:
        break;
    }
    cond.notify_one();
}

void FFVFilterThread::setWhiteValue(int value)
{
    if(faceDetector){
        faceDetector->setWhiteValue(value);
    }
}

void FFVFilterThread::setSmoothValue(int value)
{
    if(faceDetector){
        faceDetector->setSmoothValue(value);
    }
}

void FFVFilterThread::pauseEncoder()
{
    std::lock_guard<std::mutex> lock(mutex);
    if(pauseFlag.load()){
        pauseTime += av_gettime_relative() - lastPauseTime;
        pauseFlag.store(false);
    }
    else{
        pauseFlag.store(true);
        lastPauseTime = av_gettime_relative() - lastPauseTime;
    }
}

bool FFVFilterThread::peekStart()
{
    return encoderFlag.load();
}

void FFVFilterThread::wakeAllThread()
{
    if(frmQueue1){
        frmQueue1->wakeAllThread();
    }
    if(frmQueue2){
        frmQueue2->wakeAllThread();
    }
    if(frmQueue3){
        frmQueue3->wakeAllThread();
    }
    if(renderFrmQueue){
        renderFrmQueue->wakeAllThread();
    }
}

void FFVFilterThread::run()
{
    while(!m_stop){
        //检查各个模块状态
        bool hasVideo = videoFlag.load();
        bool hasScreen = screenFlag.load();
        bool hasCamera = cameraFlag.load();
        bool hasEncoder = encoderFlag.load();
        bool hasPause = pauseFlag.load();

        // 空闲等待,若所有模块均未启用，则通过条件变量等待100ms,避免空转
        // 外部可通过cond.notify_one()唤醒线程

        if(!hasVideo&&!hasScreen&&!hasCamera&&!hasEncoder)
        {
            std::unique_lock<std::mutex> lock(mutex);
            cond.wait_for(lock,std::chrono::milliseconds(100));
            continue;
        }

        // 记录开始时间,使用av_gettime_relative记录处理起始时间
        //返回单调时钟的微妙数,用于测量处理一(组)帧,所需的时间,后续会用到这个插值
        auto start = av_gettime_relative();

        // 视频文件帧处理
        if(hasVideo&&!eofFlag){
            if(lastVideoTime==0){//首次启动
                lastVideoTime = av_gettime_relative();
                videoFrame = frmQueue3->dequeue();//首次启动时立即获取一帧,并发送,记录首次时间
                if(videoFrame==nullptr||videoFrame->data[0]==nullptr){
                    eofFlag = true;
                    continue;
                }//取出的帧为空或者数据无效,则设置eofFlag = true,停止数据处理


                // 跨线程UI更新,通过QMetaObject::invokeMethod将帧发送到
                // 主窗口capWindow的sendVideoFrame槽,确保UI操作在主线程执行
                QMetaObject::invokeMethod(capWindow,
                                          "sendVideoFrame",
                                          Qt::QueuedConnection,
                                          Q_ARG(AVFrame*,av_frame_clone(videoFrame))
                                          );

                /*将上一帧lastVideoFrame释放,保存当前帧指针*/
                if(lastVideoFrame){
                    av_frame_unref(lastVideoFrame);
                    av_frame_free(&lastVideoFrame);
                }
                lastVideoFrame = videoFrame;
            }
            else{
                //不是首次启动,获取当前相对时间
                int64_t currentVideoTime = av_gettime_relative();
                if(currentVideoTime-lastVideoTime>=33000){//超过33ms
                /*只有距离上次发送超过 33ms 时才取新帧，从而实现约 30fps 的恒定帧率。*/
                    videoFrame = frmQueue3->dequeue();
                    if(videoFrame==nullptr||videoFrame->data[0]==nullptr){
                        eofFlag = true;
                        continue;
                    }
                    QMetaObject::invokeMethod(capWindow,"sendVideoFrame",Q_ARG(AVFrame*,av_frame_clone(videoFrame)));
                    if(lastVideoFrame){
                        av_frame_unref(lastVideoFrame);
                        av_frame_free(&lastVideoFrame);
                    }
                    lastVideoFrame = videoFrame;
                    lastVideoTime = currentVideoTime;

                }
            }
        }
        // 屏幕帧处理
        if(hasScreen){
            screenFrame = frmQueue1->dequeue();
            if(screenFrame!=nullptr){
                QMetaObject::invokeMethod(capWindow,
                                          "sendScreenFrame",
                                          Qt::QueuedConnection,
                                          Q_ARG(AVFrame*,av_frame_clone(screenFrame)));
            }
        }

        //摄像头帧处理 + 人脸检测
        if(hasCamera){
            //从摄像头帧队列中取出原始帧
            cameraFrame = frmQueue2->dequeue();
            if(cameraFrame!=nullptr){
                /*调用人脸检测器detectFace检测*/
                cameraFrame2 = faceDetector->detectFace(cameraFrame);
                //将处理后的帧克隆发送后显示
                QMetaObject::invokeMethod(
                    capWindow,"sendCameraFrame",
                    Qt::QueuedConnection,
                    Q_ARG(AVFrame*,av_frame_clone(cameraFrame2)));
            }
        }


        //叠加顺序:倒序
        /*capWindow->getOvelayNumbers持有用户配置的图层顺序,返回一个整形vector
         （例如列表 [SCREEN, VIDEO, CAMERA] 表示屏幕在最底，视频在中间，摄像头在最上层）*/
        overlayNumbers = capWindow->getOverlayNumbers();
        for(auto iter = overlayNumbers.rbegin();iter!=overlayNumbers.rend();++iter)
        {
            int type = *iter;
            if(type == -1)continue;
            if(type == CAMERA && cameraFrame2 && hasCamera){
                overlayFrame(av_frame_clone(cameraFrame2),type);
                av_frame_free(&cameraFrame2);
            }//摄像头
            else if(type == SCREEN && screenFrame && hasScreen){
                overlayFrame(av_frame_clone(screenFrame),type);
                av_frame_free(&screenFrame);
            }//屏幕
            else if(type == VIDEO && lastVideoFrame && hasVideo)
            {
                // videoFrame2 = av_frame_clone(lastVideoFrame);
                // overlayFrame(av_frame_clone(videoFrame2),type);
                // av_frame_free(&videoFrame2);
                overlayFrame(av_frame_clone(lastVideoFrame),type);
                av_frame_free(&lastVideoFrame);
            }//视频文件
        }

        //编码帧生成
        if(hasEncoder&&!hasPause){
            std::unique_lock<std::mutex> lock(mutex);
            //overlayProcessor->getOverlayFrame() 返回当前背景
            //图像（所有图层合成后的结果）的 AVFrame
            AVFrame* encodeFrame = overlayProcessor->getOverlayFrame();
            auto end = av_gettime_relative();
            int64_t duration = (end - start) * 10;
            overlayPts = av_gettime_relative()*10+duration-pauseTime * 10;
            encodeFrame->pkt_dts = overlayDts;
            encodeFrame->pts = overlayPts;
        }
    }
}

void FFVFilterThread::overlayFrame(AVFrame *frame, int type)
{
    if(frame){
        capWindow->getOverlayPos(&overlayX,&overlayY,&overlayWidth,&overlayHeight,type);
        overlayProcessor->sendOverlayImage(av_frame_clone(frame),overlayX,overlayY,overlayWidth,overlayHeight);
        av_frame_free(&frame);
    }
}





