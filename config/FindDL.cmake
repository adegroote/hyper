# - Find dl functions
# This module finds dl libraries.
#
# It sets the following variables:
#  DL_FOUND       - Set to false, or undefined, if dl libraries aren't found.
#  DL_INCLUDE_DIR - The dl include directory.
#  DL_LIBRARY     - The dl library to link against.

include(CheckFunctionExists)

find_path(DL_INCLUDE_DIR NAMES dlfcn.h)
find_library(DL_LIBRARY NAMES dl)

if (DL_LIBRARY)
	set(DL_FOUND TRUE)
else ()
	# if dlopen can be found without linking in dl then,
	# dlopen is part of libc, so don't need to link extra libs.
	check_function_exists(dlopen DL_FOUND)
	set(DL_LIBRARY "")
endif()

if (DL_FOUND)
	# show which dl was found only if not quiet
	if (NOT DL_FIND_QUIETLY)
		message(STATUS "Found dl: ${DL_LIBRARY}")
	endif()
else ()
	# fatal error if dl is required but not found
	if(DL_FIND_REQUIRED)
		message(FATAL_ERROR "Could not find dl")
	endif(DL_FIND_REQUIRED)
endif ()

