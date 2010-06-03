file(GLOB library_srcs *.cc)
include_directories(${HYPER_SOURCE_DIR}/include)
add_library (HyperNetwork SHARED ${library_srcs}) 

install(TARGETS HyperNetwork
		DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
		)
