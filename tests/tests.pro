include(tests_common.pri)

TEMPLATE = subdirs
SUBDIRS = \
    ut_agent.pro \
    ut_clock.pro \
    ut_manager.pro \
    ut_service.pro \
    ut_session.pro \
    ut_technology.pro \

runtest_sh.path = $${INSTALL_TESTDIR}
runtest_sh.files = runtest.sh
INSTALLS += runtest_sh

tests_xml.path = $${INSTALL_TESTDIR}
tests_xml.files = tests.xml
INSTALLS += tests_xml

make_default.CONFIG = phony
QMAKE_EXTRA_TARGETS += make_default

check.depends = make_default
check.CONFIG = phony recursive
QMAKE_EXTRA_TARGETS += check
