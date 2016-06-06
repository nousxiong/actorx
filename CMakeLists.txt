#
# This file is part of the CMake build system for Actorx
#
# Copyright (c) 2016 Nous Xiong (348944179 at qq dot com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#
# See https://github.com/nousxiong/actorx for latest version.
#

cmake_minimum_required (VERSION 3.1.0 FATAL_ERROR)
project (actorx)

# Set -std=c++11.
set(CMAKE_CXX_STANDARD 11)

# Provide user options to customise the build process.
option (ACTORX_DEV "Actorx dev mode" OFF)
option (ACTORX_BUILD_EXAMPLE "Build Actorx examples" OFF)
option (ACTORX_BUILD_TEST "Build Actorx tests" OFF)
if (UNIX)
  option (ACTORX_STATIC "Build Actorx runtime static" OFF)
endif ()

if (WIN32)
  set (ACTORX_WINVER "0x0700" CACHE STRING "Windows version maro. Default is 0x0700 - win7, user can reset")
endif ()

# Set glibc.
if (ACTORX_STATIC)
  set (GLIBC_INCLUDEDIR "" CACHE PATH "Path to glibc include directory")
  set (GLIBC_LIBRARYDIR "" CACHE PATH "Path to glibc libraries directory")
  if (GLIBC_INCLUDEDIR)
    include_directories (${GLIBC_INCLUDEDIR})
  endif ()
  if (GLIBC_LIBRARYDIR)
    link_directories (${GLIBC_LIBRARYDIR})
  endif ()
endif ()

# Add the source and build tree to the search path for include actorx header files.
include_directories (${PROJECT_SOURCE_DIR}/include)
include_directories (${PROJECT_BINARY_DIR})

# Find threads
find_package (Threads)

set (CMAKE_VERBOSE_MAKEFILE true)

if (WIN32)
  # Set >= vc10 SAFESEH to NO
  if (${CMAKE_GENERATOR} MATCHES "Visual Studio 1*")
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO")
  endif ()

  # Set vc bigobj
  if (${CMAKE_GENERATOR} MATCHES "Visual Studio *")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
  endif ()
endif ()

add_subdirectory (libs)

# Configure a header file to pass some of the CMake settings to the source code.
configure_file (
  "${PROJECT_SOURCE_DIR}/user.hpp.in"
  "${PROJECT_BINARY_DIR}/actorx/user.hpp"
  )

install (FILES ${PROJECT_BINARY_DIR}/actorx/user.hpp DESTINATION include/actorx)
