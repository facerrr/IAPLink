QT       += core gui serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets core5compat

CONFIG += c++17

RC_ICONS = serial.ico

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    config.cpp \
    iapmodule.cpp \
    main.cpp \
    mainwindow.cpp \
    mserialport.cpp \
    recvthread.cpp \
    uart.cpp \
    vplaintextedit.cpp

HEADERS += \
    config.h \
    iapmodule.h \
    mainwindow.h \
    mserialport.h \
    recvthread.h \
    uart.h \
    vplaintextedit.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
