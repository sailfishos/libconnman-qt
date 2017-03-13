TEMPLATE     = lib
CONFIG      += qt debug
CONFIG      += create_pc create_prl
QT          += core dbus network
QT          -= gui

isEmpty(VERSION) {
    VERSION = 1.1.3
    message("VERSION is unset, assuming $$VERSION")
}

isEmpty(PREFIX) {
  PREFIX=/usr
}

isEmpty(TARGET_SUFFIX) {
    TARGET_SUFFIX = qt$$QT_MAJOR_VERSION
}

CONFIG(debug, debug|release) {
    DEFINES += CONNMAN_DEBUG=1
}

TARGET = $$qtLibraryTarget(connman-$$TARGET_SUFFIX)
headers.path = $$INSTALL_ROOT$$PREFIX/include/connman-$$TARGET_SUFFIX

DBUS_INTERFACES = \
    connman_clock.xml \
    connman_manager.xml \
    connman_session.xml \
    connman_technology.xml \

PUBLIC_HEADERS += \
    networkmanager.h \
    networktechnology.h \
    networkservice.h \
    commondbustypes.h \
    connmannetworkproxyfactory.h \
    clockmodel.h \
    useragent.h \
    sessionagent.h \
    networksession.h \
    counter.h

HEADERS += \
    $$PUBLIC_HEADERS \
    libconnman_p.h \

SOURCES += \
    networkmanager.cpp \
    networktechnology.cpp \
    networkservice.cpp \
    clockmodel.cpp \
    commondbustypes.cpp \
    connmannetworkproxyfactory.cpp \
    useragent.cpp \
    sessionagent.cpp \
    networksession.cpp \
    counter.cpp

target.path = $$INSTALL_ROOT$$PREFIX/lib

headers.files = $$PUBLIC_HEADERS

QMAKE_PKGCONFIG_DESCRIPTION = Qt Connman Library
QMAKE_PKGCONFIG_DESTDIR = pkgconfig
QMAKE_PKGCONFIG_INCDIR = $$headers.path

INSTALLS += target headers

OTHER_FILES = connman_service.xml \
    connman_technology.xml \
    connman_clock.xml \
    connman_manager.xml \
    connman_session.xml \
    connman_notification.xml

