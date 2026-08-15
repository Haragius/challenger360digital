cmake_minimum_required(VERSION 3.16)

project(challenger360digital VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_SYSTEM_NAME MATCHES "Linux")

set(_is_sys_prefix FALSE)
if("${CMAKE_INSTALL_PREFIX}" STREQUAL "/usr" OR "${CMAKE_INSTALL_PREFIX}" STREQUAL "/usr/lib")
    set(_is_sys_prefix TRUE)
endif()

if(NOT _is_sys_prefix)
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
    set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib")
endif()

set(UDEV_RULES_DIR "lib/udev/rules.d")
set(SYSTEMD_SERVICE_DIR "lib/systemd/system")

add_subdirectory(libchallenger360digital)
add_subdirectory(challenger360digitalctl)
add_subdirectory(challenger360digitald)
add_subdirectory(platform)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/udev/99-challenger360digital.rules
        DESTINATION "${UDEV_RULES_DIR}")

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/systemd/challenger360digital.service
        DESTINATION "${SYSTEMD_SERVICE_DIR}")

endif()