#-------------------------------------------------
#
# Project created by QtCreator 2015-01-05T21:10:05
#
#-------------------------------------------------

QT       += core gui
QT       += network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = download
TEMPLATE = app


SOURCES += main.cpp\
        download_mainwindow.cpp \
    addnew.cpp \
    myThread.cpp \
    settings.cpp \
    network_thread.cpp \
    version.cpp \
    bt_list.cpp \
    start_task_thread.cpp \
    cloud.cpp \
    cloud_upload.cpp \
    uilog.cpp \
    login_thread.cpp \
    local_download.cpp

HEADERS  += download_mainwindow.h \
    addnew.h \
    myThread.h \
    settings.h \
    network_thread.h \
    version.h \
    bt_list.h \
    start_task_thread.h \
    cloud.h \
    cloud_upload.h \
    uilog.h \
    ../lib/apx_hftsc_api.h \
    ../lib/uci.h \
    login_thread.h \
    ../client/apx_proto_ctl.h \
    local_download.h

INCLUDEPATH += ../lib

FORMS    += download_mainwindow.ui \
    addnew.ui \
    addnew.ui \
    settings.ui \
    version.ui \
    bt_list.ui \
    cloud.ui \
    cloud_upload.ui \
    local_download.ui

RESOURCES += \
    qss.qrc \
    image.qrc

LIBS += ../lib/libhftsc.a
LIBS += ../lib/libuci.so
LIBS += ../lib/libaria2.so
LIBS += /usr/lib/gcc/x86_64-linux-gnu/4.8/libstdc++.so
LIBS += /usr/lib/x86_64-linux-gnu/libcurl.so
LIBS += /usr/lib/x86_64-linux-gnu/libssl.so
LIBS += /usr/lib/x86_64-linux-gnu/libcrypto.so
LIBS += /usr/lib/x86_64-linux-gnu/libdl.so
