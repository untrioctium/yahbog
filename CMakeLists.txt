cmake_minimum_required(VERSION 3.20)
cmake_policy(SET CMP0135 NEW)
cmake_policy(SET CMP0069 NEW)
project(yahbog)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
endif()

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

include(FetchContent)

# nlohmann/json
FetchContent_Declare(
    json 
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
    URL_HASH SHA256=42f6e95cad6ec532fd372391373363b62a14af6d771056dbfc86160e6dfff7aa)
FetchContent_MakeAvailable(json)

# zlib
set(ZLIB_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
set(ZLIB_COMPAT ON CACHE BOOL "" FORCE)
FetchContent_Declare(
  zlib
  GIT_REPOSITORY https://github.com/zlib-ng/zlib-ng
  GIT_TAG 2.2.4
  OVERRIDE_FIND_PACKAGE
  EXCLUDE_FROM_ALL
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(zlib)
add_library(ZLIB::ZLIB ALIAS zlib)
set(ZLIB_INCLUDE_DIR "${zlib_SOURCE_DIR}" "${zlib_BINARY_DIR}")

add_subdirectory(src/yahbog-core)
add_subdirectory(src/yahbog-tests)
add_subdirectory(src/yahbog-gui)