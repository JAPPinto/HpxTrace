# Require a recent version of cmake
cmake_minimum_required(VERSION 3.3.2 FATAL_ERROR)

# This project is C++ based.
project(exemplo_simples CXX)

# Instruct cmake to find the HPX settings
find_package(HPX)

set (CMAKE_CXX_STANDARD 20)


add_hpx_component( comp
    ESSENTIAL
    SOURCES comp.cpp
    HEADERS comp.hpp comp_server/comp.hpp
)

add_hpx_component( script_data
    ESSENTIAL
    SOURCES script_data.cpp
    HEADERS script_data.hpp script_data_server/script_data.hpp
)

add_hpx_component( VarsComp
    ESSENTIAL
    SOURCES VarsComp/VarsComp.cpp
    HEADERS VarsComp/VarsCompClient.hpp VarsComp/VarsCompServer.hpp
)
 
add_hpx_executable( apex_action_count
    ESSENTIAL
    SOURCES apex_action_count.cpp
    DEPENDENCIES comp_component 
)

add_hpx_executable( probes
    ESSENTIAL
    SOURCES probes.cpp
    DEPENDENCIES script_data_component  VarsComp_component
)

