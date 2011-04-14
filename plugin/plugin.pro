TARGET=ConnmanQtDeclarative
TEMPLATE = lib
QT += declarative dbus
CONFIG += plugin
SOURCES = components.cpp
HEADERS = components.h

INCLUDEPATH += ../libconnman-qt
LIBS += -L../libconnman-qt -lconnman-qt4

target.path = $$[QT_INSTALL_IMPORTS]/MeeGo/Connman
qmldir.files += qmldir
qmldir.path = $$[QT_INSTALL_IMPORTS]/MeeGo/Connman

INSTALLS += target qmldir
