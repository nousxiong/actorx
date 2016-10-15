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

if (ACTX_DEV)
  # Create vc filter
  if (MSVC)
    actorx_source_group(actorx)
  endif ()

  # Set up all files.
  file (GLOB_RECURSE ACTX_HEADER_FILES ${ACTX_HEADER_FILES} "${PROJECT_SOURCE_DIR}/actorx/*.hpp")
  file (GLOB_RECURSE ACTX_SOURCE_FILES "${PROJECT_SOURCE_DIR}/libs/src/src.cpp")

  # Actorx library for dev
  add_library (actorx STATIC ${ACTX_SOURCE_FILES} ${ACTX_HEADER_FILES})
endif ()

# All headers need install include dependences.
install (
  DIRECTORY ${PROJECT_SOURCE_DIR}/actorx DESTINATION include
  PATTERN "impl" EXCLUDE
  PATTERN "CVS" EXCLUDE
  PATTERN ".svn" EXCLUDE
  PATTERN "lua" EXCLUDE
  )

if (ACTX_INSTALL_DEPENDENCIES)
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

  install (
    DIRECTORY ${ADATA_INCLUDE_DIR}/ DESTINATION include
    PATTERN "impl" EXCLUDE
    PATTERN "CVS" EXCLUDE
    PATTERN ".svn" EXCLUDE
    PATTERN "lua" EXCLUDE
    )

  if (ACTX_HOOK)
    install (
      DIRECTORY ${XHOOK_INCLUDE_DIR}/ DESTINATION include
      PATTERN "impl" EXCLUDE
      PATTERN "CVS" EXCLUDE
      PATTERN ".svn" EXCLUDE
      PATTERN "lua" EXCLUDE
      )
  endif ()
endif ()

if (ACTX_STATIC)
  set (ACTX_LINK_PROPS "${ACTX_LINK_PROPS} -static -static-libgcc -static-libstdc++")
endif ()

# Build examples.
if (ACTX_BUILD_EXAMPLE)
  add_subdirectory (example)
endif ()

# Build tests.
if (ACTX_BUILD_TEST)
  add_subdirectory (test)
endif ()
