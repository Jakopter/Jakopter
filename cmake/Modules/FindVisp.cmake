
find_package(PackageHandleStandardArgs)

find_path(VISP_INCLUDE_DIR
	NAMES vpMath.h
	HINTS
	$ENV{VISPDIR}
	PATH_SUFFIXES include/visp include
	PATHS
	~/Library/Frameworks
	/Library/Frameworks
	/usr/local/include/visp
	/usr/include/visp
	/sw # Fink
	/opt/local # DarwinPorts/MacPorts
	/opt/csw # Blastwave/OpenCSW
	/opt
)

find_library(VISP_LIBRARY
	NAMES visp
	HINTS $ENV{VISPLIBDIR}
	PATH_SUFFIXES lib lib64 lib32
	PATHS
	/usr/local
	/usr
	/sw
	/opt/local
	/opt/csw
	/opt
)
find_package_handle_standard_args(VISP REQUIRED_VARS VISP_LIBRARY VISP_INCLUDE_DIR)