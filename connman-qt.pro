TEMPLATE = subdirs
SUBDIRS += libconnman-qt
CONFIG += ordered

# CONFIG flag to disable qml plugin
!noplugin {
    SUBDIRS += plugin
}

# CONFIG flag to disable test program
!notest {
    SUBDIRS += test
}
