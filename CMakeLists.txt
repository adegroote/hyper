# $Id: $ #
# ----------------------------------------------------------------------------
#  Root CMake file for Jafar
#
#    From the off-tree build directory, invoke:
#      $ cmake <JAFAR_DIR>
#
# ----------------------------------------------------------------------------

set(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS true)

# it MUST go before PROJECT(Jafar) in order to work
if (NOT CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR} CACHE INTERNAL "" FORCE)
endif()

cmake_minimum_required(VERSION 2.6.4 FATAL_ERROR)

project(HYPER CXX)

#set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Byuilding type, choose either Debug or Release")
#set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE LIST)

enable_testing()

enable_language(C)

# ----------------------------------------------------------------------------
# Indicate CMake 2.7 and above that we don't want to mix relative
#  and absolute paths in linker lib lists.
# Run "cmake --help-policy CMP0003" for more information.
# ----------------------------------------------------------------------------
if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
endif()

#------------------------------------------------------------------------------
# External dependencies: 
#------------------------------------------------------------------------------

include(CheckIncludeFile)

find_package(Boost 1.34 REQUIRED COMPONENTS unit_test_framework filesystem system thread)
set(BOOST_FOUND ${Boost_FOUND})
include_directories(${Boost_INCLUDE_DIRS})
message(STATUS "boost libraries "${Boost_LIBRARIES})
set(LIBS ${LIBS} ${Boost_LIBRARIES})
set(HAVE_BOOST Boost_FOUND)
set(LIBS_MAP_BOOST "Boost")

#-------------------------------------------------------------------------------
# Build executable and libraries
#-------------------------------------------------------------------------------

add_subdirectory(src)

#-------------------------------------------------------------------------------
# Tests
#-------------------------------------------------------------------------------

add_subdirectory(test)

