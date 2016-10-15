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
set (CMAKE_CXX_STANDARD 11)

list (APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/libs/cmake")
include (util)

# Provide user options to customise the build process.
option (ACTX_DEV "Actorx dev mode" OFF)
option (ACTX_BUILD_EXAMPLE "Build Actorx examples" OFF)
option (ACTX_BUILD_TEST "Build Actorx tests" OFF)
option (ACTX_DEBUG "Open debug logger" OFF)
if (NOT MINGW)
  option (ACTX_HOOK "Hook system blocking calls" OFF)
endif ()
if (UNIX)
  option (ACTX_STATIC "Build Actorx runtime static" OFF)
endif ()

# Setup dependencies.
option (ACTX_INSTALL_DEPENDENCIES "Install all dependencies" ON)
set (ASIO_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/dependencies/asio" CACHE STRING "Include directory for asio")
set (GSL_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/dependencies/gsl" CACHE STRING "Include directory for gsl")
set (SPDLOG_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/dependencies/spdlog" CACHE STRING "Include directory for spdlog")
set (ADATA_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/dependencies/adata" CACHE STRING "Include directory for adata")
if (ACTX_HOOK)
  set (XHOOK_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/dependencies/xhook" CACHE STRING "Include directory for xhook")
endif ()

mark_as_advanced (ACTX_TEST_CSEGV)

if (WIN32)
  set (ACTX_WINVER "0x0700" CACHE STRING "Windows version maro. Default is 0x0700 - win7, user can reset")
endif ()

# Set glibc.
if (ACTX_STATIC)
  set (GLIBC_INCLUDE_DIR "" CACHE PATH "Path to glibc include directory")
  set (GLIBC_LIBRARY_DIR "" CACHE PATH "Path to glibc libraries directory")
  if (GLIBC_INCLUDE_DIR)
    include_directories (${GLIBC_INCLUDE_DIR})
  endif ()
  if (GLIBC_LIBRARY_DIR)
    link_directories (${GLIBC_LIBRARY_DIR})
  endif ()
endif ()

# Add the source and build tree to the search path for include actorx and dependencies header files.
include_directories (${PROJECT_SOURCE_DIR})
include_directories (${PROJECT_BINARY_DIR})
include_directories (${ASIO_INCLUDE_DIR})
include_directories (${GSL_INCLUDE_DIR})
include_directories (${SPDLOG_INCLUDE_DIR})
include_directories (${ADATA_INCLUDE_DIR})
if (ACTX_HOOK)
  include_directories (${XHOOK_INCLUDE_DIR})
endif ()

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
