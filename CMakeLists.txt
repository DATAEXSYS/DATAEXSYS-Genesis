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
# Automatically find all .cpp source files within the 'src' directory and its subdirectories
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/**/*.cpp")

# --- OpenSSL Integration ---
find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})

# Original Simulation App
add_executable(genesis_app sim.cpp ${SOURCES})
target_link_libraries(genesis_app OpenSSL::SSL OpenSSL::Crypto) 

# New PKCertChain Test App
add_executable(pk_test main.cpp ${SOURCES})
target_link_libraries(pk_test OpenSSL::SSL OpenSSL::Crypto)

# Print a message indicating where the executable will be created
message(STATUS "Executables 'genesis_app' and 'pk_test' will be built in the 'build' directory.")
