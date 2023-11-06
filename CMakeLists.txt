cmake_minimum_required(VERSION 3.16)
#set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(environment-server
        VERSION 1.0.0
        DESCRIPTION "environment server"
        LANGUAGE CXX )

