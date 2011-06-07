#-*-Shell-Script-*-

TEMPLATE = lib
VERSION=0.0.9
CONFIG += qt \
    debug
QT += dbus
TARGET = $$qtLibraryTarget(connman-qt4)
isEmpty(PREFIX) {
  PREFIX=/usr
}
target.path = $$INSTALL_ROOT$$PREFIX/lib

system(qdbusxml2cpp -c Manager -p manager -N connman-manager.xml)
system(qdbusxml2cpp -c Service -p service -N connman-service.xml)

HEADERS += manager.h \
    service.h \
    networkitem.h \
    networklist.h \
	commondbustypes.h \
    clockproxy.h \
    clockmodel.h \
    networkmanager.h

headers.files = manager.h service.h networkitem.h \
		 networklist.h commondbustypes.h clockproxy.h clockmodel.h
headers.path = $$INSTALL_ROOT$$PREFIX/include/connman-qt

CONFIG += create_pc create_prl
QMAKE_PKGCONFIG_DESCRIPTION = Qt Connman Library
QMAKE_PKGCONFIG_INCDIR = $$headers.path
pkgconfig.path = $$INSTALL_ROOT$$PREFIX/lib/pkgconfig
pkgconfig.files = connman-qt4.pc

SOURCES += networkitem.cpp \
		   networklist.cpp \
		   manager.cpp \
		   service.cpp \
           clockproxy.cpp \
           clockmodel.cpp \
    networkmanager.cpp

INSTALLS += target headers pkgconfig
