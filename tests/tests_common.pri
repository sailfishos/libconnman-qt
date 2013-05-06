
equals(QT_MAJOR_VERSION, 4):  {
    INSTALL_TESTDIR = /opt/tests/connman-qt
}

equals(QT_MAJOR_VERSION, 5):  {
    INSTALL_TESTDIR = /opt/tests/connman-qt5
}

INSTALL_TESTDATADIR = $${INSTALL_TESTDIR}/data
