
QT += core dbus

TARGET = counters
CONFIG += console
CONFIG += link_pkgconfig
CONFIG -= app_bundle

TEMPLATE = app

isEmpty(TARGET_SUFFIX) {
    TARGET_SUFFIX = qt$$QT_MAJOR_VERSION
}
PKGCONFIG += connman-$$TARGET_SUFFIX

SOURCES += main.cpp \
    networkcounter.cpp

HEADERS += \
    networkcounter.h
