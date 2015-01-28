# FFmpeg library
#
# FFMPEG_FOUND - system has FFmpeg
# FFMPEG_INCLUDE_DIRS - the FFmpeg include directory
# FFMPEG_LIBRARIES - Link these to use FFmpeg
#
# This file is part of gvSIG (http://www.gvsig.org/web).
#
# Foobar is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Foobar is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Foobar.  If not, see <http://www.gnu.org/licenses/>.

IF(UNIX AND NOT APPLE)

	FIND_PACKAGE(PkgConfig REQUIRED)

	PKG_SEARCH_MODULE(AVCODEC libavcodec)
	PKG_SEARCH_MODULE(AVFORMAT libavformat)
	PKG_SEARCH_MODULE(AVUTIL libavutil)

ELSE(UNIX AND NOT APPLE)

	################################
	# AVCODEC includes & libraries
	FIND_PATH(AVCODEC_INCLUDE_DIRS avcodec.h ${DEPMAN_PATH}/include/ffmpeg)
	FIND_LIBRARY(AVCODEC_LIBRARIES NAMES avcodec avcodec-51)
	IF(AVCODEC_INCLUDE_DIRS AND AVCODEC_LIBRARIES)
		SET(AVCODEC_FOUND 1)
	ENDIF(AVCODEC_INCLUDE_DIRS AND AVCODEC_LIBRARIES)
	MARK_AS_ADVANCED(AVCODEC_INCLUDE_DIRS AVCODEC_LIBRARIES)

	################################
	# AVFORMAT includes & libraries
	FIND_PATH(AVFORMAT_INCLUDE_DIRS avformat.h ${DEPMAN_PATH}/include/ffmpeg)
	FIND_LIBRARY(AVFORMAT_LIBRARIES NAMES avformat avformat-51)
	IF(AVFORMAT_INCLUDE_DIRS AND AVFORMAT_LIBRARIES)
		SET(AVFORMAT_FOUND 1)
	ENDIF(AVFORMAT_INCLUDE_DIRS AND AVFORMAT_LIBRARIES)
	MARK_AS_ADVANCED(AVFORMAT_INCLUDE_DIRS AVFORMAT_LIBRARIES)

	################################
	# AVUTIL includes & libraries
	FIND_PATH(AVUTIL_INCLUDE_DIRS avutil.h ${DEPMAN_PATH}/include/ffmpeg)
	FIND_LIBRARY(AVUTIL_LIBRARIES NAMES avutil avutil-49)
	IF(AVUTIL_INCLUDE_DIRS AND AVUTIL_LIBRARIES)
		SET(AVUTIL_FOUND 1)
	ENDIF(AVUTIL_INCLUDE_DIRS AND AVUTIL_LIBRARIES)
	MARK_AS_ADVANCED(AVUTIL_INCLUDE_DIRS AVUTIL_LIBRARIES)

ENDIF(UNIX AND NOT APPLE)

################################
# FFMPEG includes & libraries
IF(AVCODEC_FOUND AND AVFORMAT_FOUND AND AVUTIL_FOUND)
	SET(FFMPEG_INCLUDE_DIRS
		${AVFORMAT_INCLUDE_DIRS}
		${AVCODEC_INCLUDE_DIRS}
		${AVUTIL_INCLUDE_DIRS}
	)
	SET(FFMPEG_LIBRARIES
		${AVFORMAT_LIBRARIES}
		${AVCODEC_LIBRARIES}
		${AVUTIL_LIBRARIES}
	)
	MARK_AS_ADVANCED(FFMPEG_INCLUDE_DIRS FFMPEG_LIBRARIES)
IF(NOT WIN32)
	SET(FFMPEG_FOUND 1)
ELSE(NOT WIN32)
	FIND_PACKAGE(INTTYPES REQUIRED)
	IF(INTTYPES_FOUND)
		SET(FFMPEG_INCLUDE_DIRS ${FFMPEG_INCLUDE_DIRS} ${INTTYPES_INCLUDE_DIR})
		SET(FFMPEG_FOUND 1)
	ENDIF(INTTYPES_FOUND)
ENDIF(NOT WIN32)
ENDIF(AVCODEC_FOUND AND AVFORMAT_FOUND AND AVUTIL_FOUND)

