cmake_minimum_required (VERSION 3.8)
project(synthewareQ LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED on)

#### Source directory
ADD_DEFINITIONS(-DPATH="${CMAKE_SOURCE_DIR}")

### Clang and glibc don't seem to mesh
if (${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -v -stdlib=libc++")
endif()

#### Libs
include_directories(libs)

#### Library
add_library(synthewareQ INTERFACE)
target_include_directories(synthewareQ INTERFACE ${PROJECT_SOURCE_DIR}/include)

#### Examples
add_subdirectory(examples)

#### Binary tools
add_subdirectory(tools)

#### Testing
add_subdirectory(libs/googletest)
add_subdirectory(unit_tests)
