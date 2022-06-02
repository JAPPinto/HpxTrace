# Require a recent version of cmake
cmake_minimum_required(VERSION 3.3.2 FATAL_ERROR)

# This project is C++ based.
project(exemplo_simples CXX)

# Instruct cmake to find the HPX settings
find_package(HPX)

set (CMAKE_CXX_STANDARD 20)


add_hpx_component( ScalarVars
    ESSENTIAL
    SOURCES ScalarVars/ScalarVarsModule.cpp 
    HEADERS ScalarVars/ScalarVarsServer.hpp ScalarVars/ScalarVars.hpp
)

add_hpx_component( MapVars
    ESSENTIAL
    SOURCES MapVars/MapVarsModule.cpp 
    HEADERS MapVars/MapVarsServer.hpp MapVars/MapVars.hpp
)

add_hpx_component( Aggregations
    ESSENTIAL
    SOURCES Aggregations/Aggregations.cpp
    HEADERS Aggregations/AggregationsServer.hpp
)

add_hpx_component( Mutexes
    ESSENTIAL
    SOURCES Mutexes/Mutexes.cpp
    HEADERS Mutexes/MutexesServer.hpp
)

add_hpx_library( HpxTrace
    INSTALL_HEADERS
    SOURCES HpxTrace.cpp
    HEADERS HpxTrace.hpp
    DEPENDENCIES Mutexes_component Aggregations_component ScalarVars_component MapVars_component
)
add_hpx_executable( probes
    ESSENTIAL
    SOURCES probes.cpp
    DEPENDENCIES HpxTrace
)