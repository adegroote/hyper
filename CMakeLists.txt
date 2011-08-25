set(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS true)

cmake_minimum_required(VERSION 2.6.4 FATAL_ERROR)

project(HYPER CXX)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/config/)

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

find_package(Boost 1.42 REQUIRED COMPONENTS unit_test_framework date_time filesystem system thread serialization program_options)
include_directories(${Boost_INCLUDE_DIRS})
message(STATUS "boost libraries "${Boost_LIBRARIES})
set(LIBS ${LIBS} ${Boost_LIBRARIES})

find_package(DL)
if (NOT DL_FOUND)
	message(FATAL Can't find librairy containing dlopen symbol)
endif()

# add_subdirectory(include)
include_directories(${HYPER_SOURCE_DIR}/include)

set(model_test_ADDITIONAL_LIBS "hyper_network;hyper_logic;hyper_compiler")
set(network_test_ADDITIONAL_LIBS "hyper_logic")

set(compiler_LIBS "${Boost_FILESYSTEM_LIBRARY}")
set(network_LIBS "${Boost_SYSTEM_LIBRARY};${Boost_DATE_TIME_LIBRARY};${Boost_THREAD_LIBRARY};${Boost_SERIALIZATION_LIBRARY}")
set(model_LIBS "${Boost_PROGRAM_OPTIONS_LIBRARY}")

# Workaround against the huge object file generated from this file in default mode
set_source_files_properties(src/compiler/expression_ast.cc COMPILE_FLAGS -Os)

set (HYPER_LIBS "")

foreach(lib compiler network logic model utils)
  string(TOUPPER "${lib}_sources" THIS_LIB_SOURCES)
  string(TOLOWER "hyper_${lib}" HyperLibrary)
  if(EXISTS ${HYPER_SOURCE_DIR}/src/${lib})
	list(APPEND HYPER_LIBS ${HyperLibrary})
    file(GLOB ${THIS_LIB_SOURCES} ${HYPER_SOURCE_DIR}/src/${lib}/*.cc)
    add_library (${HyperLibrary} SHARED ${${THIS_LIB_SOURCES}}) 
	target_link_libraries(${HyperLibrary}  ${${lib}_LIBS})
    install(TARGETS ${HyperLibrary}
      DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
      )
  endif(EXISTS ${HYPER_SOURCE_DIR}/src/${lib})
  string(TOUPPER "test_${lib}_sources" THIS_LIB_TEST_SOURCES)
  file(GLOB  ${THIS_LIB_TEST_SOURCES} ${HYPER_SOURCE_DIR}/test/test_${lib}_*.cc) 
  string(TOLOWER "test_hyper_${lib}" THIS_LIB_TEST_EXECUTABLE)
  set(${THIS_LIB_TEST_EXECUTABLE} "")
  foreach(file ${${THIS_LIB_TEST_SOURCES}})
	  get_filename_component(basename ${file} NAME_WE)
	  string(TOLOWER "hyper_${basename}" THIS_TEST_EXECUTABLE)
	  add_executable(${THIS_TEST_EXECUTABLE} ${file} ${HYPER_SOURCE_DIR}/test/test_superviseur.cc)
	  target_link_libraries(${THIS_TEST_EXECUTABLE} ${Boost_LIBRARIES} ${${lib}_test_ADDITIONAL_LIBS})
	  if(EXISTS ${HYPER_SOURCE_DIR}/src/${lib})
		target_link_libraries(${THIS_TEST_EXECUTABLE} ${HyperLibrary})
	  endif(EXISTS ${HYPER_SOURCE_DIR}/src/${lib})
	  add_test(${THIS_TEST_EXECUTABLE} ${THIS_TEST_EXECUTABLE})
	  set(${THIS_LIB_TEST_EXECUTABLE} ${${THIS_LIB_TEST_EXECUTABLE}} ${THIS_TEST_EXECUTABLE})
  endforeach()

  if(EXISTS ${HYPER_SOURCE_DIR}/src/${lib})
   add_custom_target(${lib} ALL DEPENDS ${HyperLibrary} ${${THIS_LIB_TEST_EXECUTABLE}})
  else()
   add_custom_target(${lib} ALL DEPENDS ${${THIS_TEST_EXECUTABLE}})
  endif()

endforeach(lib)

# install rules for include
install(DIRECTORY include/
		DESTINATION ${CMAKE_INSTALL_PREFIX}/include/hyper
		)

configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/HyperConfig.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/HyperConfig.cmake
	)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/HyperConfig.cmake share/HyperNode.cmake config/FindBoost.cmake
		DESTINATION share/hyper
)


# hypercc rules
file(GLOB hyperc_sources ${HYPER_SOURCE_DIR}/src/hyperc/*.cc)
add_executable(hyperc ${hyperc_sources})
target_link_libraries(hyperc hyper_compiler ${Boost_FILESYSTEM_LIBRARY} ${Boost_PROGRAM_OPTIONS_LIBRARY} ${Boost_SYSTEM_LIBRARY})
target_link_libraries(hyperc ${DL_LIBRARY})
install(TARGETS hyperc DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

# hyperruntime rules
file(GLOB hyperruntime_sources ${HYPER_SOURCE_DIR}/src/hyperruntime/*.cc)
add_executable(hyperruntime ${hyperruntime_sources})
target_link_libraries(hyperruntime hyper_network ${Boost_LIBRARIES})
install(TARGETS hyperruntime DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

# hyperlog rules
file(GLOB hyperlog_sources ${HYPER_SOURCE_DIR}/src/hyperlog/*.cc)
add_executable(hyperlog ${hyperlog_sources})
target_link_libraries(hyperlog ${HYPER_LIBS} ${Boost_LIBRARIES})
install(TARGETS hyperlog DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
