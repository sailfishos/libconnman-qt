#-*-Shell-Script-*-

TEMPLATE = lib
VERSION = 0.0.1
CONFIG += qt \
    debug
QT += dbus

isEmpty(PREFIX) {
  PREFIX=/usr
}
TARGET = $$qtLibraryTarget(connman-qt)
target.path = $$INSTALL_ROOT$$PREFIX/lib

!exists(manager.h) {
  system(qdbusxml2cpp -c Manager -p manager -N connman-manager.xml)
}

!exists(service.h) {
  system(qdbusxml2cpp -c Service -p service -N connman-service.xml)
}

HEADERS += manager.h \
    service.h \
    networkitemmodel.h \
    networklistmodel.h \
	commondbustypes.h \

headers.files = manager.h service.h networkitemmodel.h \
		 networklistmodel.h commondbustypes.h
headers.path = $$INSTALL_ROOT$$PREFIX/include/connman-qt

CONFIG += create_pc create_prl
QMAKE_PKGCONFIG_DESCRIPTION = Qt Connman Library
QMAKE_PKGCONFIG_INCDIR = $$headers.path
pkgconfig.path = $$INSTALL_ROOT$$PREFIX/lib/pkgconfig
pkgconfig.files = connman-qt.pc

SOURCES += networkitemmodel.cpp \
		   networklistmodel.cpp \
		   manager.cpp \
		   service.cpp \

INSTALLS += target headers pkgconfig
