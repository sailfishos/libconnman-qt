
QT += core dbus

TARGET = counters
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

equals(QT_MAJOR_VERSION, 5): {
    LIBS += -lconnman-qt5
    INCLUDEPATH += /usr/include/connman-qt5
}
equals(QT_MAJOR_VERSION, 4): {
    QT -= gui
    LIBS += -lconnman-qt4
    INCLUDEPATH += /usr/include/connman-qt
}
SOURCES += main.cpp \
    networkcounter.cpp

HEADERS += \
    networkcounter.h
