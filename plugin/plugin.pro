TARGET=ConnmanQtDeclarative
TEMPLATE = lib
QT += declarative dbus
CONFIG += plugin link_pkgconfig
SOURCES = components.cpp
HEADERS = components.h

PKGCONFIG += connman-qt4

target.path = $$[QT_INSTALL_IMPORTS]/MeeGo/Connman
qmldir.files += qmldir
qmldir.path = $$[QT_INSTALL_IMPORTS]/MeeGo/Connman

INSTALLS += target qmldir
