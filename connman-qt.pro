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

equals(QT_MAJOR_VERSION, 4):  {
    OTHER_FILES += rpm/connman-qt.spec \
                   rpm/connman-qt.yaml
}

equals(QT_MAJOR_VERSION, 5):  {
    OTHER_FILES += rpm/connman-qt5.spec \
                   rpm/connman-qt5.yaml
}
