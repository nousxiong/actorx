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

# Build tests.
set (TESTS_LINK_LIBS ${LINK_LIBS})
file (GLOB_RECURSE ACTORX_UNIT_TEST_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file (GLOB_RECURSE ACTORX_UNIT_TEST_FILES ${ACTORX_UNIT_TEST_FILES} "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp")

add_executable (actorx_ut ${ACTORX_UNIT_TEST_FILES})

if (ACTORX_LINK_PROPS)
  set_target_properties (actorx_ut PROPERTIES LINK_FLAGS "${ACTORX_LINK_PROPS}")
endif ()

if (TESTS_LINK_LIBS)
  target_link_libraries (actorx_ut ${TESTS_LINK_LIBS})
endif ()

install (TARGETS actorx_ut RUNTIME DESTINATION bin)
