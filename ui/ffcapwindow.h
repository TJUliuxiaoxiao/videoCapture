#ifndef FFCAPWINDOW_H
#define FFCAPWINDOW_H

#include <QWidget>
#include <QMouseEvent>
#include <mutex>

class FFCapHeaderWidget;
class FFRenderWidget;
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
    void setVFilter(FFVFilter* vFilter_);
    void setVRender(FFVRender* vRender_);
    void init(FFCaptureContext* captureCtx_);
    void adjustStackNumber(int type);
public slots:
    void sendScreenFrame(AVFrame* frame);
    void sendCameraFrame(AVFrame* frame);
    void sendVideoFrame(AVFrame* frame);
    bool peekVideoReady();
    void getOverlayPos(int *x,int *y,int *w,int*h,int type);
    const std::vector<int>& getOverlayNumbers();
    void setCaptureProcessTime(int64_t seconds);
protected:
    void paintEvent(QPaintEvent*event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent*event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;


private slots:
    void on_closeBtn_clicked();

    void on_maximizeWindow_clicked();

    void on_minimizeWindow_clicked();

    void on_StartBtn_clicked();

    void on_cameraCheckBox_toggled(bool checked);

    void on_cameraComboBox_currentIndexChanged(int index);

    void on_screenCheckBox_toggled(bool checked);

    void on_videoCheckBox_toggled(bool checked);

    void on_microphoneCheckBox_toggled(bool checked);

    void on_desktopAudioCheckBox_toggled(bool checked);

    void on_StopBtn_clicked();

    void on_beautyCheckBox_toggled(bool checked);

    void on_microphoneSlider_valueChanged(int value);

    void on_smoothSlider_valueChanged(int value);

    void on_whiteSlider_valueChanged(int value);

    void on_audiovolumeSlider_valueChanged(int value);

private:
    enum ResizeDir getResizeDirection(const QPoint &pos);
    void resizeChildWidget(FFRenderWidget* pChildWidget);
    void calcChildWidget(FFRenderWidget* pChildWidget);
    void setStackOrder(FFRenderWidget* widget1,
                       FFRenderWidget* widget2);
    void getCameraOverlayPos(int *x,int *y,int *w,int *h);
    void getScreenOverlayPos(int *x,int *y,int *w,int *h);
    void getVideoOverlayPos(int *x,int *y,int *w,int *h);
    void changeUIState(QWidget *widget);
    void setPauseUI(bool pause);
private:
    QPoint dragPos;
    QRect initialGeometry;
    enum ResizeDir resizeDir;

    FFRenderWidget* cameraWidget = nullptr;//摄像头视频渲染控件
    FFRenderWidget* screenWidget = nullptr;//屏幕采集视频渲染控件
    FFRenderWidget* videoWidget = nullptr;//视频文件渲染控件

    FFVRender* vRender = nullptr;//视频渲染对象指针（由外部传入）
    bool maxSizeFlag = false;
    FFVFilter* vFilter = nullptr;//视频滤镜对象指针（由外部传入）

    QSet<FFRenderWidget*>renderWidgetSet;
    //存储所有当前显示的渲染控件，用于统一调整布局

    FFCaptureContext* captureCtx = nullptr;//核心处理上下文指针（由外部传入）
    FFEventQueue* evQueue = nullptr;
    std::mutex mutex;
    std::vector<int> overlayNumbers;//记录三个视频源的叠加顺序（用于画中画）
    bool startFlag = false;//是否已经开始录制/推流
    bool pauseFlag = true;//是否处于暂停状态（录制/推流暂停）
private:
    Ui::FFCapWindow *ui;

};

#endif // FFCAPWINDOW_H
