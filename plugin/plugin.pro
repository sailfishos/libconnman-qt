TARGET=ConnmanQtDeclarative
TEMPLATE = lib
QT += dbus
CONFIG += plugin
SOURCES = plugin.cpp technologymodel.cpp savedservicemodel.cpp
HEADERS = technologymodel.h savedservicemodel.h
INCLUDEPATH += ../libconnman-qt
LIBS += -L../libconnman-qt
QT -= gui

isEmpty(TARGET_SUFFIX) {
    TARGET_SUFFIX = qt$$QT_MAJOR_VERSION
}

LIBS += -l$$qtLibraryTarget(connman-$$TARGET_SUFFIX)

QT += qml
OTHER_FILES += plugin.json plugins.qmltypes qmldirs

MODULENAME = Connman
TARGETPATH = $$[QT_INSTALL_QML]/$$MODULENAME

target.path = $$TARGETPATH
qmldir.files += qmldir plugins.qmltypes
qmldir.path = $$TARGETPATH

INSTALLS += target qmldir

qmltypes.target = qmltypes
qmltypes.commands = qmlplugindump -nonrelocatable Connman 0.2 > $$PWD/plugins.qmltypes

QMAKE_EXTRA_TARGETS += qmltypes
