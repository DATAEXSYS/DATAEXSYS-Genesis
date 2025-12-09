# Set the minimum required version of CMake
cmake_minimum_required(VERSION 3.10)

# Set the project name
project(DATAEXSYS-Genesis CXX)

# Set the C++ standard to C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set the output directory for the final executable
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build)

# Add the 'inc' directory to the include path for the compiler
include_directories(inc)

# Automatically find all .cpp source files within the 'src' directory and its subdirectories
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/**/*.cpp")

# Create the executable from the collected source files
add_executable(genesis_app sim.cpp ${SOURCES})

# Print a message indicating where the executable will be created
message(STATUS "Executable 'genesis_app' will be built in the 'build' directory.")
