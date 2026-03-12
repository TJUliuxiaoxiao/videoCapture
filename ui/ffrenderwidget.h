#ifndef FFRENDERWIDGET_H
#define FFRENDERWIDGET_H

#include <QWidget>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QFrame>
#include <QVBoxLayout>
#include <QHash>
#include <mutex>
#include "opengl/ffglrenderwidget.h"

class FFVFilter;
class FFCapWindow;

class FFRenderWidget : public QWidget
{
    Q_OBJECT
public:
    explicit FFRenderWidget(FFVFilter* vFilter_,int type_,QWidget *parent = nullptr);//渲染窗口
    virtual ~FFRenderWidget() override;
    bool getMoveFlag();
    void setMoveFlag(bool flag);
    //鼠标移动

    void setFilterPos(int x,int y,int w,int h);//filter的位置和尺寸
    void getOverlayPos(int *x,int *y,int*w,int *h);//叠加层的位置
    void calcPos();//计算位置(可能根据比例重新计算内部OpenGL控件的视口位)

    void setStackNumber(int number);//堆叠顺序中的索引,例如多个渲染叠加时的Z序
    int getStackNumber();
    int getType();
public slots:
    //两个重载的槽函数,用于接收YUV视频帧数据,内部glRenderWidget的同名方法
    void setYUVData(AVFrame* frame);
    void setYUVData(uint8_t *yData,uint8_t *uData,
        uint8_t *vData,int width,int height);
public:
    //四个比例值,可能用于相对于父控件或窗口的比例定位
    double xRatio;
    double yRatio;
    double widthRatio;
    double heightRatio;
protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent *event) override;
    //鼠标事件:用于实现控件的拖动,调整大小等功能
    //绘制事件:paintEvent可能用于绘制边框、焦点提示等
    //进入/离开事件,用于改变外观或显示控制按钮
private:
    int getResizeDirection(const QPoint &pos);
    FFGLRenderWidget* glRenderWidget = nullptr;//内部openGL渲染控件
private:
    bool isFocus = false;
    int type = 0;//控件类型
    QPoint dragPos;//鼠标拖动时的起始点
    QRect initialGeometry;//拖动/调整大小前的初始几何坐标
    int resizeDir;//调整大小的方向
    bool startFlag = true;//可能用于首次初始化
    bool isFoucs = false;//是否获得焦点
    bool moveFlag;//是否允许移动
    QVBoxLayout* layout = nullptr;//垂直布局
    FFVFilter* vFilter = nullptr;//视频滤镜对象
    int overlayX;
    int overlayY;
    int overlayW;
    int overlayH;//叠加层位置尺寸

    int stackNumber =0;//堆叠序号
    std::mutex mutex;//线程同步互斥量(可能用于保护共享数据)
    FFCapWindow* capWindow = nullptr;//

};

#endif // FFRENDERWIDGET_H
