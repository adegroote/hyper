set(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS true)

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

find_package(Boost 1.42 REQUIRED COMPONENTS unit_test_framework filesystem system thread serialization)
include_directories(${Boost_INCLUDE_DIRS})
message(STATUS "boost libraries "${Boost_LIBRARIES})
set(LIBS ${LIBS} ${Boost_LIBRARIES})



# add_subdirectory(include)
include_directories(${HYPER_SOURCE_DIR}/include)

set(model_test_ADDITIONAL_LIBS "hyper_network")

foreach(lib compiler network model)
  string(TOUPPER "${lib}_sources" THIS_LIB_SOURCES)
  string(TOLOWER "hyper_${lib}" HyperLibrary)
  if(EXISTS ${HYPER_SOURCE_DIR}/src/${lib})
    file(GLOB ${THIS_LIB_SOURCES} ${HYPER_SOURCE_DIR}/src/${lib}/*.cc)
    add_library (${HyperLibrary} SHARED ${${THIS_LIB_SOURCES}}) 
    install(TARGETS ${HyperLibrary}
      DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
      )
  endif(EXISTS ${HYPER_SOURCE_DIR}/src/${lib})
  string(TOUPPER "test_${lib}_sources" THIS_LIB_TEST_SOURCES)
  file(GLOB  ${THIS_LIB_TEST_SOURCES} ${HYPER_SOURCE_DIR}/test/test_${lib}_*.cc ${HYPER_SOURCE_DIR}/test/test_superviseur.cc)
  string(TOLOWER "test_hyper_${lib}" THIS_LIB_TEST_EXECUTABLE)
  add_executable(${THIS_LIB_TEST_EXECUTABLE} ${${THIS_LIB_TEST_SOURCES}})
  target_link_libraries(${THIS_LIB_TEST_EXECUTABLE} ${Boost_LIBRARIES} ${${lib}_test_ADDITIONAL_LIBS})
  if(EXISTS ${HYPER_SOURCE_DIR}/src/${lib})
	target_link_libraries(${THIS_LIB_TEST_EXECUTABLE} ${HyperLibrary})
  endif(EXISTS ${HYPER_SOURCE_DIR}/src/${lib})
  message(STATUS "add_test(${HyperLibrary} ${THIS_LIB_TEST_EXECUTABLE})")
  add_test(${HyperLibrary} ${THIS_LIB_TEST_EXECUTABLE})
  if(EXISTS ${HYPER_SOURCE_DIR}/src/${lib})
	add_custom_target(${lib} ALL DEPENDS ${HyperLibrary} ${THIS_LIB_TEST_EXECUTABLE})
  else()
	add_custom_target(${lib} ALL DEPENDS ${THIS_LIB_TEST_EXECUTABLE})
  endif()

endforeach(lib)

# install rules for include
install(DIRECTORY include/network include/model
		DESTINATION ${CMAKE_INSTALL_PREFIX}/include/hyper
		)

# hypercc rules
file(GLOB hyperc_sources ${HYPER_SOURCE_DIR}/src/hyperc/*.cc)
add_executable(hyperc ${hyperc_sources})
target_link_libraries(hyperc hyper_compiler ${Boost_FILESYSTEM_LIBRARY})
install(TARGETS hyperc DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

# hyperruntime rules
file(GLOB hyperruntime_sources ${HYPER_SOURCE_DIR}/src/hyperruntime/*.cc)
add_executable(hyperruntime ${hyperruntime_sources})
target_link_libraries(hyperruntime hyper_network ${Boost_LIBRARIES})
install(TARGETS hyperruntime DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
