# Require a recent version of cmake
cmake_minimum_required(VERSION 3.3.2 FATAL_ERROR)

# This project is C++ based.
project(exemplo_simples CXX)

# Instruct cmake to find the HPX settings
find_package(HPX)

add_hpx_executable(ex1-threads
    SOURCES ex1-threads.cpp
    COMPONENT_DEPENDENCIES iostreams)


add_hpx_executable(ex2-papi
    SOURCES ex2-papi.cpp
    COMPONENT_DEPENDENCIES iostreams)

add_hpx_executable(ex3-set
    SOURCES ex3-set.cpp
    COMPONENT_DEPENDENCIES iostreams)
