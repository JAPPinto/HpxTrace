# Require a recent version of cmake
cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

# This project is C++ based.
project(exemplo_simples CXX)

# Instruct cmake to find the HPX settings
find_package(HPX)




add_hpx_component( comp
    ESSENTIAL
    SOURCES server.cpp
    HEADERS server.hpp client.hpp
)

 
add_hpx_executable( main
    ESSENTIAL
    SOURCES main.cpp
    DEPENDENCIES comp_component  
)

