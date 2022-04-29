# Require a recent version of cmake
cmake_minimum_required(VERSION 3.3.2 FATAL_ERROR)

# This project is C++ based.
project(exemplo_simples CXX)

# Instruct cmake to find the HPX settings
find_package(HPX)

set (CMAKE_CXX_STANDARD 20)


add_hpx_component( Map
    ESSENTIAL
    SOURCES Map.cpp
    HEADERS MapClient.hpp MapServer.hpp SMapServer.hpp
)

add_hpx_executable( main
    ESSENTIAL
    SOURCES main.cpp
    DEPENDENCIES Map_component
)
