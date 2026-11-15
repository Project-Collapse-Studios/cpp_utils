cmake_minimum_required(VERSION 3.25)

project(cpp_utils
    DESCRIPTION "A library containing various small (but helpfull) features."
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(${PROJECT_NAME} STATIC
        # HEADERS
        "include/cutil_string.h"
        "include/default_pointer.h"
        "include/endless_well.h"
        "include/string_reference.h"
        # SOURCES
        "src/string_reference.cpp"
)

target_include_directories(${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
