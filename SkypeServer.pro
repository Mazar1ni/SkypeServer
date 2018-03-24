#-------------------------------------------------
#
# Project created by QtCreator 2017-09-14T21:42:37
#
#-------------------------------------------------

QT -= gui

CONFIG += c++11 console
CONFIG -= app_bundle

QT       += core network multimedia sql

TARGET = SkypeServer

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# ¬ыбираем директорию сборки исполн€емого файла
# в зависимости от режима сборки проекта
CONFIG(debug, debug|release) {
    DESTDIR = $$OUT_PWD/../../SkypeServer/Debug
} else {
    DESTDIR = $$OUT_PWD/../../SkypeServer/Release
}
# раздел€ем по директори€м все выходные файлы проекта
MOC_DIR = ../Release/common/build/moc
RCC_DIR = ../Release/common/build/rcc
UI_DIR = ../Release/common/build/ui
unix:OBJECTS_DIR = ../Release/common/build/o/unix
win32:OBJECTS_DIR = ../Release/common/build/o/win32
macx:OBJECTS_DIR = ../Release/common/build/o/mac

# раздел€ем по директори€м все выходные файлы проекта
MOC_DIR = ../Debug/common/build/moc
RCC_DIR = ../Debug/common/build/rcc
UI_DIR = ../Debug/common/build/ui
unix:OBJECTS_DIR = ../Debug/common/build/o/unix
win32:OBJECTS_DIR = ../Debug/common/build/o/win32
macx:OBJECTS_DIR = ../Debug/common/build/o/mac

# в зависимости от режима сборки проекта
# запускаем win deploy приложени€ в целевой директории, то есть собираем все dll
CONFIG(debug, debug|release) {
    QMAKE_POST_LINK = $$(QTDIR)/bin/windeployqt $$OUT_PWD/../../SkypeServer/Debug
} else {
    QMAKE_POST_LINK = $$(QTDIR)/bin/windeployqt $$OUT_PWD/../../SkypeServer/Release
}

SOURCES += \
        main.cpp \
        widget.cpp \
    socketthread.cpp \
    rooms.cpp \
    room.cpp \
    tcpsocket.cpp \
    filetransfer.cpp \
    updater.cpp \
    audioserver.cpp

HEADERS += \
        widget.h \
    socketthread.h \
    rooms.h \
    room.h \
    tcpsocket.h \
    filetransfer.h \
    updater.h \
    audioserver.h

SUBDIRS += \
    SkypeServer.pro
