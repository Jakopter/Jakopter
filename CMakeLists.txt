# Jakopter
# Copyright © 2014-2015 Thibaud Rousseau, Thibaud Hulin, Hector Labanca, Jérémy Yziquel
# Copyright © 2015 ALF@INRIA
#
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
# Modules can use a different licence.

## CMake Settings ##
cmake_minimum_required(VERSION 2.8)

project(jakopter)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
set(JAKOPTER_DOC_DIR "${CMAKE_BINARY_DIR}/doc")

## User definitions ##
option(USE_VISP "Use visp display instead of sdl" OFF)
option(USE_CLANG "Use clang compiler instead of gcc" OFF)
option(USE_DOXYGEN "Generate code documentation" OFF)

## Packages ##
find_package(Threads REQUIRED)
find_package(SDL2)
find_package(SDL2TTF)
find_package(SDL2Image)
find_package(FFmpeg)
find_package(Lua)
find_package(Leap)
find_package(Curses)
find_package(Visp)
find_package(Vicon)
find_package(CURL)
find_package(Doxygen)

## Debug: print all declared variables
# get_cmake_property(_variableNames VARIABLES)
# foreach (_variableName ${_variableNames})
# 	message(STATUS "${_variableName}=${${_variableName}}")
# endforeach()


## Compiler Infos ##

if(USE_CLANG)
	set(CMAKE_C_COMPILER "/usr/bin/clang")
	set(CMAKE_CXX_COMPILER "/usr/bin/clang++")
endif()
include_directories("include/")

set(CMAKE_BUILD_TYPE DEBUG)

add_definitions("-Wall") # Not compatible with MSVC
list(APPEND CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftest-coverage -fprofile-arcs")

### C++11 ###
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-std=c++11" COMPILER_SUPPORTS_CXX11)
check_cxx_compiler_flag("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
	message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()


set(CMAKE_LIBRARY_OUTPUT_DIRECTORY build/${CMAKE_BUILD_TYPE})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY build/${CMAKE_BUILD_TYPE})

# Use rpath on Mac OS X
if(APPLE)
	cmake_policy(SET CMP0042 NEW)
endif()


## Core Source files ##

set(
	CORE_SRC_FILES
	src/drone.c
	src/navdata.c
	src/com_channel.c
	src/com_master.c
	src/user_input.c
	src/coords.c
)

set(
	VIDEO_SRC_FILES
	src/video.c
	src/video_queue.c
	src/video_decode.c
)

set(
	NETWORK_SRC_FILES
	src/network.c
)

set(
	SDL2_SRC_FILES
	src/video_display.c
)
set(
	VISP_SRC_FILES
	src/visp.cpp
)

set(
	LEAP_SRC_FILES
	src/leap.cpp
)

set(
	VICON_SRC_FILES
	src/vicon.cpp
)

set(
	KEYB_SRC_FILES
	src/cmd.c
)

set(
	LUA_SRC_FILES
	src/lua_bindings.c
)

set(
	SRC_FILES
	${CORE_SRC_FILES}
)

## Include Packages header and source files ##


if(AVCODEC_FOUND AND SDL2_FOUND AND SDL2TTF_FOUND AND SDL2IMAGE_FOUND AND NOT USE_VISP)
	message(STATUS "Compiling with SDL2")
	include_directories(${SDL2_INCLUDE_DIR} ${SDL2TTF_INCLUDE_DIR} ${SDL2IMAGE_INCLUDE_DIR})
	set(
		SRC_FILES
		${SRC_FILES}
		${SDL2_SRC_FILES}
	)

	set(WITH_VIDEO ON)
else()
	message(WARNING "Compiling WITHOUT SDL2")
	set(WITH_VIDEO OFF)
endif()

if(AVCODEC_FOUND AND VISP_FOUND AND USE_VISP)
	message(STATUS "Compiling with libvisp")
	include_directories(${VISP_INCLUDE_DIR})
	set(
		SRC_FILES
		${SRC_FILES}
		${VISP_SRC_FILES}
	)
	set(WITH_VIDEO ON)
elseif(USE_VISP)
	message(WARNING "Compiling WITHOUT visp")
	set(WITH_VIDEO OFF)
endif()

if(AVCODEC_FOUND AND WITH_VIDEO)
	message(STATUS "Compiling with libavcodec")
	include_directories(${AVCODEC_INCLUDE_DIRS})
	set(
		SRC_FILES
		${SRC_FILES}
		${VIDEO_SRC_FILES}
	)

	add_definitions(${AVCODEC_DEFINITIONS})
else()
	message(WARNING "Compiling WITHOUT libavcodec")
endif()

if(LEAP_FOUND)
	message(STATUS "Compiling with leap motion support")
	include_directories(${LEAP_INCLUDE_DIRS})
else()
	message(WARNING "Compiling WITHOUT leap motion support")
endif()

if(VICON_FOUND)
	message(STATUS "Compiling with vicon motion capture support")
	include_directories(${VICON_INCLUDE_DIRS})

	set(WITH_COORDS ON)
else()
	message(WARNING "Compiling WITHOUT vicon motion capture support")
	set(WITH_COORDS OFF)
endif()

if(CURSES_FOUND)
	message(STATUS "Compiling keyboard command interface")
	include_directories(${CURSES_INCLUDE_DIRS})
else()
	message(WARNING "Compiling WITHOUT keyboard command interface")
endif()

if(CURL_FOUND)
	message(STATUS "Compiling with network interface")
	include_directories(${CURL_INCLUDE_DIRS})
	set(
		SRC_FILES
		${SRC_FILES}
		${NETWORK_SRC_FILES}
	)

	set(WITH_NETWORK ON)
else()
	message(WARNING "Compiling WITHOUT network interface")
	set(WITH_NETWORK OFF)
endif()

if(LUA_FOUND AND LUA_VERSION_MAJOR EQUAL 5 AND
	(LUA_VERSION_MINOR EQUAL 1 OR LUA_VERSION_MINOR EQUAL 2 OR LUA_VERSION_MINOR EQUAL 3))
	message(STATUS "Compiling with Lua")
	include_directories(${LUA_INCLUDE_DIR})
	set(
		SRC_FILES
		${SRC_FILES}
		${LUA_SRC_FILES}
	)
else()
	message(WARNING "Compiling WITHOUT Lua")
endif()


configure_file(${CMAKE_SOURCE_DIR}/include/common.h.in ${CMAKE_SOURCE_DIR}/include/common.h)

## Executables ##

add_library(
	jakopter SHARED
	${SRC_FILES}
)
set_target_properties(jakopter PROPERTIES C_VISIBILITY_PRESET hidden)
set_target_properties(jakopter PROPERTIES CXX_VISIBILITY_PRESET hidden)

if(LEAP_FOUND)
	add_executable(
		leap
		${LEAP_SRC_FILES}
	)
endif()

if(WITH_COORDS AND VICON_FOUND)
	add_executable(
		vicon
		${VICON_SRC_FILES}
	)
endif()

## Linking libraries ##


if(CURSES_FOUND)
	add_executable(
		keyb_control
		${KEYB_SRC_FILES}
	)
	target_link_libraries(keyb_control ${CURSES_LIBRARIES})
endif()


target_link_libraries(jakopter ${CMAKE_THREAD_LIBS_INIT})

if(LUA_FOUND)
	target_link_libraries(jakopter ${LUA_LIBRARIES})
endif()

if(WITH_VIDEO)
	target_link_libraries(jakopter ${AVCODEC_LIBRARIES} ${AVUTIL_LIBRARIES})
endif()

if(WITH_VIDEO AND SDL2_FOUND AND SDL2TTF_FOUND AND SDL2IMAGE_FOUND AND NOT USE_VISP)
	target_link_libraries(jakopter ${SDL2_LIBRARY} ${SDL2TTF_LIBRARY} ${SDL2IMAGE_LIBRARY})
endif()

if(WITH_VIDEO AND VISP_FOUND AND USE_VISP)
	target_link_libraries(jakopter ${VISP_LIBRARY})
endif()

if(WITH_NETWORK AND CURL_FOUND)
	target_link_libraries(jakopter ${CURL_LIBRARIES})
endif()

if(LEAP_FOUND)
	target_link_libraries(leap ${LEAP_LIBRARIES})
endif()

if(WITH_COORDS AND VICON_FOUND)
	target_link_libraries(vicon ${CMAKE_THREAD_LIBS_INIT} ${VICON_LIBRARIES})
endif()

if(USE_DOXYGEN AND DOXYGEN_FOUND)
	message(STATUS "Generate doc with Doxygen")
	configure_file(${CMAKE_SOURCE_DIR}/doc/config-doxygen.in
		${JAKOPTER_DOC_DIR}/config-doxygen
		@ONLY)
	configure_file(${CMAKE_SOURCE_DIR}/doc/mainpage.doc.in
		${JAKOPTER_DOC_DIR}/mainpage.doc
		@ONLY)
else()
	message(WARNING "Doc NOT generated")
endif()