file(GLOB library_srcs *.cc)
include_directories(${HYPER_SOURCE_DIR}/include)
add_library (HyperCompiler ${library_srcs}) 
