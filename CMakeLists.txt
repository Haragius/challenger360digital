cmake_minimum_required(VERSION 3.16)

project(challenger360digital VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(libchallenger360digital)
add_subdirectory(challenger360digitalctl)
add_subdirectory(challenger360digitald)
add_subdirectory(platform)

if(CMAKE_SYSTEM_NAME MATCHES "Linux")

    if("${CMAKE_INSTALL_PREFIX}" MATCHES "^/usr")
        set(UDEV_RULES_DIR "/lib/udev/rules.d")
        set(SYSTEMD_SERVICE_DIR "/lib/systemd/system")
    else()
        set(UDEV_RULES_DIR "${CMAKE_INSTALL_PREFIX}/lib/udev/rules.d")
        set(SYSTEMD_SERVICE_DIR "${CMAKE_INSTALL_PREFIX}/lib/systemd/system")
    endif()

    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/udev/99-challenger360digital.rules
        DESTINATION ${UDEV_RULES_DIR}
    )

    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/systemd/challenger360digital.service
        DESTINATION ${SYSTEMD_SERVICE_DIR}
    )

endif()