cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Boost 1.78.0 REQUIRED)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

find_package(Threads REQUIRED)

add_library(game_server_lib STATIC
	src/http_server.cpp
	src/http_server.h
	src/sdk.h
	src/model.h
	src/model.cpp
	src/loot_generator.h
	src/loot_generator.cpp
	src/tagged.h
	src/tagged_uuid.cpp
	src/tagged_uuid.h
	src/boost_json.cpp
	src/json_loader.h
	src/json_loader.cpp
	src/request_handler.cpp
	src/request_handler.h
	src/extra_data.cpp
	src/extra_data.h
	src/collision_detector.cpp
	src/collision_detector.h
	src/model_serialization.h
	src/model_core.cpp
	src/model_core.h
	src/save_manager.h
	src/DB_manager.h
)

target_include_directories(game_server_lib PUBLIC CONAN_PKG::boost)
target_link_libraries(game_server_lib PUBLIC CONAN_PKG::boost Threads::Threads CONAN_PKG::libpq CONAN_PKG::libpqxx)

add_executable(game_server
	src/main.cpp
)

target_link_libraries(game_server game_server_lib)
