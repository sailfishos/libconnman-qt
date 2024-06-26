TEMPLATE     = lib
QT          += core dbus network
QT          -= gui
CONFIG      += qt create_pc create_prl link_pkgconfig

isEmpty(VERSION) {
    VERSION = 1.2.35
    message("VERSION is unset, assuming $$VERSION")
}

isEmpty(HAVE_LIBDBUSACCESS) {
    packagesExist(libdbusaccess) {
        HAVE_LIBDBUSACCESS = 1
    } else {
        HAVE_LIBDBUSACCESS = 0
    }
}

equals(HAVE_LIBDBUSACCESS, 1) {
    PKGCONFIG   += libdbusaccess libglibutil
    DEFINES     += HAVE_LIBDBUSACCESS=1
} else {
    DEFINES     += HAVE_LIBDBUSACCESS=0
}

isEmpty(PREFIX) {
  PREFIX=/usr
}

isEmpty(TARGET_SUFFIX) {
    TARGET_SUFFIX = qt$$QT_MAJOR_VERSION
}

TARGET = $$qtLibraryTarget(connman-$$TARGET_SUFFIX)
headers.path = $$INSTALL_ROOT$$PREFIX/include/connman-$$TARGET_SUFFIX

custom_dbus_interface.files = \
    connman_service.xml \
    connman_vpn_manager.xml \
    connman_vpn_connection.xml

custom_dbus_interface.header_flags = -i qdbusxml2cpp_dbus_types.h

DBUS_INTERFACES = \
    connman_clock.xml \
    connman_session.xml \
    connman_technology.xml \
    custom_dbus_interface

PUBLIC_HEADERS += \
    networkmanager.h \
    networktechnology.h \
    networkservice.h \
    connmannetworkproxyfactory.h \
    clockmodel.h \
    useragent.h \
    sessionagent.h \
    networksession.h \
    counter.h \
    vpnconnection.h \
    vpnmanager.h \
    vpnmodel.h

HEADERS += \
    $$PUBLIC_HEADERS \
    logging.h \
    marshalutils.h \
    commondbustypes.h \
    vpnconnection_p.h \
    vpnmanager_p.h \
    vpnmodel_p.h \
    qdbusxml2cpp_dbus_types.h

SOURCES += \
    logging.cpp \
    marshalutils.cpp \
    networkmanager.cpp \
    networktechnology.cpp \
    networkservice.cpp \
    clockmodel.cpp \
    commondbustypes.cpp \
    connmannetworkproxyfactory.cpp \
    useragent.cpp \
    sessionagent.cpp \
    networksession.cpp \
    counter.cpp \
    vpnconnection.cpp \
    vpnmanager.cpp \
    vpnmodel.cpp

target.path = $$[QT_INSTALL_LIBS]

headers.files = $$PUBLIC_HEADERS

QMAKE_PKGCONFIG_DESCRIPTION = Qt Connman Library
QMAKE_PKGCONFIG_DESTDIR = pkgconfig
QMAKE_PKGCONFIG_INCDIR = $$headers.path

INSTALLS += target headers

OTHER_FILES = connman_notification.xml \
    DBUS_INTERFACES
