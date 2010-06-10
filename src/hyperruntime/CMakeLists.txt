file(GLOB hyperruntime_sources *.cc)
include_directories(${HYPER_SOURCE_DIR}/include)
link_directories(${HYPER_BINARY_DIR}/src/compiler)
add_executable(hyperruntime ${hyperruntime_sources})
target_link_libraries(hyperruntime ${Boost_SERIALIZATION_LIBRARY})
target_link_libraries(hyperruntime ${Boost_THREAD_LIBRARY})
target_link_libraries(hyperruntime ${Boost_SYSTEM_LIBRARY})
target_link_libraries(hyperruntime HyperNetwork)

install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/hyperruntime
		DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
	   )
