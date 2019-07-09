cmake_minimum_required (VERSION 3.8)
project(synthewareQ LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED on)

# Libs
include_directories(libs)

# Library
add_library(synthewareQ INTERFACE)
target_include_directories(synthewareQ INTERFACE ${PROJECT_SOURCE_DIR}/include)

# Examples
add_subdirectory(examples)

# Tests
add_subdirectory(test)

# Binary tools
add_subdirectory(tools)

