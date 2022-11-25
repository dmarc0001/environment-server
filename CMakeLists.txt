# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

list( APPEND EXTRA_COMPONENT_DIRS /usr/local/esp-extra-components/components )

project(rest-server
        VERSION 0.0.2
        DESCRIPTION "environment server"
        LANGUAGE CXX )
