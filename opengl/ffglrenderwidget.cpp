#include "ffglrenderwidget.h"
#include <QMouseEvent>

//顶点数据(位置(x,y)+纹理坐标(u,v))
//静态常量初始化
const QVector<float> FFGLRenderWidget::vertices = {
    -1.0f,1.0f,0.0f,0.0f,//顶点0,左上(-1,1),纹理坐标左下
    -1.0f,-1.0f,0.0f,1.0f,//顶点1,左下(-1,-1),纹理坐标左上
    1.0f,-1.0f,1.0f,1.0f,//顶点2,右下(1,-1),纹理坐标右上
    1.0f,1.0f,1.0f,0.0f//顶点3,右上(1,1),
};
/*纹理坐标（Texture Coordinates）是
 * 图形编程中用来把一张图片（纹理）贴到多边形（比如矩形）上的“位置记号”。*/

const QVector<unsigned int> FFGLRenderWidget::indices = {0,1,2,3};

FFGLRenderWidget::FFGLRenderWidget(QWidget *parent):QOpenGLWidget(parent) {
    setMinimumSize(100,100);
    //确保每个实例使用独立的着色器程序
    shaderProgram = new QOpenGLShaderProgram(this);

}

FFGLRenderWidget::~FFGLRenderWidget()
{
    makeCurrent();//确保当前线程绑定了有效的OpenGL上下文,这样才能安全地调用OpenGL删除函数和

    glDeleteVertexArrays(1,&vao);
    glDeleteBuffers(1,&vboVertice);
    glDeleteBuffers(1,&ebo);

    glDeleteTextures(1,&yTexture);
    glDeleteTextures(1,&uTexture);
    glDeleteTextures(1,&vTexture);

    //清理着色器程序
    delete shaderProgram;

    doneCurrent();//在清理完成后解除上下文绑定,避免影响后续操作
    //Qt可能在析构时就已经切换了上下文，r如果不调用makeCurrent,删除操作可能无效甚至导致崩溃
}

void FFGLRenderWidget::setAspect(float aspect_)
{
    aspect = aspect_;
}

void FFGLRenderWidget::setBlackScreen()
{
    makeCurrent();
    //配置纹理参数并初始化为黑色YUV数据
    uint8_t yData = 0;//Y分量全0
    uint8_t uData = 128;//U分量全为128
    uint8_t vData = 128;//v分量全为128

    //初始化Y纹理
    //1.绑定纹理
    glBindTexture(GL_TEXTURE_2D,yTexture);//将当前活动的2D纹理单元绑定到yTexture,后续操作都针对此单元
    /*GL_TEXTURE_2D：目标纹理类型。*/

    //2.上传纹理图像:创建了一个1X1大小的纹理,每个纹理单元的值就是yData(0)
    glTexImage2D(GL_TEXTURE_2D,0,GL_RED,1,1,0,
                  GL_RED,GL_UNSIGNED_BYTE,&yData);
    /*参数解释：
    GL_TEXTURE_2D：目标纹理类型。
    0：Mipmap 级别（0 表示基本级别）。
    GL_RED：纹理内部格式，表示每个纹素只存储一个红色通道（对应 Y/U/V 单通道数据）。
    1, 1：纹理宽度和高度（1x1 像素）。
    0：边框（旧版 OpenGL 参数，必须为 0）。
    GL_RED：像素数据格式，表示提供的像素数据是红色通道。
    GL_UNSIGNED_BYTE：像素数据类型（8 位无符号整数）。
    &yData：指向像素数据的指针（这里是一个单字节）。*/
    /*为什么用 1x1 纹理？
    因为黑屏只需要单一颜色，用最小的纹理可以节省显存，
    且不影响后续更新为实际视频帧（届时会用 glTexSubImage2D
    替换部分区域或重新上传全尺寸纹理）。*/

    //3.设置纹理参数
    /*GL_TEXTURE_MIN_FILTER / GL_TEXTURE_MAG_FILTER：纹理缩小/放大时的滤波方式。
     * GL_LINEAR 表示线性插值，使颜色平滑。对于 1x1 纹理，无论用何种滤波结果都一样。
    GL_TEXTURE_WRAP_S / GL_TEXTURE_WRAP_T：纹理坐标超出 [0,1] 范围时的处理方式。
    GL_CLAMP_TO_EDGE 表示将坐标约束在边界，防止边缘采样出现问题（例如当坐标正好落在边界时仍取边缘纹素）。 */
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);

    //初始化U纹理
    glBindTexture(GL_TEXTURE_2D,uTexture);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RED,1,1,0,
                 GL_RED,GL_UNSIGNED_BYTE,&uData);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);

    //初始化
    glBindTexture(GL_TEXTURE_2D,vTexture);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RED,1,1,0,
                    GL_RED,GL_UNSIGNED_BYTE,&vData);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);

    update();//请求Qt尽快重绘控件,即触发paintGL调用,使得新设置的黑色纹理生效
    doneCurrent();//释放OpenGL上下文绑定,避免影响后续其他操作
}

void FFGLRenderWidget::setKeepRario(bool flag)
{
    keepRatio = flag;
}

void FFGLRenderWidget::setYUVData(uint8_t *yData,
                                  uint8_t *uData,
                                  uint8_t *vData,
                                  int width,
                                  int height)
/*参数:分别指向YUV三个平面的数据指针,以及视频帧的宽度和高度要素*/
/*用于将新的YUV视频帧数据上传大OpenGL纹理.
 并触发控件重绘。*/
/*分别指向YUV三个平面的数据指针,以及视频帧的宽度和高度*/
/*将给定的YUV数据上传到对应的OpenGL纹理中，然后请求重绘控件,最后释放传入的数据内存*/
{
    makeCurrent();
    //确保当前线程拥有该QOpenGLWidget的OpenGL上下文,以便安全调用OpenGL函数
    //因为该函数可能从非GUI线程调用(例如视频解码线程),所以必须手动激活上下文

    //上传Y分量纹理
    glBindTexture(GL_TEXTURE_2D,yTexture);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RED,
                 width,height,0,GL_RED,GL_UNSIGNED_BYTE,yData);
    //绑定Y纹理对象(yTexture),此调用会为Y纹理重新分配显存,并拷贝yData指向的数据。

    //上传UV分量(宽高各减半)
    int uvWidth = width/2;
    int uvHeight = height/2;
    //假设输入的YUV数据格式为YUV420P,在YUV420P中,U和V平面的宽高均为Y平面的一半
    //因此为U、V纹理分配的大小为uvWidth x uvHeight
    glBindTexture(GL_TEXTURE_2D,uTexture);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RED,uvWidth,uvHeight,0,GL_RED,GL_UNSIGNED_BYTE,uData);
    glBindTexture(GL_TEXTURE_2D,vTexture);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RED,uvWidth,uvHeight,0,GL_RED,GL_UNSIGNED_BYTE,vData);
    //同样使用GL_RED内部格式和GL_UNSIGNED_BYTE数据类型

    update();
    //重绘,他会向Qt事件系统发送一个重绘请求.
    //随后，Qt会在合适的时机调用paintGL(),从而将
    //新上传的纹理数据显示到屏幕上

    doneCurrent();
    delete[] yData;
    delete[] uData;
    delete[] vData;
    //释放
}

void FFGLRenderWidget::setYUVData(AVFrame *frame)
{
    makeCurrent();
    int width = frame->width;
    int height = frame->height;
    //保存视频帧的原始宽度和高度,用于后续纹理创建
    //上传Y分量
    glBindTexture(GL_TEXTURE_2D,yTexture);//绑定Y纹理对象
    glPixelStorei(GL_UNPACK_ROW_LENGTH,frame->linesize[0]/sizeof(uint8_t));
    //用于处理FFmpeg中可能存在的行对齐问题
    /*frame->linesize[0]是Y平面一行数据的实际字数(可能包含填充字节
    大于width*sizeof(uint8_t)
    除以sizeof(uint8_t)得到每行的像素数(因为每个像素占1字节).
    设置GL_UNPACK_ROW_LENGTH告诉OpenGL源数据中一行有多少个像素
    这样即使每行末尾有额外填充,OpenGL也能正确跳过他们，只读取前width个像素
    */
    /*glPixelStorei用于修改OpenGL的像素存储模式,这些模式控制OpenGL
     如何从客户端内存读取像素数据传输到显存。
    将行字节数转换为行像素数，赋值给 GL_UNPACK_ROW_LENGTH*/


    glTexImage2D(GL_TEXTURE_2D,0,GL_RED,width,height,0,
                 GL_RED,GL_UNSIGNED_BYTE,frame->data[0]);//Y分量
    //上传数据,与之前版本类似,使用GL_RED内部格式和像素格式,
    //GL_UNSIGNED_BYTE类型,数据指针为frame->data[0].
    //恢复默认行对齐,上传完成后将GL_UNPACK_ROW_LENGTH重置为0
    //恢复默认行为,即每行像素数等于纹理宽度,避免影响后续其他OpenGL操作
    glPixelStorei(GL_UNPACK_ROW_LENGTH,0);

    //上传UV分量(宽高各减一半)
    int uvWidth = width/2;
    int uvHeight = height/2;

    //上传U分量
    glBindTexture(GL_TEXTURE_2D,uTexture);
    glPixelStorei(GL_UNPACK_ROW_LENGTH,frame->linesize[1]/sizeof(uint8_t));
    glTexImage2D(GL_TEXTURE_2D,0,GL_RED,
                 uvWidth,uvHeight,0,GL_RED,GL_UNSIGNED_BYTE,frame->data[1]);//U分量
    glPixelStorei(GL_UNPACK_ROW_LENGTH,0);

    //上传V分量
    glBindTexture(GL_TEXTURE_2D,vTexture);
    glPixelStorei(GL_UNPACK_ROW_LENGTH,frame->linesize[2]/sizeof(uint8_t));
    glTexImage2D(GL_TEXTURE_2D,0,
                 GL_RED,uvWidth,uvHeight,0,GL_RED,GL_UNSIGNED_BYTE,frame->data[2]);//V分量
    glPixelStorei(GL_UNPACK_ROW_LENGTH,0);

    update();
    doneCurrent();

    av_frame_unref(frame);
    av_frame_free(&frame);
}



void FFGLRenderWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(0.0f,0.0f,0.0f,1.0f);//清空画面

    //初始化缓冲对象
    glGenVertexArrays(1,&vao);
    glGenBuffers(1,&vboVertice);
    glGenBuffers(1,&ebo);

    //配置顶点数据
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER,vboVertice);
    glBufferData(GL_ARRAY_BUFFER,vertices.size()*sizeof(float),vertices.data(),GL_STATIC_DRAW);
    /*target：绑定目标，如GL_ARRAY_BUFFER, GL_ELEMENT_ARRAY_BUFFER等。
    size：字节大小。
    data：数据指针，可为NULL以预留内存。
    usage：提示缓冲区的使用模式，如GL_STATIC_DRAW, GL_DYNAMIC_DRAW, GL_STREAM_DRAW。*/
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,indices.size()*sizeof(unsigned int),indices.data(),GL_STATIC_DRAW);
    //设置顶点属性指针
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)0);
    /*参数详解：
    index = 0：指定要配置的顶点属性位置（对应着色器中的location=0）。
    size = 2：每个顶点属性由几个分量组成（这里是2个，比如x,y坐标）。
    type = GL_FLOAT：数据类型是浮点数。
    normalized = GL_FALSE：是否将整数数据归一化到[0,1]或[-1,1]（这里数据已是浮点，不需要）。
    stride = 4 * sizeof(float)：连续顶点属性之间的字节偏移量。因为每个顶点包含位置(2 floats)和纹理坐标(2 floats)，所以步长是4个float。
    pointer = (void*)0：起始偏移量，表示从缓冲区开始的位置读取数据。*/
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)(2*sizeof(float)));
    glEnableVertexAttribArray(1);

    //解绑VAO
    glBindVertexArray(vao);
    //初始化纹理
    glGenTextures(1,&yTexture);
    glGenTextures(1,&uTexture);
    glGenTextures(1,&vTexture);

    //初始绑定为黑屏
    //配置纹理参数并初始化为黑色YUV数据
    uint8_t yData = 0;
    uint8_t uData = 128;
    uint8_t vData = 128;

    //初始化Y纹理
    glBindTexture(GL_TEXTURE_2D,yTexture);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RED,1,1,0,GL_RED,GL_UNSIGNED_BYTE,&yData);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);

    //u
    glBindTexture(GL_TEXTURE_2D,uTexture);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RED,1,1,0,GL_RED,GL_UNSIGNED_BYTE,&uData);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);

    //v
    glBindTexture(GL_TEXTURE_2D,vTexture);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RED,1,1,0,GL_RED,GL_UNSIGNED_BYTE,&vData);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);

    //加载着色器
    //于在Qt的OpenGL环境中加载顶点着色器和片段着色器，并将它们链接着色器程序
    if(!shaderProgram->addShaderFromSourceFile(QOpenGLShader::Vertex,":/shaderSource/source.vert")
                                ||!shaderProgram->addShaderFromSourceFile(QOpenGLShader::Fragment,":/shaderSource/source.frag")
                                ||!shaderProgram->link()){
        qCritical()<<"Shader error:"<<shaderProgram->log();
    }
}

void FFGLRenderWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT);

    //确保使用当前实例的着色器程序
    shaderProgram->bind();

    //设置视口
    if(keepRatio){
        glViewport(viewportX,viewportY,viewportWidth,viewportHeight);
    }
    //绑定纹理
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,yTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D,uTexture);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D,vTexture);

    shaderProgram->setUniformValue("yTexture",0);
    shaderProgram->setUniformValue("uTexture",1);
    shaderProgram->setUniformValue("vTexture",2);

    //绘制四边形
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLE_FAN,4,GL_UNSIGNED_INT,nullptr);
    glBindVertexArray(0);
    shaderProgram->release();
}

void FFGLRenderWidget::resizeGL(int w, int h)
{
    if(keepRatio){
        const float targetAspect = aspect;
        const float currentAspect = static_cast<float>(w)/h;
        if(currentAspect > targetAspect){
            //窗口过宽,左右加黑边
            viewportWidth = static_cast<int>(h*targetAspect);
            viewportHeight = h;
            viewportX = (w - viewportWidth)/2;
            viewportY = 0;
        }else{
            //窗口过高,上下加黑边
            viewportWidth = w;
            viewportHeight = static_cast<int>(w/targetAspect);
            viewportX = 0;
            viewportY = (h - viewportHeight)/2;
        }
    }
}

void FFGLRenderWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton){
        emit mouseDoubleClick();
    }
    QOpenGLWidget::mouseDoubleClickEvent(event);
}

void FFGLRenderWidget::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton){
        emit mouseClick();
    }
    QOpenGLWidget::mousePressEvent(event);
}









