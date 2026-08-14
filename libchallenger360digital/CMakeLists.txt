cmake_minimum_required(VERSION 3.16)

project(libchallenger360digital LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(GNUInstallDirs)

add_library(libchallenger360digital SHARED
  libchallenger360digital_global.h
  libchallenger360digital.cpp
  libchallenger360digital.h
  hiddevice.h
  hiddevice.cpp
  displayprotocol.h
  displayprotocol.cpp
  displaystate.h
  configparser.cpp
  configparser.h
  sensors.h
  sensors.cpp
  configexceptions.h
  sensorexceptions.h
  applicationconfig.h
  applicationconfig.cpp
  displayexceptions.h
)

set_target_properties(libchallenger360digital PROPERTIES
  PUBLIC_HEADER libchallenger360digital.h
)

target_compile_definitions(libchallenger360digital PRIVATE LIBCHALLENGER36_DIGITAL_LIBRARY)

target_include_directories(libchallenger360digital
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

find_package(hidapi REQUIRED)

target_link_libraries(libchallenger360digital
    PRIVATE
        hidapi::hidapi
        sensors
)

install(TARGETS libchallenger360digital
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/challenger360
    )