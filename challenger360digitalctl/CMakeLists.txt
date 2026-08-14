cmake_minimum_required(VERSION 3.16)

project(challenger360digitalctl LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(challenger360digitalctl main.cpp
    ctlcommandline.h
    ctlcommandline.cpp
    ctlapplication.h
    ctlapplication.cpp)

include(GNUInstallDirs)
install(TARGETS challenger360digitalctl
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
target_link_libraries(challenger360digitalctl
    PRIVATE
        libchallenger360digital
        challenger360digital_platform
)
