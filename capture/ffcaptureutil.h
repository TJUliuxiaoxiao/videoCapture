#ifndef FFCAPTUREUTIL_H
#define FFCAPTUREUTIL_H

#include <QObject>
#include "ui/ffcapwindow.h"
#include "capture/ffcapturecontext.h"
// #include "thread/ffthread.h"
#include "thread/ffthreadpool.h"
class FFCaptureUtil
{
    Q_OBJECT
public:
    explicit FFCaptureUtil(QObject *parent = nullptr);
    ~FFCaptureUtil();
    void initialize();
    void startCapture();
    void stopCapture();
private:
    void initCoreComponents();
    void initCaptureContext();
    void registerMetaTypes();
private:
    FFCapWindow* m_capWindow = nullptr;
    FFCaptureContext* m_captureCtx = nullptr;
    FFThreadPool* m_threadPool = nullptr;


};

#endif // FFCAPTUREUTIL_H
