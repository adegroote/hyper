file(GLOB hyperc_sources *.cc)
include_directories(${HYPER_SOURCE_DIR}/include)
link_directories(${HYPER_BINARY_DIR}/src/compiler)
add_executable(hyperc ${hyperc_sources})
target_link_libraries(hyperc ${Boost_FILESYSTEM_LIBRARY})
target_link_libraries(hyperc HyperCompiler)

install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/hyperc
		DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
	   )

