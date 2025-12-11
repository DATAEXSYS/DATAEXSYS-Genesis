cmake_minimum_required(VERSION 3.10)

project(DATAEXSYS-Genesis CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build)

include_directories(inc)

file(GLOB_RECURSE SOURCES
    "src/*.cpp"
    "src/*/*.cpp"
    "src/*/*/*.cpp"
    "src/*/*/*/*.cpp"
)

find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})

add_executable(genesis_app sim.cpp ${SOURCES})
target_link_libraries(genesis_app OpenSSL::SSL OpenSSL::Crypto)

add_executable(pk_test main.cpp ${SOURCES})
target_link_libraries(pk_test OpenSSL::SSL OpenSSL::Crypto)

message(STATUS "Executables built in: ${EXECUTABLE_OUTPUT_PATH}")
