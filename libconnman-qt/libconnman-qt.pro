#-*-Shell-Script-*-

TEMPLATE = lib
VERSION=0.1.4
CONFIG += qt \
    debug
QT += dbus
TARGET = $$qtLibraryTarget(connman-qt4)
isEmpty(PREFIX) {
  PREFIX=/usr
}
target.path = $$INSTALL_ROOT$$PREFIX/lib

#system(qdbusxml2cpp -c Manager -p manager -N connman-manager.xml)
system(qdbusxml2cpp -c Service -p service -N connman-service.xml)
system(qdbusxml2cpp -c Technology -p technology -N connman-technology.xml)

HEADERS += manager.h \
    service.h \
    technology.h \
    networkmanager.h \
    networktechnology.h \
    networkservice.h \
	commondbustypes.h \
    clockproxy.h \
    clockmodel.h

headers.files = manager.h service.h technology.h \
         commondbustypes.h clockproxy.h clockmodel.h networkmanager.h \
         networktechnology.h networkservice.h
headers.path = $$INSTALL_ROOT$$PREFIX/include/connman-qt

CONFIG += create_pc create_prl
QMAKE_PKGCONFIG_DESCRIPTION = Qt Connman Library
QMAKE_PKGCONFIG_INCDIR = $$headers.path
pkgconfig.path = $$INSTALL_ROOT$$PREFIX/lib/pkgconfig
pkgconfig.files = connman-qt4.pc

SOURCES += \
		   networkmanager.cpp \
		   networktechnology.cpp \
		   networkservice.cpp \
		   manager.cpp \
		   service.cpp \
           technology.cpp \
           clockproxy.cpp \
           clockmodel.cpp \
           commondbustypes.cpp

INSTALLS += target headers pkgconfig
