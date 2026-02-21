#ifndef FFCAPWINDOW_H
#define FFCAPWINDOW_H

#include <QWidget>
#include <QMouseEvent>
#include <mutex>
QT_BEGIN_NAMESPACE
namespace Ui {
class FFCapWindow;
}
QT_END_NAMESPACE


#ifndef RESIZE_DIR
#define RESIZE_DIR
enum ResizeDir{
    NODIR,
    TOP,
    BOTTOM,
    LEFT,
    RIGHT,
    BOTTOM_LEFT,
    BOTTOM_RIGHT,
    TOP_LEFT,
    TOP_RIGHT
};
#endif

const int BORDER_WIDTH = 5;
class AVFrame;
class FFRenderWidget;
class FFVFilter;
class FFVRender;
class FFCaptureContext;
class FFEventQueue;


class FFCapWindow : public QWidget
{
    Q_OBJECT

public:
    explicit FFCapWindow(QWidget *parent = nullptr);
    virtual ~FFCapWindow() override;

    bool getMaxSizeFlag();
    void setVFilter(FFVFilter* vFilter);
    void setVRender(FFVRender* vRender_);
    void init(FFCaptureContext* captureCtx_);
    void adjustStackNumber(int type);
public slots:
    void sendScreenFrame(AVFrame* frame);
    void sendCameraFrame(AVFrame* frame);
    void sendVideoFrame(AVFrame* frame);
    bool peekVideoReady();
    void getOverlayPos(int *x,int *y,int *w,int*h,int type);
    const std::vector<int>& getOverlayNumber();
    void setCaptureProcessTime(int64_t seconds);
protected:
    void paintEvent(QPaintEvent*event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent*event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    QPoint dragPos;
    QRect initialGeometry;
    enum ResizeDir resizeDir;
    bool pauseFlag = true;
    FFRenderWidget* cameraWidget = nullptr;
    FFRenderWidget* screenWidget = nullptr;
    FFRenderWidget* videoWidget = nullptr;
    FFVRender* vRender = nullptr;
    bool maxSizeFlag = false;
    FFVFilter* vFilter = nullptr;
    QSet<FFRenderWidget*>renderWidgetSet;
    FFCaptureContext* captureCtx = nullptr;
    FFEventQueue* evQueue = nullptr;
    std::mutex mutex;
    std::vector<int> overlayNumbers;
    bool startFlag = false;
private:
    Ui::FFCapWindow *ui;

};

#endif // FFCAPWINDOW_H
