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