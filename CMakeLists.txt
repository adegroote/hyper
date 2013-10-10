set(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS true)

cmake_minimum_required(VERSION 2.6.4 FATAL_ERROR)

project(HYPER CXX)

set(PACKAGE_NAME hyper)
set(HYPER_MAJOR_VERSION 0)
set(HYPER_MINOR_VERSION 13)
set(HYPER_PATCH_VERSION 0)
set(HYPER_VERSION "${HYPER_MAJOR_VERSION}.${HYPER_MINOR_VERSION}.${HYPER_PATCH_VERSION}")
set(PACKAGE_VERSION "${HYPER_VERSION}") 

option(BUILD_DOC "Build and install documentation (Require Sphinx)" OFF)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/config/)

enable_testing()

enable_language(C)

if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
endif()


## RPATH settings
# use, i.e. don't skip the full RPATH for the build tree
SET(CMAKE_SKIP_BUILD_RPATH  FALSE)

# when building, don't use the install RPATH already
# (but later on when installing)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE) 

SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

# add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)


# the RPATH to be used when installing, but only if it's not a system directory
LIST(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
IF("${isSystemDir}" STREQUAL "-1")
	   SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
ENDIF("${isSystemDir}" STREQUAL "-1")

include(CheckIncludeFile)

find_package(Boost 1.46.1 REQUIRED COMPONENTS unit_test_framework date_time filesystem system thread serialization program_options)
include_directories(${Boost_INCLUDE_DIRS})
message(STATUS "boost libraries "${Boost_LIBRARIES})
set(LIBS ${LIBS} ${Boost_LIBRARIES})

configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/include/version.hh.in
	${CMAKE_CURRENT_BINARY_DIR}/version.hh
	@ONLY
	)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/version.hh DESTINATION include/hyper)

# add_subdirectory(include)
include_directories(${HYPER_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(network_test_ADDITIONAL_LIBS "hyper_logic")

set(compiler_LIBS "${Boost_FILESYSTEM_LIBRARY}")
set(network_LIBS "${Boost_SYSTEM_LIBRARY};${Boost_DATE_TIME_LIBRARY};${Boost_THREAD_LIBRARY};${Boost_SERIALIZATION_LIBRARY};hyper_logic")
set(model_LIBS "${Boost_PROGRAM_OPTIONS_LIBRARY};hyper_network;hyper_logic;hyper_compiler")

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
      add_test(${THIS_TEST_EXECUTABLE}  ${THIS_TEST_EXECUTABLE})
      set_tests_properties(${THIS_TEST_EXECUTABLE} PROPERTIES ENVIRONMENT "HYPER_ROOT_ADDR=localhost:4242")
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
		FILES_MATCHING PATTERN "*.hh"
		)

configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/HyperConfig.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/HyperConfig.cmake
	@ONLY
	)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/HyperConfig.cmake share/HyperNode.cmake config/FindBoost.cmake
		DESTINATION share/hyper
)


# hypercc rules
file(GLOB hyperc_sources ${HYPER_SOURCE_DIR}/src/hyperc/*.cc)
add_executable(hyperc ${hyperc_sources})
target_link_libraries(hyperc hyper_compiler ${Boost_FILESYSTEM_LIBRARY} ${Boost_PROGRAM_OPTIONS_LIBRARY} ${Boost_SYSTEM_LIBRARY})
target_link_libraries(hyperc ${CMAKE_DL_LIBS})
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

# Create a symlink test files
ADD_CUSTOM_TARGET(
	link_test ALL
	COMMAND ${CMAKE_COMMAND} -E create_symlink ${HYPER_SOURCE_DIR}/test/example.ability ${HYPER_BINARY_DIR}/example.ability
	COMMAND ${CMAKE_COMMAND} -E create_symlink ${HYPER_SOURCE_DIR}/test/other.ability ${HYPER_BINARY_DIR}/other.ability
)


######################################################################
# Documentation
######################################################################

IF (BUILD_DOC)

CONFIGURE_FILE(
    ${CMAKE_CURRENT_SOURCE_DIR}/doc/conf.py.in
    ${CMAKE_CURRENT_BINARY_DIR}/doc/conf.py
)


FIND_PROGRAM(SPHINX_BUILD NAMES sphinx-build)
IF (NOT SPHINX_BUILD)
	MESSAGE(FATAL_ERROR "Can't find sphinx-build : will not build the documentation")
ELSE()
	SET (CMAKE_DOC_OUT ${CMAKE_CURRENT_BINARY_DIR}/doc)
	SET (SPHINX_OUTPUT ${CMAKE_DOC_OUT}/html)
	SET (SPHINX_SRC ${CMAKE_CURRENT_SOURCE_DIR}/doc)
	SET (SPHINX_LATEX_OUTPUT ${CMAKE_DOC_OUT}/tex)
	SET (SPHINX_MAN_OUTPUT ${CMAKE_DOC_OUT}/man)

	ADD_CUSTOM_TARGET(
		doc ALL
		COMMAND ${SPHINX_BUILD} -b html
                            -c ${CMAKE_DOC_OUT}
								${SPHINX_SRC}/hyper
							    ${SPHINX_OUTPUT}
	)

	FIND_PROGRAM(GZIP NAMES gzip)
	IF (NOT GZIP)
		MESSAGE(STATUS "Can't find gzip : will not build man pages")
	ELSE()
		ADD_CUSTOM_TARGET(man ALL COMMAND ${SPHINX_BUILD} -b man
                                        -c ${CMAKE_DOC_OUT}
										   ${SPHINX_SRC}/man
        								   ${SPHINX_MAN_OUTPUT} &&
						  ${GZIP} -f ${SPHINX_MAN_OUTPUT}/*.1)

		INSTALL(FILES ${SPHINX_MAN_OUTPUT}/hyperc.1.gz 
				DESTINATION ${CMAKE_INSTALL_PREFIX}/share/man/man1
		)
	ENDIF()

	INSTALL(DIRECTORY ${SPHINX_OUTPUT}
			DESTINATION ${CMAKE_INSTALL_PREFIX}/share/doc/hyper
			REGEX .doctrees.* EXCLUDE
			REGEX _sources.* EXCLUDE
			REGEX .*.inv EXCLUDE
			REGEX .buildinfo EXCLUDE
			)
ENDIF()
ENDIF()

#########################################################################
### Uninstall ###
#########################################################################
configure_file("${CMAKE_HOME_DIRECTORY}/config/cmake_uninstall.cmake.in" "cmake_uninstall.cmake" IMMEDIATE @ONLY)
add_custom_target(uninstall "${CMAKE_COMMAND}" -P "cmake_uninstall.cmake")
