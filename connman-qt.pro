TEMPLATE = subdirs
SUBDIRS += libconnman-qt

# Adds 'coverage' target
include(coverage.pri)
# CONFIG flag to disable qml plugin
!noplugin {
    SUBDIRS += plugin
    libconnman-qt.target = lib-target
    plugin.depends = lib-target
}
example {
   SUBDIRS += examples/counters
}

OTHER_FILES += rpm/connman-qt5.spec
