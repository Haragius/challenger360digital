cmake_minimum_required(VERSION 3.16)

project(challenger360digitald LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(challenger360digitald main.cpp
    commandline.h
    commandline.cpp
    signals.h
    signals.cpp
    displaycontroller.h
    displaycontroller.cpp
    application.h
    application.cpp)

include(GNUInstallDirs)
install(TARGETS challenger360digitald
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
target_link_libraries(challenger360digitald
    PRIVATE
        libchallenger360digital
        challenger360digital_platform
)
target_compile_options(challenger360digitald PRIVATE -pthread)
