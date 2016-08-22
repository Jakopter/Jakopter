# Jakopter
# Copyright © 2014 - 2015 Thibaud Hulin
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

find_package(PackageHandleStandardArgs)

find_path(VISP_INCLUDE_DIR
	NAMES vpMath.h
	HINTS
	$ENV{VISPDIR}
	PATH_SUFFIXES include/visp include
	PATHS
	~/Library/Frameworks
	/Library/Frameworks
	/usr/include/visp
	/usr/local/include/visp
	/sw # Fink
	/opt/local # DarwinPorts/MacPorts
	/opt/csw # Blastwave/OpenCSW
	/opt
)

find_library(VISP_CORE
	NAMES visp_core
	HINTS $ENV{VISPLIBDIR}
	PATH_SUFFIXES lib lib64 lib32
	PATHS
	/usr
	/usr/local
	/sw
	/opt/local
	/opt/csw
	/opt
)

find_library(VISP_DETECTION
	NAMES visp_detection
	HINTS $ENV{VISPLIBDIR}
	PATH_SUFFIXES lib lib64 lib32
	PATHS
	/usr
	/usr/local
	/sw
	/opt/local
	/opt/csw
	/opt
)

find_library(VISP_BLOB
	NAMES visp_blob
	HINTS $ENV{VISPLIBDIR}
	PATH_SUFFIXES lib lib64 lib32
	PATHS
	/usr
	/usr/local
	/sw
	/opt/local
	/opt/csw
	/opt
)

find_library(VISP_GUI
	NAMES visp_gui
	HINTS $ENV{VISPLIBDIR}
	PATH_SUFFIXES lib lib64 lib32
	PATHS
	/usr
	/usr/local
	/sw
	/opt/local
	/opt/csw
	/opt
)
set(VISP_LIBRARY ${VISP_CORE} ${VISP_BLOB} ${VISP_DETECTION} ${VISP_GUI})

find_package_handle_standard_args(VISP REQUIRED_VARS VISP_LIBRARY VISP_INCLUDE_DIR)