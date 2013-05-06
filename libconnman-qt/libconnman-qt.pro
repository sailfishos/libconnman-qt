TEMPLATE     = lib
VERSION      = 0.3.0
CONFIG      += qt debug
CONFIG      += create_pc create_prl
QT          += core dbus
QT -= gui

equals(QT_MAJOR_VERSION, 4): {
    TARGET = $$qtLibraryTarget(connman-qt4)
    headers.path = $$INSTALL_ROOT$$PREFIX/include/connman-qt
    pkgconfig.files = connman-qt4.pc
}

equals(QT_MAJOR_VERSION, 5): {
    TARGET = $$qtLibraryTarget(connman-qt5)
    headers.path = $$INSTALL_ROOT$$PREFIX/include/connman-qt5
    pkgconfig.files = connman-qt5.pc
}

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
    counter.h

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
    counter.cpp

target.path = $$INSTALL_ROOT$$PREFIX/lib

headers.files = $$HEADERS

QMAKE_PKGCONFIG_DESCRIPTION = Qt Connman Library
QMAKE_PKGCONFIG_INCDIR = $$headers.path

pkgconfig.path = $$INSTALL_ROOT$$PREFIX/lib/pkgconfig

INSTALLS += target headers pkgconfig

OTHER_FILES = connman_service.xml \
    connman_technology.xml \
    connman_clock.xml \
    connman_manager.xml \
    connman_session.xml \
    connman_notification.xml

