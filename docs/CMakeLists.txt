# ##############################################################################
# SatNOGS-COMMS MCU software
#
# Copyright (C) 2024, Libre Space Foundation <http://libre.space>
#
# This program is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program.  If not, see <http://www.gnu.org/licenses/>.
#
# SPDX-License-Identifier: GNU General Public License v3.0 or later
# ##############################################################################

find_package(Doxygen)

option(BUILD_DOCS "Build documentation" ON)
if(DOXYGEN_FOUND AND BUILD_DOCS)

  set(DOXYGEN_IN_FILE Doxyfile.in)
  set(DOXYGEN_OUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
  configure_file(${DOXYGEN_IN_FILE} ${DOXYGEN_OUT_FILE} @ONLY)

  add_custom_target(
    docs
    COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT_FILE}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Generating API documentation with Doxygen"
    VERBATIM)
endif()
