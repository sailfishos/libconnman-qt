TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS += libconnman-qt

check.depends = all
check.CONFIG = phony recursive
QMAKE_EXTRA_TARGETS += check

# Adds 'coverage' target
include(coverage.pri)
# CONFIG flag to disable qml plugin
!noplugin {
    SUBDIRS += plugin
}
example {
   SUBDIRS += examples/counters
}

OTHER_FILES += rpm/connman-qt5.spec
