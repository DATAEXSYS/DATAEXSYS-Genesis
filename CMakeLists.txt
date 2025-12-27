cmake_minimum_required(VERSION 3.20)
project(DATAEXSYS-Genesis VERSION 1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler warnings (optional, mirrors your clang++ task)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    add_compile_options(
        -Wall -Wextra -Wpedantic -Wshadow -Wformat=2
        -Wcast-align -Wconversion -Wsign-conversion -Wnull-dereference
    )
endif()

# Include directories
include_directories(
    ${PROJECT_SOURCE_DIR}/inc
    ${PROJECT_SOURCE_DIR}           # root folder, if headers are here
)

# Recursively find all .cpp files in src/ and subdirectories
file(GLOB_RECURSE SRC_FILES "${PROJECT_SOURCE_DIR}/src/*.cpp")

# Also include all .cpp files in the root folder
file(GLOB ROOT_CPP_FILES "${PROJECT_SOURCE_DIR}/*.cpp")

# Combine all source files
list(APPEND SRC_FILES ${ROOT_CPP_FILES})

# Create the executable
add_executable(${PROJECT_NAME} ${SRC_FILES})
