QT += core gui network sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

TARGET = RemoteSupportServer
CONFIG += console
CONFIG -= app_bundle
# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    clientsession.cpp \
    database.cpp \
    deviceproxy.cpp \
    filerouter.cpp \
    main.cpp \
    mediarelay.cpp \
    messagerouter.cpp \
    servercore.cpp \
    workorder.cpp \
    workorderdao.cpp \
    workordermanager.cpp

HEADERS += \
    clientsession.h \
    common.h \
    database.h \
    deviceproxy.h \
    filerouter.h \
    mediarelay.h \
    messagerouter.h \
    servercore.h \
    workorder.h \
    workorderdao.h \
    workordermanager.h

FORMS +=

TRANSLATIONS += \
    Server_zh_CN.ts
CONFIG += lrelease
CONFIG += embed_translations

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
