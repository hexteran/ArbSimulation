cmake_minimum_required(VERSION 3.5.0)

project(Platform CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-g")

set(LD_LIBRARY_PATH /usr/local/lib)

find_package(GTest REQUIRED)

add_executable(ArbSimulation src/main.cpp)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(Tests tests/tests.cpp)
                    
target_link_libraries(Tests PUBLIC gtest)
set(CMAKE_EXE_LINKER_FLAGS 
          "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath -Wl,/usr/local/lib")
set_property(TARGET ArbSimulation PROPERTY CXX_STANDARD 20)
set_property(TARGET Tests PROPERTY CXX_STANDARD 20)

enable_testing()

