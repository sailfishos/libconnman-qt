#-------------------------------------------------
#
# Project created by QtCreator 2010-10-12T13:49:46
#
#-------------------------------------------------

QT += core  gui

equals(QT_MAJOR_VERSION, 4): {
    QT += declarative opengl
    TARGET = testconnman-qt
    target.path = $$INSTALL_ROOT/usr/lib/libconnman-qt4/test
}
equals(QT_MAJOR_VERSION, 5): {
    QT += quick
    TARGET = testconnman-qt5
    target.path = $$INSTALL_ROOT/usr/lib/libconnman-qt5/test
}
CONFIG   += console
CONFIG   -= app_bundle

OTHER_FILES += *.qml

TEMPLATE = app


SOURCES += main.cpp

HEADERS +=


qml.files = $$OTHER_FILES
qml.path = $$target.path


INSTALLS += target qml
