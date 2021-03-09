#######################################################################################################################
### Definitions																										###
#######################################################################################################################

cmake_minimum_required(VERSION 3.14 FATAL_ERROR)
SET(LITMUS_EXAMPLES ${LITMUS_PROJECT}_examples)
set(LOCAL_PROJECT ${LITMUS_EXAMPLES})
project(${LOCAL_PROJECT} VERSION 0.0.1 LANGUAGES CXX)

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(
    FATAL_ERROR
      "In-source builds not allowed. Please make a new directory and run CMake from there."
  )
endif()

#######################################################################################################################
### Includes 																										###
#######################################################################################################################

list(APPEND LITMUS_EXAMPLES_INC_SRC)

list(APPEND LITMUS_EXAMPLES_INC 
	${LITMUS_EXAMPLES_INC_SRC}
)

list(APPEND LITMUS_EXAMPLES_SRC
	${LITMUS_EXAMPLES_INC_SRC}
	basic_tests
	templated_generator
	)

list(TRANSFORM LITMUS_EXAMPLES_INC PREPEND include/examples/)
list(TRANSFORM LITMUS_EXAMPLES_INC APPEND .hpp)
list(TRANSFORM LITMUS_EXAMPLES_SRC PREPEND source/)
list(TRANSFORM LITMUS_EXAMPLES_SRC APPEND .cpp)


#######################################################################################################################
### Setup	 																										###
#######################################################################################################################

add_executable(${LOCAL_PROJECT} ${LITMUS_EXAMPLES_SRC})
target_include_directories(${LOCAL_PROJECT} PUBLIC include)
target_compile_features(${LOCAL_PROJECT} PUBLIC cxx_std_20)

if(develop_mode)		
	target_compile_options(${LOCAL_PROJECT} PUBLIC 
		$<$<CXX_COMPILER_ID:MSVC>:/permissive->
		$<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Release>>:/WX>
		$<$<CXX_COMPILER_ID:CLANG>:-Wno-error=terminate -Wall -Wextra -pedantic -Wno-unknown-pragmas>
		$<$<CXX_COMPILER_ID:GNU>:-Wno-error=terminate -Wall -Wextra -pedantic -Wno-unknown-pragmas -g>
		)
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(${LOCAL_PROJECT} PUBLIC ${LITMUS_PROJECT} Threads::Threads)
