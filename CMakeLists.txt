cmake_minimum_required(VERSION 3.10)

project(DATAEXSYS-Genesis CXX)

# Use C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Output executables into build/ folder
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build)

# Add include folder
include_directories(inc)

# Recursively collect all .cpp files (Linux-compatible)
file(GLOB_RECURSE SOURCES
    "src/*.cpp"
    "src/*/*.cpp"
    "src/*/*/*.cpp"
    "src/*/*/*/*.cpp"
)

# Ensure Trust Diary is included even if folder changes
set(SOURCES ${SOURCES}
    src/Trust/LocalTrustDiary.cpp
)

# --- OpenSSL ---
find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})

# Main simulation app
add_executable(genesis_app sim.cpp ${SOURCES})
target_link_libraries(genesis_app OpenSSL::SSL OpenSSL::Crypto)

# PKCertChain + Crypto test app
add_executable(pk_test main.cpp ${SOURCES})
target_link_libraries(pk_test OpenSSL::SSL OpenSSL::Crypto)

message(STATUS "Executables built in: ${EXECUTABLE_OUTPUT_PATH}")
