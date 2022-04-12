# Require a recent version of cmake
cmake_minimum_required(VERSION 3.3.2 FATAL_ERROR)

# This project is C++ based.
project(exemplo_simples CXX)

# Instruct cmake to find the HPX settings
find_package(HPX)


add_hpx_component( test
    ESSENTIAL
    SOURCES test.cpp
    HEADERS test.hpp server/test.hpp
)

add_hpx_executable( exec
    ESSENTIAL
    SOURCES main.cpp
    DEPENDENCIES test_component
  )
