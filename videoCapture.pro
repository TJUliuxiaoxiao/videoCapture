QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    capture/ffcapturecontext.cpp \
    capture/ffcaptureutil.cpp \
    main.cpp \
    opengl/ffglrenderwidget.cpp \
    thread/ffthread.cpp \
    thread/ffthreadpool.cpp \
    ui/ffcapheaderwidget.cpp \
    ui/ffcapwindow.cpp

HEADERS += \
    capture/ffcapturecontext.h \
    capture/ffcaptureutil.h \
    opengl/ffglrenderwidget.h \
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
