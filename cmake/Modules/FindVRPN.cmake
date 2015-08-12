# Jakopter
#
# Based on the FindFMODEX.cmake of Team Pantheon.
# Copyright © 2015 Alexandre Leonardi
# Copyright © 2015 INRIA
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 3 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library. If not, see <http://www.gnu.org/licenses/>.
#
#
# Finds the libraries and header files for the VRPN SDK which is used to
# track the VRPN Armband
# This module defines
# VRPN_FOUND       - VRPN was found
# VRPN_INCLUDE_DIR - Directory containing VRPN header files
# VRPN_LIBRARY     - Library name of VRPN library
# VRPN_INCLUDE_DIRS- Directories required by CMake convention
# VRPN_LIBRARIES   - Libraries required by CMake convention
#

# Don't be verbose if previously run successfully
if(VRPN_INCLUDE_DIR AND VRPN_LIBRARY)
	set(VRPN_FIND_QUIETLY TRUE)
endif(VRPN_INCLUDE_DIR AND VRPN_LIBRARY)

# Set locations to search
if(UNIX)
	set(VRPN_INCLUDE_SEARCH_DIRS
		#set VRPNDIR in your env to use a personnal repository
		/usr/include
		/usr/local/include
		/opt/include INTERNAL
	)
	set(VRPN_LIBRARY_SEARCH_DIRS
		#set VRPNLIBDIR in your env to use a personnal repository
		/usr/lib
		/usr/lib/x86_64-linux-gnu
		/usr/lib/vrpn
		/usr/lib64
		/usr/local/lib
		/usr/local/lib64 INTERNAL
	)
	set(VRPN_INC_DIR_SUFFIXES PATH_SUFFIXES VRPN)
else(UNIX)
	#WIN32
	set(VRPN_INC_DIR_SUFFIXES PATH_SUFFIXES inc)
	set(VRPN_LIB_DIR_SUFFIXES PATH_SUFFIXES lib)
endif(UNIX)

# Set name of the VRPN library to use
if(UNIX)
	set(VRPN_LIBRARY_NAME libvrpn.a)
	set(QUAT_LIBRARY_NAME libquat.a)
	set(PTHREAD_LIBRARY_NAME libpthread.so)
else(UNIX)
	set(VRPN_LIBRARY_NAME libvrpn.lib)
	set(QUAT_LIBRARY_NAME libquat.lib)
	set(PTHREAD_LIBRARY_NAME libpthread.dylib)
endif(UNIX)

if(NOT VRPN_FIND_QUIETLY)
	message(STATUS "Checking for VRPN")
endif(NOT VRPN_FIND_QUIETLY)

# Search for header files
find_path(VRPN_INCLUDE_DIR
	NAMES vrpn_Button.h vrpn_Analog.h
	HINTS $ENV{VRPNDIR}
	PATHS ${VRPN_INCLUDE_SEARCH_DIRS}
	PATH_SUFFIXES ${VRPN_INC_DIR_SUFFIXES}
)

find_library(VRPN_LIBRARY ${VRPN_LIBRARY_NAME}
	HINTS $ENV{VRPNLIBDIR}
	PATHS ${VRPN_LIBRARY_SEARCH_DIRS}
	PATH_SUFFIXES ${VRPN_LIB_DIR_SUFFIXES}
)

find_library(QUAT_LIBRARY ${QUAT_LIBRARY_NAME}
	HINTS $ENV{VRPNLIBDIR}
	PATHS ${VRPN_LIBRARY_SEARCH_DIRS}
	PATH_SUFFIXES ${VRPN_LIB_DIR_SUFFIXES}
)

find_library(PTHREAD_LIBRARY ${PTHREAD_LIBRARY_NAME}
	HINTS $ENV{VRPNLIBDIR}
	PATHS ${VRPN_LIBRARY_SEARCH_DIRS}
	PATH_SUFFIXES ${VRPN_LIB_DIR_SUFFIXES}
)

set(VRPN_INCLUDE_DIR ${VRPN_INCLUDE_DIR} CACHE STRING
	"Directory containing  VRPN header files for VRPN")
set(VRPN_LIBRARY ${VRPN_LIBRARY} CACHE STRING "Library name of VRPN library for VRPN")

set(VRPN_INCLUDE_DIRS ${VRPN_INCLUDE_DIR})
if(VRPN_LIBRARY AND QUAT_LIBRARY AND PTHREAD_LIBRARY)
	set(VRPN_LIBRARIES
		${VRPN_LIBRARY}
		${QUAT_LIBRARY}
		${PTHREAD_LIBRARY}
	)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(VRPN DEFAULT_MSG VRPN_LIBRARIES VRPN_INCLUDE_DIR)

mark_as_advanced(VRPN_INCLUDE_DIR VRPN_LIBRARIES)

