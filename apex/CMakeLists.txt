# Require a recent version of cmake
cmake_minimum_required(VERSION 3.3.2 FATAL_ERROR)

# This project is C++ based.
project(exemplo_simples CXX)

# Instruct cmake to find the HPX settings
find_package(HPX)


set(example_programs
    apex_policy_engine_periodic apex_policy_engine_events
    apex_policy_engine_active_thread_count apex_balance apex_fibonacci
    custom_event
)

foreach(example_program ${example_programs})
  set(${example_program}_PARAMETERS THREADS_PER_LOCALITY 4)

  set(sources ${example_program}.cpp)

  source_group("Source Files" FILES ${sources})

  # add example executable
  add_hpx_executable(
    ${example_program} INTERNAL_FLAGS
    SOURCES ${sources} ${${example_program}_FLAGS}
  )

endforeach()
