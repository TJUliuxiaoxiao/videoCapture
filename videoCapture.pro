QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
INCLUDEPATH += $$PWD/ffmpeg/include
LIBS += -L$$PWD/ffmpeg/lib \
        -lavcodec \
        -lavformat \
        -lavutil \
        -lavdevice \
        -lswscale \
        -lswresample
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
    event/ffevent.cpp \
    filter/ffafilter.cpp \
    filter/ffvfilter.cpp \
    main.cpp \
    muxer/ffmuxer.cpp \
    opengl/ffglrenderwidget.cpp \
    queue/ffaframequeue.cpp \
    queue/ffapacketqueue.cpp \
    queue/ffeventqueue.cpp \
    queue/ffpacket.cpp \
    queue/ffvframequeue.cpp \
    queue/ffvpacketqueue.cpp \
    resampler/ffaresampler.cpp \
    resampler/ffvresampler.cpp \
    thread/ffadecoderthread.cpp \
    thread/ffthread.cpp \
    thread/ffthreadpool.cpp \
    ui/ffcapheaderwidget.cpp \
    ui/ffcapwindow.cpp

HEADERS += \
    capture/ffcapturecontext.h \
    capture/ffcaptureutil.h \
    clock/ffglobalclock.h \
    decoder/ffadecoder.h \
    decoder/ffvdecoder.h \
    demuxer/ffdemuxer.h \
    encoder/ffaencoder.h \
    encoder/ffvencoder.h \
    event/ffevent.h \
    filter/ffafilter.h \
    filter/ffvfilter.h \
    muxer/ffmuxer.h \
    opengl/ffglrenderwidget.h \
    queue/ffaframequeue.h \
    queue/ffapacketqueue.h \
    queue/ffeventqueue.h \
    queue/ffpacket.h \
    queue/ffvframequeue.h \
    queue/ffvpacketqueue.h \
    resampler/ffaresampler.h \
    resampler/ffvresampler.h \
    thread/ffadecoderthread.h \
    thread/ffthread.h \
    thread/ffthreadpool.h \
    ui/ffcapheaderwidget.h \
    ui/ffcapwindow.h

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
