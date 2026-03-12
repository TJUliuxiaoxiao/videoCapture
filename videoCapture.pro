QT       += core gui openglwidgets multimedia
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
CONFIG += c++17
# CONFIG += rtti
INCLUDEPATH += $$PWD/ffmpeg/include
INCLUDEPATH += $$PWD/3rdparty/opencv/include
INCLUDEPATH += $$PWD/ui
INCLUDEPATH += $$PWD/opengl
LIBS += -L$$PWD/ffmpeg/lib \
        -lavcodec \
        -lavformat \
        -lavutil \
        -lavdevice \
        -lswscale \
        -lswresample\
        -lavfilter

LIBS +=$$PWD/3rdparty/opencv/lib/libopencv_*.a
# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    capture/ffcapturecontext.cpp \
    capture/ffcaptureutil.cpp \
    clock/ffglobalclock.cpp \
    decoder/ffadecoder.cpp \
    decoder/ffvdecoder.cpp \
    demuxer/ffdemuxer.cpp \
    encoder/ffaencoder.cpp \
    encoder/ffvencoder.cpp \
    event/ffbeautyevent.cpp \
    event/ffcaptureprocessevent.cpp \
    event/ffclosesourceevent.cpp \
    event/ffendevent.cpp \
    event/ffevent.cpp \
    event/ffeventloop.cpp \
    event/ffopensourceevent.cpp \
    event/ffpauseevent.cpp \
    # event/ffprocessevent.cpp \
    # event/ffreadyevent.cpp \
    event/ffstartevent.cpp \
    event/ffstopevent.cpp \
    event/ffvolumeevent.cpp \
    filter/ffafilter.cpp \
    filter/ffvfilter.cpp \
    main.cpp \
    muxer/ffmuxer.cpp \
    opencv/fffacedetector.cpp \
    opencv/ffoverlayprocessor.cpp \
    opencv/ffvideoadapter.cpp \
    opengl/ffglrenderwidget.cpp \
    queue/ffaframequeue.cpp \
    queue/ffapacketqueue.cpp \
    queue/ffeventqueue.cpp \
    queue/ffvframequeue.cpp \
    queue/ffvpacketqueue.cpp \
    # render/ffarender.cpp \
    render/ffvrender.cpp \
    resampler/ffaresampler.cpp \
    resampler/ffvresampler.cpp \
    sonic/sonic.c \
    thread/ffadecoderthread.cpp \
    thread/ffaencoderthread.cpp \
    thread/ffafilterthread.cpp \
    # thread/ffamuxerthread.cpp \
    thread/ffdemuxerthread.cpp \
    thread/ffmuxerthread.cpp \
    thread/ffthread.cpp \
    thread/ffthreadpool.cpp \
    thread/ffvdecoderthread.cpp \
    thread/ffvencoderthread.cpp \
    thread/ffvfilterthread.cpp \
    # thread/ffvmuxerthread.cpp \
    timer/fftimer.cpp \
    ui/ffcapheaderwidget.cpp \
    ui/ffcapwindow.cpp \
    ui/ffrenderwidget.cpp

HEADERS += \
    capture/ffcapturecontext.h \
    capture/ffcaptureutil.h \
    clock/ffglobalclock.h \
    decoder/ffadecoder.h \
    decoder/ffvdecoder.h \
    demuxer/ffdemuxer.h \
    encoder/ffaencoder.h \
    encoder/ffvencoder.h \
    event/ffbeautyevent.h \
    event/ffcaptureprocessevent.h \
    event/ffclosesourceevent.h \
    event/ffendevent.h \
    event/ffevent.h \
    event/ffeventloop.h \
    event/ffopensourceevent.h \
    event/ffpauseevent.h \
    # event/ffprocessevent.h \
    # event/ffreadyevent.h \
    event/ffstartevent.h \
    event/ffstopevent.h \
    event/ffvolumeevent.h \
    filter/ffafilter.h \
    filter/ffvfilter.h \
    muxer/ffmuxer.h \
    opencv/fffacedetector.h \
    opencv/ffoverlayprocessor.h \
    opencv/ffvideoadapter.h \
    opengl/ffglrenderwidget.h \
    # player/ffplayercontext.h \
    queue/ffaframequeue.h \
    queue/ffapacketqueue.h \
    queue/ffeventqueue.h \
    queue/ffpacket.h \
    queue/ffvframequeue.h \
    queue/ffvpacketqueue.h \
    # render/ffarender.h \
    render/ffvrender.h \
    resampler/ffaresampler.h \
    resampler/ffvresampler.h \
    sonic/sonic.h \
    thread/ffadecoderthread.h \
    thread/ffaencoderthread.h \
    thread/ffafilterthread.h \
    # thread/ffamuxerthread.h \
    thread/ffdemuxerthread.h \
    thread/ffmuxerthread.h \
    thread/ffthread.h \
    thread/ffthreadpool.h \
    thread/ffvdecoderthread.h \
    thread/ffvencoderthread.h \
    thread/ffvfilterthread.h \
    # thread/ffvmuxerthread.h \
    timer/fftimer.h \
    ui/ffcapheaderwidget.h \
    ui/ffcapwindow.h \
    ui/ffrenderwidget.h

FORMS += \
    ui/ffcapwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    image.qrc

DISTFILES += \
    .gitignore
