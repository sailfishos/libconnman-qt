set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS DBus)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS DBus Network)

qt_add_dbus_interfaces(DBUS_GENERATED_SRCS 
    connman_clock.xml
    connman_notification.xml
    connman_service.xml
    connman_session.xml
    connman_technology.xml
    connman_vpn_connection.xml
    connman_vpn_manager.xml
)

add_library(connman-qt 

    ${DBUS_GENERATED_SRCS}

    clockmodel.cpp
    clockmodel.h

    commondbustypes.cpp
    commondbustypes.h

    connmannetworkproxyfactory.cpp
    connmannetworkproxyfactory.h

    counter.cpp
    counter.h

    marshalutils.cpp
    marshalutils.h

    networkmanager.cpp
    networkmanager.h

    networkservice.cpp
    networkservice.h

    networktechnology.cpp
    networktechnology.h

    commondbustypes.cpp
    commondbustypes.h
)

target_link_libraries(connman-qt  PRIVATE Qt${QT_VERSION_MAJOR}::DBus Qt${QT_VERSION_MAJOR}::Network)
target_include_directories(connman-qt PUBLIC ./ ${CMAKE_CURRENT_BINARY_DIR})

# cmake add CONNMANQT_CMAKE for fix c++ #define 
#   qmake generate "connman_clock_interface.h" 
#   cmake generate "connman_clockinterface.h" 
target_compile_definitions(connman-qt PUBLIC CONNMANQT_CMAKE)