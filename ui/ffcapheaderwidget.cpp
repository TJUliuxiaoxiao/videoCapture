#include "ffcapheaderwidget.h"
#include <QPainter>
#include <QMouseEvent>
#include "ffcapwindow.h"
FFCapHeaderWidget::FFCapHeaderWidget(QWidget*parent):QWidget(parent){
    this->setWindowFlags(windowFlags() | Qt::FramelessWindowHint);//无边框，通过按位或|操作，将该标志添加到原有标志集合
    setMouseTracking(true);
    //开启鼠标追踪，使得只要鼠标在部件范围内移动，都会产生mouseMoveEvent;
}//FFCapHeaderWidget会被用作某个无边框窗口的标题栏

FFCapHeaderWidget::~FFCapHeaderWidget()
{

}

void FFCapHeaderWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);//1.调用基类的绘制事件,确保基类默认绘制(如背景、子控件等)
    QPainter painter(this);//2.创建画家对象,绘制目标为当前控件
    painter.setBrush(QBrush(QColor(32,32,32)));//3.设置画刷颜色为深灰色
    painter.fillRect(this->rect(),painter.brush());//4.用画刷填充整个控件区域
}

void FFCapHeaderWidget::mousePressEvent(QMouseEvent *event)
{
    //Qt事件处理函数,当鼠标在FFCapheaderWidget上按下时调用
    if(event->button()==Qt::LeftButton){
        //判断按下的鼠标按钮是否为左键

        dragPos = event->globalPos() - this->parentWidget()->frameGeometry().topLeft();
        //计算鼠标相对于父窗口左上角的偏移量

        initialGeometry = this->parentWidget()->geometry();//记录窗口初始几何信息

        resizeDir = getResizeDirection(event->pos());//根据鼠标在当前控件上的位置event->pos()获取调整方向
        //例如左右上下角等.
        event->accept();
    }
}

void FFCapHeaderWidget::mouseMoveEvent(QMouseEvent *event){
    if(event->buttons()&Qt::LeftButton){
        FFCapWindow* parentWidget = //父窗口
            static_cast<FFCapWindow*>(this->parentWidget());
        if(resizeDir==NODIR&&!parentWidget->getMaxSizeFlag()){
            //当缩放方向为NODIR并且窗口不是最大值,即不缩放，只移动。
            this->parentWidget()->move(event->globalPos()-dragPos);
            //移动窗口
        }
        else{
            QPoint delta = event->globalPos()-(dragPos + initialGeometry.topLeft());
            //delta是当前鼠标位置相当于按下瞬间鼠标位置的偏移量

            QRect newGeometry = initialGeometry;
            switch(resizeDir){
            case TOP:
                newGeometry.setTop(newGeometry.top() + delta.y());
                break;
            case LEFT:
                newGeometry.setLeft(newGeometry.left() + delta.x());
                break;
            case RIGHT:
                newGeometry.setRight(newGeometry.right() + delta.x());
                break;
            case TOP_LEFT:
                newGeometry.setLeft(newGeometry.left() + delta.x());
                newGeometry.setTop(newGeometry.top() + delta.y());
                break;
            case TOP_RIGHT:
                newGeometry.setRight(newGeometry.right() + delta.x());
                newGeometry.setTop(newGeometry.top() + delta.y());
                break;
            default:
                break;
            }

            //最小尺寸限制,对于每种方向，如果新尺寸小于最小值，则强制将对应边界调整到刚好达到最小尺寸的位置
            //并设置canResize = false阻止后续应用新几何
            //以初始几何为基础，保持另一边不动，使得当前边移动使得尺寸等于最小值
            bool canResize = true;
            QWidget* parentWidget = this->parentWidget();
            if(resizeDir == TOP && newGeometry.height()< parentWidget->minimumHeight()){
                newGeometry.setTop(initialGeometry.top() + initialGeometry.height() - parentWidget->minimumHeight());
                canResize = false;
            }else if((resizeDir == LEFT || resizeDir == RIGHT)&&newGeometry.width()<parentWidget->minimumWidth()){
                if(resizeDir == LEFT){
                    newGeometry.setLeft(initialGeometry.right()-parentWidget->minimumWidth());
                }else{
                    newGeometry.setRight(initialGeometry.left()+parentWidget->minimumWidth());
                }
                canResize = false;
            }
            else if(resizeDir == TOP_LEFT){
                if(newGeometry.width()<parentWidget->minimumWidth()){
                    newGeometry.setLeft(initialGeometry.right()-parentWidget->minimumWidth());
                    canResize = false;
                }
                if(newGeometry.height()<parentWidget->minimumHeight()){
                    newGeometry.setTop(initialGeometry.bottom()-parentWidget->minimumHeight());
                    canResize = false;
                }
            }
            else if(resizeDir == TOP_RIGHT){
                if(newGeometry.width()<parentWidget->minimumWidth()){
                    newGeometry.setRight(initialGeometry.left() + parentWidget->minimumWidth());
                    canResize = false;
                }
                if(newGeometry.height() < parentWidget->minimumHeight()){
                    newGeometry.setTop(initialGeometry.bottom()-parentWidget->minimumHeight());
                    canResize = false;
                }
            }

            if(canResize){
                parentWidget->setGeometry(newGeometry);
            }
        }
    }
    else{
        int dir = getResizeDirection(event->pos());
        switch(dir){
        case TOP:
            setCursor(Qt::SizeVerCursor);
            break;
        case LEFT:
            setCursor(Qt::SizeHorCursor);
            break;
        case RIGHT:
            setCursor(Qt::SizeHorCursor);
            break;
        case TOP_LEFT:
            setCursor(Qt::SizeFDiagCursor);
            break;
        case TOP_RIGHT:
            setCursor(Qt::SizeBDiagCursor);
            break;
        default:
            setCursor(Qt::ArrowCursor);
            break;

        }
        QWidget::mouseMoveEvent(event);
    }
}

void FFCapHeaderWidget::mouseReleaseEvent(QMouseEvent* event){
    if(event->button()==Qt::LeftButton){
        resizeDir = NODIR;
        setCursor(Qt::ArrowCursor);
    }
}

int FFCapHeaderWidget::getResizeDirection(const QPoint &pos){
    enum ResizeDir ret = NODIR;
    if(pos.x()<BORDER_WIDTH){
        ret = LEFT;
    }
    else if(pos.x()>width() - BORDER_WIDTH){
        ret = RIGHT;
    }
    else if(pos.y()<BORDER_WIDTH){
        ret = TOP;
    }

    if(pos.x() < BORDER_WIDTH&&pos.y()<BORDER_WIDTH){
        ret = TOP_LEFT;
    }
    else if(pos.x()>width()-BORDER_WIDTH&&pos.y()<BORDER_WIDTH){
        ret = TOP_RIGHT;
    }
    return ret;
}
