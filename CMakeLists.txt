cmake_minimum_required(VERSION 3.20)
project(DATAEXSYS-Genesis VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    add_compile_options(
        -Wall -Wextra -Wpedantic -Wshadow -Wformat=2
        -Wcast-align -Wconversion -Wsign-conversion -Wnull-dereference
    )
endif()

# Collect all .cpp files under src/
file(GLOB_RECURSE SRC_FILES "${PROJECT_SOURCE_DIR}/src/*.cpp")

# Create executable
add_executable(${PROJECT_NAME} ${SRC_FILES})

# Include headers in inc/
target_include_directories(${PROJECT_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/inc")
