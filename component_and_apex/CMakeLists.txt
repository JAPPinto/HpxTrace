# Require a recent version of cmake
cmake_minimum_required(VERSION 3.3.2 FATAL_ERROR)

# This project is C++ based.
project(exemplo_simples CXX)

# Instruct cmake to find the HPX settings
find_package(HPX)




add_hpx_component( comp
    ESSENTIAL
    SOURCES comp.cpp
    HEADERS comp.hpp comp_server/comp.hpp
)

 
add_hpx_executable( apex_action_count
    ESSENTIAL
    SOURCES apex_action_count.cpp
    DEPENDENCIES comp_component 
)

