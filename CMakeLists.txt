#######################################################################################################################
### Definitions																										###
#######################################################################################################################

cmake_minimum_required(VERSION 3.14 FATAL_ERROR)
set(LITMUS_PROJECT "litmus")
set(LOCAL_PROJECT ${LITMUS_PROJECT})
project(${LOCAL_PROJECT} VERSION 0.0.1 LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

option(LITMUS_EXAMPLES "build examples" FALSE)
option(LITMUS_DEVELOP_MODE "develop mode" FALSE)

if(LITMUS_DEVELOP_MODE)
	SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()



if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(
    FATAL_ERROR
      "In-source builds not allowed. Please make a new directory and run CMake from there."
  )
endif()

#######################################################################################################################
### Includes 																										###
#######################################################################################################################

list(APPEND LITMUS_INC_IMPL
	litmus
	expect

	details/cache
	)

list(APPEND LITMUS_INCLUDES
	${LITMUS_INC_IMPL}
	benchmark
	formatter
	section
	suite

	details/exceptions
	details/fixed_string
	details/runner
	details/source_location
	details/test_result
	details/verbosity
	)

list(APPEND LITMUS_SOURCES 
	${LITMUS_INC_IMPL}
	)

list(TRANSFORM LITMUS_INCLUDES PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/include/litmus/)
list(TRANSFORM LITMUS_INCLUDES APPEND .hpp)

list(TRANSFORM LITMUS_SOURCES PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/source/)
list(TRANSFORM LITMUS_SOURCES APPEND .cpp)

list(APPEND LITMUS_INCLUDES_DIRECTORIES include)

#######################################################################################################################
### Setup	 																										###
#######################################################################################################################

if(NOT TARGET strtype)
	add_subdirectory(extern/strtype)
else()
	message(STATUS "found already existing strtype library, using the provided one")
endif()


add_library(${LOCAL_PROJECT} STATIC ${LITMUS_SOURCES})
target_include_directories(${LOCAL_PROJECT} PUBLIC ${LITMUS_INCLUDES_DIRECTORIES})
target_compile_features(${LOCAL_PROJECT} PUBLIC cxx_std_20)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(${LOCAL_PROJECT} PUBLIC Threads::Threads strtype)
if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" OR "${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
	list(APPEND LITMUS_COMPILE_OPTIONS
		-Wno-unused-comparison
	)
	if(${LITMUS_DEVELOP_MODE})
		list(APPEND LITMUS_COMPILE_OPTIONS
			-Wall
			-Wextra
			-pedantic
			-Wno-unknown-pragmas
		)
		if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
			list(APPEND LITMUS_COMPILE_OPTIONS
				-g
			)
		endif()
	endif()

elseif(${LITMUS_DEVELOP_MODE})
	list(APPEND LITMUS_COMPILE_OPTIONS
		/permissive
	)
	if("${CONFIG}" MATCHES "Release")
		list(APPEND LITMUS_COMPILE_OPTIONS
			/WX
		)
	endif()
endif()

target_compile_options(${LOCAL_PROJECT} PUBLIC ${LITMUS_COMPILE_OPTIONS})

if(LITMUS_EXAMPLES)
	add_subdirectory(examples)
endif()