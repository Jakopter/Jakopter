CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT(jakopter)

FIND_PACKAGE(Threads REQUIRED)
FIND_PACKAGE(Lua51)
# Voir http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries#Using_external_libraries_that_CMake_doesn.27t_yet_have_modules_for pour sdl2 et libavcodec
# libavcodec : https://github.com/performous/composer/blob/master/cmake/FindAVCodec.cmake
# sdl2 : https://code.google.com/p/dolphin-emu/source/browse/CMakeTests/FindSDL2.cmake?name=async-dvd
INCLUDE_DIRECTORIES("include/")
INCLUDE_DIRECTORIES(${LUA_INCLUDE_DIR})

SET(CMAKE_BUILD_TYPE DEBUG)

SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY build/${CMAKE_BUILD_TYPE})

SET(
	CORE_SRC_FILES
	src/drone.c
	src/navdata.c
)

SET(
	VIDEO_SRC_FILES
	video.c
	video_decode.c
	video_display.c
	video_process.c
)

SET(
	LUA_SRC_FILES
	src/lua_bindings.c
)

SET(
	SRC_FILES
	${CORE_SRC_FILES}
)

IF(LUA51_FOUND)
	MESSAGE("Compiling with Lua")
	SET(
		SRC_FILES
		${SRC_FILES}
		${LUA_SRC_FILES}
	)
ELSE()
	MESSAGE("Compiling WITHOUT Lua")
ENDIF()

ADD_LIBRARY(
	jakopter SHARED 
	${SRC_FILES}	
)

TARGET_LINK_LIBRARIES(jakopter ${CMAKE_THREAD_LIBS_INIT})

