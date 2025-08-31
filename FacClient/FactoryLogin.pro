QT       += core gui network charts

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    MainWindow_main.cpp \
    chatroom.cpp \
    clientcore.cpp \
    filereceiver.cpp \
    filesender.cpp \
    handlers.cpp \
    main.cpp \
    page_mine.cpp \
    pagedevice.cpp \
    pageorder.cpp \
    register.cpp \
    screensharereceiver.cpp \
    screensharesender.cpp \
    session.cpp \
    widget.cpp

HEADERS += \
    ../../Downloads/FacExpSys-main/FacExpSys-main/Server/common.h \
    MainWindow_main.h \
    chatroom.h \
    clientcore.h \
    common.h \
    filereceiver.h \
    filesender.h \
    handlers.h \
    page_mine.h \
    pagedevice.h \
    pageorder.h \
    register.h \
    screensharereceiver.h \
    screensharesender.h \
    session.h \
    widget.h

FORMS += \
    MainWindow_main.ui \
    chatroom.ui \
    clientcore.ui \
    page_mine.ui \
    pagedevice.ui \
    pageorder.ui \
    register.ui \
    widget.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    icons.qrc
