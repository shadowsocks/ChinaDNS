# UCI_FOUND - true if library and headers were found
# UCI_INCLUDE_DIRS - include directories
# UCI_LIBRARIES - library directories

find_package(PkgConfig)
pkg_check_modules(PC_UCI QUIET uci)

find_path(UCI_INCLUDE_DIR uci.h
	HINTS ${PC_UCI_INCLUDEDIR} ${PC_UCI_INCLUDE_DIRS})

find_library(UCI_LIBRARY NAMES uci libuci
	HINTS ${PC_UCI_LIBDIR} ${PC_UCI_LIBRARY_DIRS})

set(UCI_LIBRARIES ${UCI_LIBRARY})
set(UCI_INCLUDE_DIRS ${UCI_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(UCI DEFAULT_MSG UCI_LIBRARY UCI_INCLUDE_DIR)

mark_as_advanced(UCI_INCLUDE_DIR UCI_LIBRARY)
