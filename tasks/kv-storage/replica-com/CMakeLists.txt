cmake_minimum_required(VERSION 3.20)

SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)
SET(CMAKE_CXX_STANDARD 17)

include(FetchContent)
FetchContent_Declare(
    argparse
    GIT_REPOSITORY https://github.com/p-ranav/argparse.git
)
FetchContent_MakeAvailable(argparse)

find_package(Protobuf CONFIG REQUIRED)

option(USE_SYSTEM_GRPC "Use system installed gRPC" ON) #SET OFF IF YOU HAVEN'T GOT gRPC

if(USE_SYSTEM_GRPC)
  find_package(gRPC CONFIG REQUIRED)
else()
    include(FetchContent)
    FetchContent_Declare(
    gRPC
    GIT_REPOSITORY https://github.com/grpc/grpc
    GIT_TAG        v1.28.0  
    )
    set(FETCHCONTENT_QUIET OFF)
    FetchContent_MakeAvailable(gRPC)
endif()

find_package(gRPC CONFIG REQUIRED)
add_library(protolib proto/kv.proto)
target_link_libraries(protolib gRPC::grpc++)
target_include_directories(protolib PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
get_target_property(grpc_cpp_plugin_location gRPC::grpc_cpp_plugin LOCATION)
PROTOBUF_GENERATE(TARGET protolib LANGUAGE cpp)
PROTOBUF_GENERATE(TARGET protolib LANGUAGE grpc
GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc PLUGIN "protoc-gen-grpc=${grpc_cpp_plugin_location}")

add_executable(
    replica_com
    main.cpp
)

target_link_libraries(
    replica_com 
    protolib
    argparse
)