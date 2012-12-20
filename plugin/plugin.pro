TARGET=ConnmanQtDeclarative
TEMPLATE = lib
QT += declarative dbus
CONFIG += plugin
SOURCES = components.cpp networkingmodel.cpp technologymodel.cpp 
HEADERS = components.h networkingmodel.h technologymodel.h 

INCLUDEPATH += ../libconnman-qt
LIBS += -L../libconnman-qt -lconnman-qt4

target.path = $$[QT_INSTALL_IMPORTS]/MeeGo/Connman
qmldir.files += qmldir
qmldir.path = $$[QT_INSTALL_IMPORTS]/MeeGo/Connman

INSTALLS += target qmldir
