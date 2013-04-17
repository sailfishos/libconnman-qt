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

# CONFIG flag to disable test program
!notest {
    SUBDIRS += test
}

# CONFIG flag to disable automatic test
!notests {
    SUBDIRS += tests
}

equals(QT_MAJOR_VERSION, 4):  {
    OTHER_FILES += rpm/connman-qt.spec \
                   rpm/connman-qt.yaml
}

equals(QT_MAJOR_VERSION, 5):  {
    OTHER_FILES += rpm/connman-qt5.spec \
                   rpm/connman-qt5.yaml
}
