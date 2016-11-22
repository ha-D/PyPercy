cmake_minimum_required(VERSION 3.3)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -fPIC -g" )

project(pypercy)


include_directories(/home/hadi/Projects/PIR/percy)
include_directories(/home/hadi/Projects/PIR/percy/ntl/include)
include_directories(/home/hadi/Projects/PIR/percy/ntl/include/NTL)


add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../percy ${CMAKE_CURRENT_BINARY_DIR}/../percy)
add_library(pypercy SHARED client.cc server.cc)

target_compile_options(pypercy PRIVATE -fPIC)
target_link_libraries(pypercy percy)
