# Require a recent version of cmake
cmake_minimum_required(VERSION 3.3.2 FATAL_ERROR)

# This project is C++ based.
project(exemplo_simples CXX)

# Instruct cmake to find the HPX settings
find_package(HPX)

set (CMAKE_CXX_STANDARD 20)

if(NOT HPX_WITH_DISTRIBUTED_RUNTIME)
  return()
endif()

set(example_programs
    1d_stencil_1
    1d_stencil_2
    1d_stencil_3
    1d_stencil_4
    1d_stencil_4_parallel
    1d_stencil_5
    1d_stencil_6
    1d_stencil_7
    1d_stencil_8
    1d_stencil_channel
)

if(HPX_WITH_APEX)
  set(example_programs ${example_programs} 1d_stencil_4_repart
                       1d_stencil_4_throttle
  )

  set(1d_stencil_4_repart_PARAMETERS THREADS_PER_LOCALITY 4)
  set(1d_stencil_4_throttle_PARAMETERS THREADS_PER_LOCALITY 4)
endif()


set(1d_stencil_1_PARAMETERS THREADS_PER_LOCALITY 4)
set(1d_stencil_2_PARAMETERS THREADS_PER_LOCALITY 4)
set(1d_stencil_3_PARAMETERS THREADS_PER_LOCALITY 4)
set(1d_stencil_4_PARAMETERS THREADS_PER_LOCALITY 4)
set(1d_stencil_4_parallel_PARAMETERS THREADS_PER_LOCALITY 4)
set(1d_stencil_5_PARAMETERS THREADS_PER_LOCALITY 4)
set(1d_stencil_6_PARAMETERS THREADS_PER_LOCALITY 4)
set(1d_stencil_7_PARAMETERS THREADS_PER_LOCALITY 4)
set(1d_stencil_8_PARAMETERS THREADS_PER_LOCALITY 4)


include_directories(${CMAKE_SOURCE_DIR}/../HpxTrace/build)
link_directories(${CMAKE_SOURCE_DIR}/../HpxTrace/build)

foreach(example_program ${example_programs})

  set(${example_program}_FLAGS COMPONENT_DEPENDENCIES iostreams)

  set(sources ${example_program}.cpp)

  source_group("Source Files" FILES ${sources})

  # add example executable
  add_hpx_executable(
    ${example_program} INTERNAL_FLAGS
    SOURCES ${sources} ${${example_program}_FLAGS}
    DEPENDENCIES HpxTrace
  )


endforeach()
