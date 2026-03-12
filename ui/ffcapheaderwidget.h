#ifndef FFCAPHEADERWIDGET_H
#define FFCAPHEADERWIDGET_H

#include <QWidget>
QT_BEGIN_NAMESPACE
namespace Ui {class FFCapHeaderWidget;}
QT_END_NAMESPACE
//该头文件定义了一个FFCapHeaderWidget的C++类,他继承自Qt框架中的QWidget。
//这个类名和成员函数来看,这类实现了一个可拖动且可自定义标题栏部件，通常用于实现无边框窗口,支持鼠标拖拽移动窗口
//以及在窗口边缘拖拽改变窗口大小
class FFCapHeaderWidget:public QWidget
{
    Q_OBJECT
//启用Qt的元对象系统特性(如信号和槽、属性等)
public:
    FFCapHeaderWidget(QWidget *parent = nullptr);
    ~FFCapHeaderWidget();
protected:
    void paintEvent(QPaintEvent *event) override;
    //绘制事件。通常在这里绘制标题栏的背景边框、标题文件或者自定义图标等
    void mousePressEvent(QMouseEvent *event) override;
    //鼠标按下事件,用于记录按下位置、判断调整方向、初始化拖拽或大小调整操作
    void mouseMoveEvent(QMouseEvent* event) override;
    //鼠标移动事件。根据按下的状态实时更新窗口的位置或大小
    void mouseReleaseEvent(QMouseEvent* event)override;
    //鼠标释放事件，结束拖拽或者大小调整操作

private:
    int getResizeDirection(const QPoint&pos);
    //私有辅助函数,根据鼠标在部位上的pos返回一个整数值，表示当前应该启用哪种窗口
    //调整方向
private:
    QPoint dragPos;//用于记录鼠标按下时的全局位置，用于计算拖动偏移量
    QRect initialGeometry;//记录窗口在开始拖动/调整前的初始几何形状
    int resizeDir;//调整大小的方向
};

#endif // FFCAPHEADERWIDGET_H
