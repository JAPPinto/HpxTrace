# Require a recent version of cmake
cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

# This project is C++ based.
project(exemplo_simples CXX)

# Instruct cmake to find the HPX settings
find_package(HPX)


add_hpx_component( timer
    ESSENTIAL
    SOURCES example.cpp server/example.cpp
    HEADERS server/example.hpp
)

 
add_hpx_executable( main
    ESSENTIAL
    SOURCES example_client.cpp
    DEPENDENCIES timer_component  
)
