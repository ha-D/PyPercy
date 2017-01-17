cmake_minimum_required(VERSION 3.3)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -fPIC -g" )

project(pypercy)


include_directories(../../percy)
include_directories(../../ntl/include)
include_directories(../../ntl/include/NTL)


add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../percy ${CMAKE_CURRENT_BINARY_DIR}/../percy)
add_library(pypercy SHARED client.cc server.cc err.cc ag.cc itzzp_client.cc itzzp_server.cc chor_client.cc chor_server.cc)

target_compile_options(pypercy PRIVATE -fPIC)
target_link_libraries(pypercy percy)
