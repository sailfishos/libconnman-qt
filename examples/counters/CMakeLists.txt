set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS DBus)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS DBus)

add_executable(counters main.cpp networkcounter.cpp networkcounter.h)
target_link_libraries(counters PRIVATE Qt${QT_VERSION_MAJOR}::DBus connman-qt)