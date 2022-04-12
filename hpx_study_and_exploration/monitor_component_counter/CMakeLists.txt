# Require a recent version of cmake
cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

# This project is C++ based.
project(exemplo_simples CXX)

# Instruct cmake to find the HPX settings
find_package(HPX)




add_hpx_component( comp
    ESSENTIAL
    SOURCES comp.cpp
    HEADERS comp.hpp comp_server/comp.hpp
)

add_hpx_component( example_counters
    ESSENTIAL
    SOURCES examples.cpp name_counter_server/name_counter.cpp
    HEADERS name_counter_server/name_counter.hpp
)

 
add_hpx_executable( api
    ESSENTIAL
    SOURCES api.cpp
    DEPENDENCIES example_counters_component comp_component  
)

