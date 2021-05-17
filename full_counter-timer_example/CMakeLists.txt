# Require a recent version of cmake
cmake_minimum_required(VERSION 3.3.2 FATAL_ERROR)

# This project is C++ based.
project(exemplo_simples CXX)

# Instruct cmake to find the HPX settings
find_package(HPX)


# Copyright (c) 2007-2012 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#set(root "${PROJECT_SOURCE_DIR}/examples/performance_counters/example")
set(root "${PROJECT_SOURCE_DIR}")

# ##############################################################################
# add component
add_hpx_component(
  example INTERNAL_FLAGS NOEXPORT
  HEADER_GLOB "${root}/*.h*"
  SOURCE_GLOB "${root}/example.c*"
  AUTOGLOB PLUGIN
)

# ##############################################################################
# add example executable
add_hpx_executable(
  example_client INTERNAL_FLAGS
  SOURCES example_client.cpp
  DEPENDENCIES example_component
)

