include(tests_common.pri)

pro_file_basename = $$basename(_PRO_FILE_)
pro_file_basename = $$replace(pro_file_basename, '.pro', '')

TEMPLATE = app
QT += dbus testlib
TARGET = $${pro_file_basename}.bin

SOURCES = $${pro_file_basename}.cpp

LIBS += -L$${OUT_PWD}/../libconnman-qt -l$$qtLibraryTarget(connman-qt4)

target.path = $${INSTALL_TESTDIR}
INSTALLS += target

check.depends = all
check.commands = '\
    bash -c \'eval `dbus-launch --sh-syntax`; \
        trap "kill \$\${DBUS_SESSION_BUS_PID}" EXIT; \
        export LD_LIBRARY_PATH="$${OUT_PWD}/../libconnman-qt:\$\${LD_LIBRARY_PATH}"; \
        $${OUT_PWD}/$${TARGET};\''
check.CONFIG = phony
QMAKE_EXTRA_TARGETS += check
