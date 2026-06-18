QT       += core gui serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets core5compat

CONFIG += c++17

# Ensure direct Chinese literals in source are decoded consistently.
win32-msvc*:QMAKE_CXXFLAGS += /utf-8
win32-g++:QMAKE_CXXFLAGS += -finput-charset=UTF-8 -fexec-charset=UTF-8

RC_ICONS = serial.ico

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    config.cpp \
    iapmodule.cpp \
    main.cpp \
    mainwindow.cpp \
    modelcfgdialog.cpp \
    mserialport.cpp \
    recvmodule.cpp \
    uart.cpp \
    vplaintextedit.cpp

HEADERS += \
    config.h \
    iapmodule.h \
    mainwindow.h \
    modelcfgdialog.h \
    mserialport.h \
    recvmodule.h \
    uart.h \
    vplaintextedit.h


FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
