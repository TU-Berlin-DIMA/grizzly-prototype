cmake_minimum_required(VERSION 3.5.1)
project(Grizzly CXX)

# Custome CMake find instructions and macros
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH}")
include(cmake/macros.cmake)

# Check if build type is set
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING
            "Choose build type: Debug or Release."
            FORCE
            )
endif (NOT CMAKE_BUILD_TYPE)

# C++ Standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set Optimization Flags
set(CMAKE_CXX_FLAGS "-Wall -lnuma -lpapi")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -lnuma -lpapi")
set(CMAKE_CXX_FLAGS_RELEASE "-Wextra -O3 -lnuma -lpapi")

# Compiler should produce specific code for system architecture
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-march=native" COMPILER_SUPPORTS_MARCH_NATIVE)
if (COMPILER_SUPPORTS_MARCH_NATIVE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -mtune=native -mavx")
endif ()

# Threads
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
set(LIBRARIES ${LIBRARIES} Threads::Threads)

# Use Performance Counters?
find_package(PAPI)
set(LIBRARIES ${LIBRARIES} ${PAPI_LIBRARIES})
#endif(USE_PERFORMANCE_COUNTER)

set(LIBRARIES ${LIBRARIES} "-lnuma -ltbb")

# Boost Libraries
find_package(Boost 1.47.0 REQUIRED system thread program_options filesystem serialization) # Only check if lib is available on system for generated code.
link_directories(${Boost_LIBRARY_DIRS})
include_directories(${Boost_INCLUDE_DIRS})
set(LIBRARIES ${LIBRARIES} ${Boost_LIBRARIES})

# Library containing dlopen and dlcose.
set(LIBRARIES ${LIBRARIES} ${CMAKE_DL_LIBS})


# Create a libgmock target to be used as a dependency by test programs
add_library(lnuma IMPORTED STATIC GLOBAL)


# Build and Link #######################################################################################################

# Add Source Code
add_subdirectory(src)

# Add Library
get_source_grizzly(GRIZZLY_SOURCE_FILES)
get_header_grizzly(GRIZZLY_HEADER_FILES)
add_library(grizzly-lib ${GRIZZLY_SOURCE_FILES} ${GRIZZLY_HEADER_FILES})
target_include_directories(grizzly-lib PUBLIC "include")

add_executable(grizzly start.cpp ${Grizzly_HEADER_FILES})
target_link_libraries(grizzly grizzly-lib ${LIBRARIES})

# Make directories for generated code + copy runtime
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/generated-code)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/jit-generated-code)
file(COPY include/jit/runtime DESTINATION jit-generated-code)
file(COPY include/runtime DESTINATION generated-code)
file(COPY include/runtime/input_types.h DESTINATION generated-code/runtime)
