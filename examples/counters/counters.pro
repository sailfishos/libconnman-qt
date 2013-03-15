
QT       += core dbus

QT       -= gui

TARGET = counters
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

LIBS += -lconnman-qt4
INCLUDEPATH += /usr/include/connman-qt

SOURCES += main.cpp \
    networkcounter.cpp

HEADERS += \
    networkcounter.h
