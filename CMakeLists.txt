# Set the minimum required version of CMake
cmake_minimum_required(VERSION 3.10)

# Set the project name
project(DATAEXSYS-Genesis CXX)

# Set the C++ standard to C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set the output directory for the final executables
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build)

# Add the 'inc' directory to the include path for the compiler
include_directories(inc)

# Find required packages
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

# Make Boost optional
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
find_package(Boost COMPONENTS system filesystem)

# Automatically find all .cpp source files within the 'src' directory and its subdirectories
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/**/*.cpp")

# Add common include directories
include_directories(${OPENSSL_INCLUDE_DIR})

# Common link libraries
set(COMMON_LIBS
    ${OPENSSL_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
)

# Add Boost if found
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    list(APPEND COMMON_LIBS ${Boost_LIBRARIES})
    message(STATUS "Boost libraries found and will be linked")
else()
    message(WARNING "Boost libraries not found. Some features may be limited.")
    # Add any necessary workarounds for missing Boost here
endif()

# Create the main genesis_app executable
add_executable(genesis_app sim.cpp ${SOURCES})
target_link_libraries(genesis_app ${COMMON_LIBS})

# Create the test executable
add_executable(sim_test simTest.cpp ${SOURCES})
target_link_libraries(sim_test ${COMMON_LIBS})

# Create the PKCertChain simulation executable
add_executable(sim_pkcert_chain simPKCertChain.cpp ${SOURCES})
target_link_libraries(sim_pkcert_chain ${COMMON_LIBS})

# Print a message indicating where the executables will be built
message(STATUS "Executables will be built in the 'build' directory")
message(STATUS "Available executables:")
message(STATUS "  - genesis_app: Main simulation")
message(STATUS "  - sim_test: Test simulation")
message(STATUS "  - sim_pkcert_chain: PKCertChain simulation")

# Print build configuration summary
message(STATUS "Build configuration:")
message(STATUS "  - C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  - Boost: ${Boost_FOUND}")
message(STATUS "  - OpenSSL: ${OPENSSL_FOUND}")
message(STATUS "  - Threads: ${CMAKE_USE_PTHREADS_INIT}")