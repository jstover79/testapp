cmake_minimum_required(VERSION 3.20)
project(HarrisLab VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(HARRIS_BUILD_GUI "Build the Harris Lab desktop application" ON)
option(HARRIS_BUILD_TESTS "Build the simulation unit tests" ON)

function(harris_enable_warnings target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4 /permissive-)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endfunction()

add_library(harris_process src/contact_process.cpp)
target_include_directories(harris_process PUBLIC include)
target_compile_features(harris_process PUBLIC cxx_std_20)
harris_enable_warnings(harris_process)

if(HARRIS_BUILD_GUI)
  include(FetchContent)
  set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
  # Harris Lab only uses raylib's windowing and graphics modules. Disabling
  # audio also keeps its bundled module-file loaders out of the build.
  set(CUSTOMIZE_BUILD ON CACHE BOOL "Customize the raylib build" FORCE)
  set(SUPPORT_MODULE_RAUDIO OFF CACHE BOOL "Build raylib audio support" FORCE)
  FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP OFF
  )
  FetchContent_MakeAvailable(raylib)

  # raudio.c is optional in raylib's customized build. Fail at configure time
  # rather than silently regressing to the audio-enabled source set.
  get_target_property(raylib_sources raylib SOURCES)
  foreach(raylib_source IN LISTS raylib_sources)
    if(raylib_source MATCHES "(^|[/\\\\])raudio\\.c$")
      message(FATAL_ERROR "raylib audio was requested even though Harris Lab disables it")
    endif()
  endforeach()

  # glibc marks fgets() warn_unused_result, and raylib 5.5 intentionally
  # discards one such result in rcore.c. Keep this exception on vendor C only.
  target_compile_options(raylib PRIVATE
    $<$<COMPILE_LANG_AND_ID:C,GNU>:-Wno-unused-result>
  )

  add_executable(harris_lab WIN32 src/main.cpp)
  target_link_libraries(harris_lab PRIVATE harris_process raylib)
  target_compile_definitions(harris_lab PRIVATE HARRIS_LAB_VERSION="${PROJECT_VERSION}")
  harris_enable_warnings(harris_lab)
endif()

if(HARRIS_BUILD_TESTS)
  enable_testing()
  add_executable(harris_tests tests/contact_process_tests.cpp)
  target_link_libraries(harris_tests PRIVATE harris_process)
  harris_enable_warnings(harris_tests)
  add_test(NAME harris_process_tests COMMAND harris_tests)
endif()
