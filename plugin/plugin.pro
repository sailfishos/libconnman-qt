TARGET=ConnmanQtDeclarative
TEMPLATE = lib
QT += dbus
CONFIG += plugin
SOURCES = components.cpp networkingmodel.cpp technologymodel.cpp savedservicemodel.cpp
HEADERS = components.h networkingmodel.h technologymodel.h savedservicemodel.h
INCLUDEPATH += ../libconnman-qt
LIBS += -L../libconnman-qt
QT -= gui

equals(QT_MAJOR_VERSION, 4): {
    QT += declarative
    LIBS += -lconnman-qt4
}

equals(QT_MAJOR_VERSION, 5): {
    QT += qml
    LIBS += -lconnman-qt5
    OTHER_FILES += plugin.json qmldirs
}

MODULENAME = MeeGo/Connman
equals(QT_MAJOR_VERSION, 4): TARGETPATH = $$[QT_INSTALL_IMPORTS]/$$MODULENAME
equals(QT_MAJOR_VERSION, 5): TARGETPATH = $$[QT_INSTALL_QML]/$$MODULENAME

target.path = $$TARGETPATH
qmldir.files += qmldir
qmldir.path = $$TARGETPATH

INSTALLS += target qmldir
