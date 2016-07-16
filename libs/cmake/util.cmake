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

# Create source dirs.
macro (actorx_source_group prefix)
  set (ROOT_DIR ${PROJECT_SOURCE_DIR}/${prefix})
  # Glob all sources file inside directory ${ROOT_DIR}
  file (GLOB_RECURSE TMP_FILES
    ${ROOT_DIR}/*.hpp
    ${ROOT_DIR}/*.cpp
    )

  foreach (f ${TMP_FILES})
    # Get the path of the file relative to ${ROOT_DIR},
    # then alter it (not compulsory)
    file (RELATIVE_PATH SRCGR ${ROOT_DIR} ${f})
    set (SRCGR "${prefix}/${SRCGR}")

    # Extract the folder, ie remove the filename part
    string (REGEX REPLACE "(.*)(/[^/]*)$" "\\1" SRCGR ${SRCGR})

    # Source_group expects \\ (double antislash), not / (slash)
    string (REPLACE / \\ SRCGR ${SRCGR})
    source_group ("${SRCGR}" FILES ${f})
  endforeach ()
endmacro (actorx_source_group)
