cmake_minimum_required(VERSION 3.0.0)
project(connman-qt VERSION 0.1.0 LANGUAGES C CXX)


# Determine if lib is built as a subproject (using add_subdirectory)
# or if it is the master project.
if (NOT DEFINED CONNMANQT_MASTER_PROJECT)
  set(CONNMANQT_MASTER_PROJECT OFF)
  if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(CONNMANQT_MASTER_PROJECT ON)
    message(STATUS "CONNMANQT_MASTER_PROJECT:${CONNMANQT_MASTER_PROJECT} CMake version: ${CMAKE_VERSION}")
  endif ()
endif ()

add_subdirectory(libconnman-qt)
add_subdirectory(examples/counters)