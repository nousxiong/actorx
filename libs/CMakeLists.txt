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
  if (MINGW)
    # MinGW for link ws2_32.
    set (LINK_LIBS ${LINK_LIBS} ws2_32)
  endif ()
endif ()

if (ACTORX_DEV)
  # Set up all files.
  file (GLOB_RECURSE ACTORX_HEADER_FILES ${ACTORX_HEADER_FILES} "${PROJECT_SOURCE_DIR}/actorx/*.hpp")
  file (GLOB_RECURSE ACTORX_SOURCE_FILES "${PROJECT_SOURCE_DIR}/libs/src/src.cpp")

  # Actorx library for dev
  add_library (actorx STATIC ${ACTORX_SOURCE_FILES} ${ACTORX_HEADER_FILES})
endif ()

# All headers need install include dependences.
install (
  DIRECTORY ${PROJECT_SOURCE_DIR}/actorx DESTINATION include
  PATTERN "impl" EXCLUDE
  PATTERN "CVS" EXCLUDE
  PATTERN ".svn" EXCLUDE
  PATTERN "lua" EXCLUDE
  )

if (ACTORX_INSTALL_DEPENDENCIES)
  install (
    DIRECTORY ${ASIO_INCLUDE_DIR}/ DESTINATION include
    PATTERN "impl" EXCLUDE
    PATTERN "CVS" EXCLUDE
    PATTERN ".svn" EXCLUDE
    PATTERN "lua" EXCLUDE
    )

  install (
    DIRECTORY ${GSL_INCLUDE_DIR}/ DESTINATION include
    PATTERN "impl" EXCLUDE
    PATTERN "CVS" EXCLUDE
    PATTERN ".svn" EXCLUDE
    PATTERN "lua" EXCLUDE
    )

  install (
    DIRECTORY ${SPDLOG_INCLUDE_DIR}/ DESTINATION include
    PATTERN "impl" EXCLUDE
    PATTERN "CVS" EXCLUDE
    PATTERN ".svn" EXCLUDE
    PATTERN "lua" EXCLUDE
    )

  if (ACTORX_HOOK)
    install (
      DIRECTORY ${XHOOK_INCLUDE_DIR}/ DESTINATION include
      PATTERN "impl" EXCLUDE
      PATTERN "CVS" EXCLUDE
      PATTERN ".svn" EXCLUDE
      PATTERN "lua" EXCLUDE
      )
  endif ()
endif ()

if (ACTORX_STATIC)
  set (ACTORX_LINK_PROPS "${ACTORX_LINK_PROPS} -static -static-libgcc -static-libstdc++")
endif ()

# Build examples.
if (ACTORX_BUILD_EXAMPLE)
  add_subdirectory (example)
endif ()

# Build tests.
if (ACTORX_BUILD_TEST)
  add_subdirectory (test)
endif ()
