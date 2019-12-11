cmake_minimum_required (VERSION 3.8)
project(staq LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED on)

if (NOT ${CMAKE_VERSION} VERSION_LESS "3.13")
    CMAKE_POLICY(SET CMP0076 NEW)
endif ()

### Clang and glibc don't seem to mesh
if (${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif()

#### Source directory
ADD_DEFINITIONS(-DPATH="${CMAKE_SOURCE_DIR}")

#### Libs
include_directories(libs)

#### Library
add_library(libstaq INTERFACE)
target_include_directories(libstaq INTERFACE ${PROJECT_SOURCE_DIR}/include)

#### Compiler
set(compiler "staq")
add_executable(${compiler} ${PROJECT_SOURCE_DIR}/staq/main.cpp)
target_link_libraries(${compiler} PUBLIC libstaq)

#### Additional command line tools
add_subdirectory(tools)

#### Testing
add_subdirectory(libs/googletest)
add_subdirectory(unit_tests)
