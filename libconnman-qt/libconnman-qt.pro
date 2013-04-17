TEMPLATE     = lib
VERSION      = 0.3.0
CONFIG      += qt debug
CONFIG      += create_pc create_prl
QT          += core dbus
QT          -= gui
equals(QT_MAJOR_VERSION, 4): TARGET = $$qtLibraryTarget(connman-qt4)
equals(QT_MAJOR_VERSION, 5): TARGET = $$qtLibraryTarget(connman-qt5)

isEmpty(PREFIX) {
  PREFIX=/usr
}

#system(qdbusxml2cpp -c Manager -p manager -N connman-manager.xml)
system(qdbusxml2cpp -c Service -p service -N connman-service.xml)
system(qdbusxml2cpp -c Technology -p technology -N connman-technology.xml)
#system(qdbusxml2cpp -c Session -p session -N connman-session.xml)

HEADERS += manager.h \
    service.h \
    technology.h \
    networkmanager.h \
    networktechnology.h \
    networkservice.h \
    commondbustypes.h \
    clockproxy.h \
    clockmodel.h \
    debug.h \
    useragent.h \
    session.h \
    sessionagent.h \
    networksession.h \
    counter.h

SOURCES += \
    networkmanager.cpp \
    networktechnology.cpp \
    networkservice.cpp \
    manager.cpp \
    service.cpp \
    technology.cpp \
    clockproxy.cpp \
    clockmodel.cpp \
    commondbustypes.cpp \
    debug.cpp \
    useragent.cpp \
    session.cpp \
    sessionagent.cpp \
    networksession.cpp \
    counter.cpp


target.path = $$INSTALL_ROOT$$PREFIX/lib

headers.files = $$HEADERS
headers.path = $$INSTALL_ROOT$$PREFIX/include/connman-qt

QMAKE_PKGCONFIG_DESCRIPTION = Qt Connman Library
QMAKE_PKGCONFIG_INCDIR = $$headers.path
pkgconfig.path = $$INSTALL_ROOT$$PREFIX/lib/pkgconfig
pkgconfig.files = connman-qt4.pc

INSTALLS += target headers pkgconfig

OTHER_FILES = connman-service.xml \
    connman-technology.xml \
    connman-clock.xml \
    connman-manager.xml \
    connman-session.xml \
    connman-notification.xml
