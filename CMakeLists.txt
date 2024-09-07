cmake_minimum_required(VERSION 3.5.0)

project(Platform CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_EXE_LINKER_FLAGS 
          "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath -Wl,/usr/local/lib")

set(LD_LIBRARY_PATH /usr/local/lib)

find_package(GTest REQUIRED)

include(FetchContent)
FetchContent_Declare(
  simdjson
  GIT_REPOSITORY https://github.com/simdjson/simdjson.git
  GIT_TAG  tags/v3.10.1
  GIT_SHALLOW TRUE)
 
FetchContent_MakeAvailable(simdjson)

add_executable(ArbSimulation src/main.cpp)
add_executable(Tests tests/tests.cpp)

target_link_libraries(ArbSimulation PUBLIC simdjson)
target_link_libraries(Tests PUBLIC gtest)

set_property(TARGET ArbSimulation PROPERTY CXX_STANDARD 20)
set_property(TARGET Tests PROPERTY CXX_STANDARD 20)

