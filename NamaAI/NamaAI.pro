#-------------------------------------------------
#
# Project created by QtCreator 2014-05-18T18:34:48
#
#-------------------------------------------------

QT       -= gui

TARGET = NamaAI
TEMPLATE = lib
CONFIG += plugin

DEFINES += NAMAAI_LIBRARY

SOURCES += \
    code/namaai.cpp \
    code/mt19937ar.cpp \
    code/utility.cpp

HEADERS += \
    code/namaai.h\
    code/namaai_global.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}
