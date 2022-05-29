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


contains(CONFIG,no-module-prefix) {
    system("sed -i 's/@@ModulePrefix@@//' qmldir")
    system("sed -i 's/@@ModulePrefix@@//' plugins.qmltypes")
    MODULENAME = Connman
} else {
    system("sed -i 's/@@ModulePrefix@@/MeeGo\./' qmldir")
    system("sed -i 's/@@ModulePrefix@@/MeeGo\./' plugins.qmltypes")
    MODULENAME = MeeGo/Connman
    DEFINES += USE_MODULE_PREFIX
}

TARGETPATH = $$[QT_INSTALL_QML]/$$MODULENAME

target.path = $$TARGETPATH
qmldir.files += qmldir plugins.qmltypes
qmldir.path = $$TARGETPATH

INSTALLS += target qmldir

qmltypes.target = qmltypes
contains(CONFIG,no-module-prefix) {
    qmltypes.commands = qmlplugindump -nonrelocatable Connman 0.2 > $$PWD/plugins.qmltypes
} else {
    qmltypes.commands = qmlplugindump -nonrelocatable MeeGo.Connman 0.2 > $$PWD/plugins.qmltypes
}

QMAKE_EXTRA_TARGETS += qmltypes
