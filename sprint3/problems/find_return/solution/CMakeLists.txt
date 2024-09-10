cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
conan_basic_setup(TARGETS)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_library(ModelLib STATIC 
	src/model.cpp src/model.h
	src/loot_generator.cpp src/loot_generator.h
	src/tagged.h
)

add_library(collision_detection_lib STATIC
	src/collision_detector.h
	src/collision_detector.cpp
	src/geom.h
)
target_link_libraries(collision_detection_lib PUBLIC CONAN_PKG::boost Threads::Threads)


add_executable(game_server 
	src/main.cpp
	src/cmd_parser.cpp src/cmd_parser.h
	src/http_server.cpp src/http_server.h
	src/sdk.h src/tagged.h
	src/boost_json.cpp
	src/json_loader.h src/json_loader.cpp
	src/request_handler.cpp src/request_handler.h
	src/player.cpp src/player.h
	src/app.cpp src/app.h
	src/logger.cpp src/logger.h
)
target_link_libraries(game_server CONAN_PKG::boost Threads::Threads ModelLib collision_detection_lib)

add_executable(game_server_tests
    tests/loot_generator_tests.cpp
)
target_link_libraries(game_server_tests PRIVATE CONAN_PKG::catch2 Threads::Threads ModelLib)

add_executable(collision_detection_tests
	tests/collision-detector-tests.cpp
)
target_link_libraries(collision_detection_tests CONAN_PKG::catch2 collision_detection_lib)

#include(CTest)
#include(${CONAN_BUILD_DIRS_CATCH2}/Catch.cmake)
#catch_discover_tests(collision_detection_tests)
