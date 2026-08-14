add_library(challenger360digital_platform
    pidfile.cpp
    pidfile.h
)

target_include_directories(challenger360digital_platform
    PUBLIC
        ${PROJECT_SOURCE_DIR}
)

target_compile_features(challenger360digital_platform
    PUBLIC
        cxx_std_20
)