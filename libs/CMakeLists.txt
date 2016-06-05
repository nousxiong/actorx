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

set (LINK_LIBS ${CMAKE_THREAD_LIBS_INIT})
if (UNIX)
  set (LINK_LIBS ${LINK_LIBS} dl)
elseif (WIN32)
  set (LINK_LIBS ${LINK_LIBS} Dbghelp)
endif ()

# Not only x64 linux need this, but all linux gcc, we need this.
if (CMAKE_COMPILER_IS_GNUCXX AND NOT MINGW)
  add_definitions (-fPIC)
endif ()

if (ACTORX_DEV)
  # Set up all files.
  file (GLOB_RECURSE ACTORX_HEADER_FILES ${ACTORX_HEADER_FILES} "${PROJECT_SOURCE_DIR}/include/*.hpp")
  file (GLOB_RECURSE ACTORX_HEADER_FILES ${ACTORX_HEADER_FILES} "${PROJECT_SOURCE_DIR}/include/*.ipp")
  file (GLOB_RECURSE ACTORX_HEADER_FILES ${ACTORX_HEADER_FILES} "${PROJECT_SOURCE_DIR}/include/*.h")
  file (GLOB_RECURSE ACTORX_SOURCE_FILES "${PROJECT_SOURCE_DIR}/libs/src/src.cpp")

  # Actorx library for dev
  add_library (actorx STATIC ${ACTORX_SOURCE_FILES} ${ACTORX_HEADER_FILES})
endif ()

# All headers need install include dependences.
install (
  DIRECTORY ${PROJECT_SOURCE_DIR}/include DESTINATION .
  PATTERN "impl" EXCLUDE
  PATTERN "CVS" EXCLUDE
  PATTERN ".svn" EXCLUDE
  PATTERN "lua" EXCLUDE
  )

if (ACTORX_STATIC)
  set (ACTORX_LINK_PROP "${ACTORX_LINK_PROP} -static -static-libgcc -static-libstdc++")
endif ()

# Build examples.
if (ACTORX_BUILD_EXAMPLE)
  add_subdirectory (example)
endif ()

# Build tests.
if (ACTORX_BUILD_TEST)
  add_subdirectory (test)
endif ()
