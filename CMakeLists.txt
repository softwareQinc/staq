cmake_minimum_required (VERSION 3.8)
project(staq VERSION 1.3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED on)

if (NOT ${CMAKE_VERSION} VERSION_LESS "3.13")
    CMAKE_POLICY(SET CMP0076 NEW)
endif ()

#### Force clang to use libc++
if (${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif ()

#### Microsoft Visual Studio
if (MSVC)
    add_compile_options(-bigobj)
    include_directories(SYSTEM libs/pthreadwin32)
endif ()

#### MinGW or Cygwin have issues with object files that are too large
if (MINGW OR CYGWIN)
    add_compile_options("-Wa,-mbig-obj")
endif()

#### Source directory
add_definitions(-DPATH="${CMAKE_SOURCE_DIR}" -DNOMINMAX)

#### Libs
include_directories(SYSTEM libs)

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
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
add_subdirectory(libs/googletest)
add_subdirectory(unit_tests)
