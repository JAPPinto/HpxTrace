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



add_hpx_component( example_counters
    ESSENTIAL
    SOURCES examples.cpp basename_counter_server/basename_counter.cpp name_counter_server/name_counter.cpp
    HEADERS basename_counter_server/basename_counter.hpp name_counter_server/name_counter.hpp
)

add_hpx_executable( api_name
    ESSENTIAL
    SOURCES api_name.cpp
    DEPENDENCIES comp_component example_counters_component
)

add_hpx_executable( api_basename
    ESSENTIAL
    SOURCES api_basename.cpp
    DEPENDENCIES comp_component example_counters_component
)
 
add_hpx_executable( cmd
    ESSENTIAL
    SOURCES cmd.cpp
    DEPENDENCIES comp_component example_counters_component
    COMPONENT_DEPENDENCIES iostreams
  )