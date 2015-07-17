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

find_path(VICON_INCLUDE_DIRS
	NAMES Client.h
	HINTS
	$ENV{VICONDIR}
	PATH_SUFFIXES include/vicon include
	PATHS
	~/Library/Frameworks
	/Library/Frameworks
	/usr/local
	/usr
	/sw # Fink
	/opt/local # DarwinPorts/MacPorts
	/opt/csw # Blastwave/OpenCSW
	/opt
)

find_library(VICON_LIBRARIES
	NAMES
	ViconDataStreamSDK_CPP
#	DebugServices
	HINTS $ENV{VICONLIBDIR}
	PATH_SUFFIXES lib lib64 lib32
	PATHS
	/usr/local
	/usr
	/sw
	/opt/local
	/opt/csw
	/opt
)

find_package_handle_standard_args(VICON REQUIRED_VARS VICON_LIBRARIES VICON_INCLUDE_DIRS)