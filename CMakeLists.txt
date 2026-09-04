cmake_minimum_required(VERSION 3.20)
project(HarrisLab VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(HARRIS_BUILD_GUI "Build the Harris Lab desktop application" ON)
option(HARRIS_BUILD_TESTS "Build the simulation unit tests" ON)

add_library(harris_process src/contact_process.cpp)
target_include_directories(harris_process PUBLIC include)
target_compile_features(harris_process PUBLIC cxx_std_20)

if(MSVC)
  target_compile_options(harris_process PRIVATE /W4 /permissive-)
else()
  target_compile_options(harris_process PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(HARRIS_BUILD_GUI)
  include(FetchContent)
  set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP OFF
  )
  FetchContent_MakeAvailable(raylib)

  add_executable(harris_lab WIN32 src/main.cpp)
  target_link_libraries(harris_lab PRIVATE harris_process raylib)
  target_compile_definitions(harris_lab PRIVATE HARRIS_LAB_VERSION="${PROJECT_VERSION}")
endif()

if(HARRIS_BUILD_TESTS)
  enable_testing()
  add_executable(harris_tests tests/contact_process_tests.cpp)
  target_link_libraries(harris_tests PRIVATE harris_process)
  add_test(NAME harris_process_tests COMMAND harris_tests)
endif()
