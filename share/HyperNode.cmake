macro(HYPER_NODE_RUBY_CLIENT node)
	find_package(SWIG)
	find_package(Ruby)
	include(${SWIG_USE_FILE})

	set(base_directory src/${node})

	include_directories(${RUBY_INCLUDE_PATH})
	execute_process(COMMAND ${RUBY_EXECUTABLE} -r rbconfig -e "print Config::CONFIG['sitearch']"
				OUTPUT_VARIABLE RUBY_SITEARCH)
	string(REGEX MATCH "[0-9]+\\.[0-9]+" RUBY_VERSION "${RUBY_VERSION}")

	add_custom_command(
		OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/${node}_wrap.cpp
		COMMAND ${SWIG_EXECUTABLE}
		ARGS -c++ -Wall -v -ruby -prefix "hyper::" -I${HYPER_INCLUDE_DIRS} -I${Boost_INCLUDE_DIRS} -I${RUBY_INCLUDE_DIR} -o ${CMAKE_CURRENT_BINARY_DIR}/${node}_wrap.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/${node}.i
		MAIN_DEPENDENCY src/${node}.i
		DEPENDS ${base_directory}/export.hh
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	)
	add_library(${node}_ruby_wrap SHARED ${CMAKE_CURRENT_BINARY_DIR}/${node}_wrap.cpp ${base_directory}/export.cc)
	target_link_libraries(${node}_ruby_wrap stdc++ ${RUBY_LIBRARY})
	target_link_libraries(${node}_ruby_wrap ${HYPER_LIBS})
	set_target_properties(${node}_ruby_wrap 
						  PROPERTIES OUTPUT_NAME ${node}
						  PREFIX "")
	install(TARGETS ${node}_ruby_wrap
			DESTINATION lib/ruby/${RUBY_VERSION}/${RUBY_SITEARCH}/hyper)
endmacro(HYPER_NODE_RUBY_CLIENT)

macro(HYPER_NODE node)
	project(HYPER_ABILITY_${node} CXX)
	enable_language(C)
	include(CheckIncludeFile)

	find_package(Boost 1.42 REQUIRED COMPONENTS system thread serialization filesystem date_time)
	set(BOOST_FOUND ${Boost_FOUND})
	include_directories(${Boost_INCLUDE_DIRS})
	message(STATUS "boost libraries "${Boost_LIBRARIES})

	set(base_directory src/${node})

	if (EXISTS ${CMAKE_SOURCE_DIR}/${base_directory}/CMakeLists.txt)
		include(${CMAKE_SOURCE_DIR}/${base_directory}/CMakeLists.txt)
	endif()
	
	include_directories(${CMAKE_SOURCE_DIR}/src)
	include_directories(${HYPER_INCLUDE_DIRS})

	if (EXISTS ${CMAKE_SOURCE_DIR}/${base_directory}/import.cc)
		file(GLOB funcs ${base_directory}/funcs/*.cc)
		add_library(hyper_${node} SHARED ${funcs} ${base_directory}/import.cc ${USER_SRCS})
		target_link_libraries(hyper_${node} ${USER_LIBS})
		install(TARGETS hyper_${node} DESTINATION lib/hyper/)
		install(FILES ${base_directory}/funcs.hh ${base_directory}/import.hh
				DESTINATION include/hyper/${node})
	endif()

	set(SRC src/main.cc)
	file(GLOB tasks ${base_directory}/tasks/*cc)
	set(SRC ${tasks} ${SRC})
	file(GLOB recipes ${base_directory}/recipes/*cc)
	set(SRC ${recipes} ${SRC})

	link_directories(${HYPER_LIB_DIR})
	add_executable(${node} ${SRC})
	set_property(TARGET ${node} PROPERTY OUTPUT_NAME hyper_${node})
	install(TARGETS ${node}
			 DESTINATION ${HYPER_ROOT}/bin)
	install(FILES ${base_directory}/types.hh
			 DESTINATION ${HYPER_ROOT}/include/hyper/${node})
	install(FILES ${node}.ability
			 DESTINATION ${HYPER_ROOT}/share/hyper)
	if (EXISTS ${CMAKE_SOURCE_DIR}/${base_directory}/import.cc)
		target_link_libraries(${node} hyper_${node})
	endif()
	target_link_libraries(${node} ${Boost_LIBRARIES})

	target_link_libraries(${node} ${HYPER_LIBS})

	hyper_node_ruby_client(${node})
endmacro(HYPER_NODE)

