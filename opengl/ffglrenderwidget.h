#ifndef FFGLRENDERWIDGET_H
#define FFGLRENDERWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLShaderProgram>
#include <QVector>
#include <QOpenGLTexture>
/*Qt框架中用于引入QOpenGLTexture类，提供了对OpenGL纹理对象的跨平台封装*/
#include <QOpenGLFunctions_4_5_Core>
extern "C"{
#include "libavformat/avformat.h"
}

/*FFGLRenderWidget是一个基于OpenGL的视频渲染控件,专门用于显示YUV数据,
 利用OpenGL纹理上传三个平面,通过着色器将YUV转换为RGB并
渲染到屏幕上,控件支持1.动态更新视频帧
2.保持原始宽高比(通过计算视口)
3.相应鼠标点击事件(发出信号，便于外部控制，如播放/暂停)
4.使用Qt的OpenGL封装,简化了跨平台和OpenGL上下文管理*/
/*用于视频播放器,视频编辑软件或者任何需要高效渲染YUV视频流的Qt应用程序*/
class FFGLRenderWidget : public QOpenGLWidget,protected QOpenGLFunctions_4_5_Core
{
    Q_OBJECT
signals:
    void mouseDoubleClick();
    void mouseClick();
public:
    explicit FFGLRenderWidget(QWidget *parent = nullptr);
    ~FFGLRenderWidget() override;
    void setAspect(float aspect_);//设置视频的原始宽高比,用于计算视口
    void setBlackScreen();//将当前显示清为黑色
    void setKeepRario(bool flag);//设置是否保持视屏原始宽高比例
    //若为true,窗口缩放时视频居中显示，周围填充黑边；若为false,视频拉伸填充整个窗口
//公共槽函数
public slots:
    void setYUVData(uint8_t *yData,uint8_t *uData,uint8_t *vData,int width,int height);
    //直接接收三个平面的数据指针和视频尺寸,用于更新纹理并触发重绘
    void setYUVData(AVFrame *frame);
    //接收FFmpeg的AVFrame结构体，内部提取YUV数据后调用第一个版本,可以直接与FFmpeg解码流程集成
//受保护的重写函数
protected:
    void initializeGL() override;
    //初始化openGL，创建着色器程序,生成VAO、VBO、EBO、并上传顶点数据和索引数据
    //生成Y,U,V三个纹理，并设置纹理参数
    void paintGL() override;
    //绑定三个纹理并上传新数据,通过glTexSubImage2D或类似方式,使用着色器程序并绘制矩形
    void resizeGL(int w,int h) override;
    //窗口大小改变时调用
    void mouseDoubleClickEvent(QMouseEvent* event) override;//双击时发送doubleClick信号
    void mousePressEvent(QMouseEvent *event) override;//单击时发送click信号
    void initShaders();//辅助函数，用于加载、编译和链接着色器(顶点着色器和片段着色器),
    //YUV转RGB的算法通常写在片段着色器中。

private:
    static const QVector<float> vertices;//静态常量,存储顶点位置和纹理坐标
    static const QVector<unsigned int> indices;//静态变量,存储绘制矩形的索引
    GLuint vao = 0,vboVertice = 0,ebo = 0;
    //顶点数组对象,顶点缓冲对象,元素缓冲对象
    GLuint yTexture;
    GLuint uTexture;
    GLuint vTexture;//分别对应YUV平面的纹理ID
    QOpenGLShaderProgram* shaderProgram = nullptr;
    /*QOpenGLShaderProgram 是 Qt 对 OpenGL
     *  着色器程序（glCreateProgram、glAttachShader、glLinkProgram 等）的
     *  高层次封装。它简化了着色器的创建、编译、链接和绑定过程。*/

    //着色器程序对象指针
    float aspect = 16.0f/9;//视频的原始宽高比,默认16:9
    int viewportX = 0;
    int viewportY = 0;
    int viewportWidth = 0;
    int viewportHeight = 0;//在保持宽高比时，实际渲染区域在窗口中的位置和大小
    bool keepRatio = false;//是否保持宽高比的标志


};

#endif // FFGLRENDERWIDGET_H
