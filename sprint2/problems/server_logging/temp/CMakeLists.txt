cmake_minimum_required(VERSION 3.11)

project(temp CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_TYPE Debug)


include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(temp
	src/main.cpp
	src/boost_log.h
	src/logger.h
	src/logger.cpp
)
target_link_libraries(temp PRIVATE Threads::Threads)
target_link_libraries(temp PRIVATE CONAN_PKG::boost)

