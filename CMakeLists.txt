cmake_minimum_required(VERSION 3.20)
project(Cerridwen VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enforce GCC 16 for C++26 reflection
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 16.0)
        message(FATAL_ERROR "GCC 16 or higher is required for C++26 reflection.")
    endif()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -freflection")
else()
    message(WARNING "Compiler might not support C++26 reflection. GCC 16 is recommended.")
endif()

# Fetch Wasmtime C API
include(FetchContent)
# For now, we will use a mocked runtime interface for the TDD tests until we need the real Wasmtime C-API binaries.
# In a real scenario, this would download the wasmtime-c-api release.

# Include directories
add_library(cerridwen_core INTERFACE)
target_include_directories(cerridwen_core INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Enable testing
enable_testing()
add_subdirectory(tests)
