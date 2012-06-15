TARGET=ConnmanQtDeclarative
TEMPLATE = lib
QT += declarative dbus
CONFIG += plugin
SOURCES = components.cpp networkingmodel.cpp
HEADERS = components.h networkingmodel.h

INCLUDEPATH += ../libconnman-qt
LIBS += -L../libconnman-qt -lconnman-qt4

target.path = $$[QT_INSTALL_IMPORTS]/Connman/Qt
qmldir.files += qmldir
qmldir.path = $$[QT_INSTALL_IMPORTS]/Connman/Qt

INSTALLS += target qmldir
