TARGET=ConnmanQtDeclarative
TEMPLATE = lib
QT += dbus
CONFIG += plugin
SOURCES = components.cpp networkingmodel.cpp technologymodel.cpp 
HEADERS = components.h networkingmodel.h technologymodel.h 
INCLUDEPATH += ../libconnman-qt
LIBS += -L../libconnman-qt
QT -= gui
QT += declarative

equals(QT_MAJOR_VERSION, 4): {
    LIBS += -lconnman-qt4
}

equals(QT_MAJOR_VERSION, 5): {
    LIBS += -lconnman-qt5
    OTHER_FILES += plugin.json qmldirs
}

target.path = $$[QT_INSTALL_IMPORTS]/MeeGo/Connman
qmldir.files += qmldir
qmldir.path = $$[QT_INSTALL_IMPORTS]/MeeGo/Connman

INSTALLS += target qmldir
