file(GLOB library_srcs *.cc)
include_directories(${HYPER_SOURCE_DIR}/include)
add_library (HyperCompiler SHARED ${library_srcs}) 

install(TARGETS HyperCompiler
		DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
		)
