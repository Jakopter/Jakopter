# This file was adapted from openage project, an open source clone of Age of Empires II engine.
# Copyright 2014-2014 the openage authors. See copying.md for legal info.
# Copyright (C) 2014 openage authors <http://github.com/SFTtech/openage>

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
# Contact: <http://github.com/Jakopter/Jakopter>


find_package(PackageHandleStandardArgs)

if(APPLE)
	find_library(SDL2IMAGE_LIBRARY SDL2_image DOC "SDL2 library framework for MacOS X")
	find_path(SDL2IMAGE_INCLUDE_DIR SDL_image.h
		HINTS
		$ENV{SDL2DIR}
		PATH_SUFFIXES include/SDL2 include
		PATHS
		~/Library/Frameworks
		/Library/Frameworks
		/usr/local/include/SDL2
		/usr/include/SDL2
		/sw # Fink
		/opt/local # DarwinPorts/MacPorts
		/opt/csw # Blastwave
		/opt
		DOC "Include directory for SDL2_image under MacOS X"
	)
else(APPLE)
	if(UNIX)
		find_library(SDL2IMAGE_LIBRARY SDL2_image DOC "SDL2 library")
		find_path(SDL2IMAGE_INCLUDE_DIR SDL2/SDL_image.h DOC "Include directory for SDL2_image")
	endif()
endif()

# handle the QUIETLY and REQUIRED arguments and set OPENGL_FOUND to TRUE if
# all listed variables are TRUE
find_package_handle_standard_args(SDL2IMAGE REQUIRED_VARS SDL2IMAGE_LIBRARY SDL2IMAGE_INCLUDE_DIR)