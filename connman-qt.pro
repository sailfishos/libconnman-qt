TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS += libconnman-qt

# CONFIG flag to disable qml plugin
!noplugin {
    SUBDIRS += plugin
}

# CONFIG flag to disable test program
!notest {
    SUBDIRS += test
}
