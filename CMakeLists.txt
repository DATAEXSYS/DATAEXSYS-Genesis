cmake_minimum_required(VERSION 3.12)
project(DATAEXSYS_Genesis CXX)

# Set the C++ standard to C++23, as indicated by the project analysis.
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Recursively find all C++ source and header files in the BABE directory.
# CONFIGURE_DEPENDS ensures that CMake will re-run if files are added or removed.
file(GLOB_RECURSE BABE_FILES CONFIGURE_DEPENDS
    "BABE/*.h"
    "BABE/*.cpp"
)

# Recursively find all C++ source and header files in the DATAEXSYS directory.
file(GLOB_RECURSE DATAEXSYS_FILES CONFIGURE_DEPENDS
    "DATAEXSYS/*.h"
    "DATAEXSYS/*.cpp"
)

# Combine the file lists.
set(SOURCE_FILES ${BABE_FILES} ${DATAEXSYS_FILES})

# Add an executable target.
# Note: This assumes a main() function exists in one of the source files.
add_executable(dataexsys_genesis "")

# Add the source files to the target.
target_sources(dataexsys_genesis PRIVATE ${SOURCE_FILES})

# Add include directories to the target. This is the modern way.
target_include_directories(dataexsys_genesis PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/BABE/SDK/inc"
    "${CMAKE_CURRENT_SOURCE_DIR}/BABE/SDK/inc/App"
    "${CMAKE_CURRENT_SOURCE_DIR}/BABE/SDK/inc/App/Classes"
    "${CMAKE_CURRENT_SOURCE_DIR}/BABE"
    "${CMAKE_CURRENT_SOURCE_DIR}/DATAEXSYS"
)

# Optional: Add compiler flags for warnings and debugging.
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANGXX)
    target_compile_options(dataexsys_genesis PRIVATE -Wall -Wextra -Wpedantic -g)
endif()

# Print the files that will be compiled
message(STATUS "Source files to compile: ${SOURCE_FILES}")
