TEMPLATE     = lib
VERSION      = 0.3.0
CONFIG      += qt debug
CONFIG      += create_pc create_prl
QT          += core dbus declarative
QT          -= gui
equals(QT_MAJOR_VERSION, 4): TARGET = $$qtLibraryTarget(connman-qt4)
equals(QT_MAJOR_VERSION, 5): TARGET = $$qtLibraryTarget(connman-qt5)

isEmpty(PREFIX) {
  PREFIX=/usr
}

DBUS_INTERFACES = \
    connman_clock.xml \
    connman_manager.xml \
    connman_service.xml \
    connman_session.xml \
    connman_technology.xml \

HEADERS += \
    networkmanager.h \
    networktechnology.h \
    networkservice.h \
    commondbustypes.h \
    clockmodel.h \
    debug.h \
    useragent.h \
    sessionagent.h \
    networksession.h \
    counter.h \

SOURCES += \
    networkmanager.cpp \
    networktechnology.cpp \
    networkservice.cpp \
    clockmodel.cpp \
    commondbustypes.cpp \
    debug.cpp \
    useragent.cpp \
    sessionagent.cpp \
    networksession.cpp \
    counter.cpp \

target.path = $$INSTALL_ROOT$$PREFIX/lib

headers.files = $$HEADERS
headers.path = $$INSTALL_ROOT$$PREFIX/include/connman-qt

QMAKE_PKGCONFIG_DESCRIPTION = Qt Connman Library
QMAKE_PKGCONFIG_INCDIR = $$headers.path
pkgconfig.path = $$INSTALL_ROOT$$PREFIX/lib/pkgconfig
pkgconfig.files = connman-qt4.pc

INSTALLS += target headers pkgconfig

OTHER_FILES = connman_service.xml \
    connman_technology.xml \
    connman_clock.xml \
    connman_manager.xml \
    connman_session.xml \
    connman_notification.xml
