TARGET=ConnmanQtDeclarative
TEMPLATE = lib
QT += dbus
CONFIG += plugin
SOURCES = components.cpp networkingmodel.cpp technologymodel.cpp savedservicemodel.cpp
HEADERS = components.h networkingmodel.h technologymodel.h savedservicemodel.h
INCLUDEPATH += ../libconnman-qt
LIBS += -L../libconnman-qt
QT -= gui

isEmpty(TARGET_SUFFIX) {
    TARGET_SUFFIX = qt$$QT_MAJOR_VERSION
}

LIBS += -l$$qtLibraryTarget(connman-$$TARGET_SUFFIX)

QT += qml
OTHER_FILES += plugin.json qmldirs

MODULENAME = MeeGo/Connman
TARGETPATH = $$[QT_INSTALL_QML]/$$MODULENAME

target.path = $$TARGETPATH
qmldir.files += qmldir
qmldir.path = $$TARGETPATH

INSTALLS += target qmldir
