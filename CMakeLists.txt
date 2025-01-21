cmake_minimum_required(VERSION 3.15)
set(STAQ_VERSION_NUM 3.5)
set(STAQ_VERSION_STR "${STAQ_VERSION_NUM}")
project(
  staq
  VERSION ${STAQ_VERSION_NUM}
  LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}")
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/")
enable_testing()

# staq version
add_compile_definitions(STAQ_VERSION_NUM=${STAQ_VERSION_NUM})
add_compile_definitions(STAQ_VERSION_STR="${STAQ_VERSION_STR}")

# staq root directory
add_compile_definitions(PROJECT_ROOT_DIR="${PROJECT_SOURCE_DIR}")

# Guard against in-source builds (snippet from Eigen's CMakeLists.txt)
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(
    FATAL_ERROR
      "In-source builds not allowed. Please instruct CMake to use an\
      out-of-source build, e.g.,
      cmake -B build && cmake --build build
You may need to remove CMakeCache.txt.")
endif()

# # Force Clang to use libc++

# if(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
# add_compile_options("-stdlib=libc++") endif()

# Windows issues with Microsoft Visual Studio
if(MSVC)
  include_directories(SYSTEM libs/third_party/pthreadwin32)
  add_compile_options(-bigobj)
  if(MSVC_VERSION GREATER_EQUAL 1914)
    add_compile_options("/Zc:__cplusplus")
  endif()
endif()

# MinGW or Cygwin have issues with object files that are too large
if(MINGW OR CYGWIN)
  add_compile_options("-Wa,-mbig-obj")
endif()

# staq headers
add_library(libstaq INTERFACE)
target_compile_definitions(libstaq
                           INTERFACE -DSTAQ_VERSION_NUM=${STAQ_VERSION_NUM})
target_compile_definitions(libstaq
                           INTERFACE -DSTAQ_VERSION_STR="${STAQ_VERSION_STR}")
target_include_directories(
  libstaq INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
                    $<INSTALL_INTERFACE:include/staq>)

# qasmtools library
target_include_directories(
  libstaq
  INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/qasmtools/include/>
            $<INSTALL_INTERFACE:include/>)

# 3rd party libs
include_directories(SYSTEM libs/third_party)
target_include_directories(
  libstaq INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/libs>
                    $<INSTALL_INTERFACE:include/staq/third_party>)

# pystaq and pybind11 (only if the Python development kit is detected)
find_package(Python3 QUIET COMPONENTS Interpreter Development)
if(${Python3_FOUND})
  include(cmake/pybind11.cmake)
  include(cmake/pystaq.cmake)
endif()

# Enable OpenQASM 2.0 Specs
option(USE_OPENQASM2_SPECS
       "Use OpenQASM 2.0 standard instead of Qiskit gate specifications" OFF)
if(${USE_OPENQASM2_SPECS})
  target_compile_definitions(libstaq INTERFACE -DUSE_OPENQASM2_SPECS=true)
  message(STATUS "OpenQASM2 specs - ON")
else()
  target_compile_definitions(libstaq INTERFACE -DUSE_OPENQASM2_SPECS=false)
  message(STATUS "OpenQASM2 specs - OFF")
endif()

# Compiler
set(COMPILER "staq")
add_executable(${COMPILER} ${PROJECT_SOURCE_DIR}/src/staq/main.cpp)
target_link_libraries(${COMPILER} PUBLIC libstaq)

# Additional command line tools
add_subdirectory(src/tools)

# Unit testing
add_subdirectory(${CMAKE_SOURCE_DIR}/unit_tests/ EXCLUDE_FROM_ALL SYSTEM)

# Enable all warnings for GCC and Clang/AppleClang
if(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang" OR ${CMAKE_CXX_COMPILER_ID}
                                               STREQUAL "GNU")
  add_compile_options("-pedantic" "-Wall" "-Wextra" "-Weffc++")
endif()

# Default build type
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE
      Release
      CACHE STRING "Choose the type of build, options are: \
         None Debug Release MinSizeRel RelWithDebInfo." FORCE)
endif()

# Installation (binaries)
install(TARGETS ${COMPILER} DESTINATION ${CMAKE_INSTALL_BINDIR})

option(INSTALL_SOURCES "Enable staq's source code installation" OFF)

# Installation (source)
if(INSTALL_SOURCES)
  set(STAQ_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME}")
  install(DIRECTORY include/ DESTINATION ${STAQ_INSTALL_DIR})
  install(DIRECTORY qasmtools/include/ DESTINATION ${STAQ_INSTALL_DIR})
  install(DIRECTORY libs/third_party DESTINATION ${STAQ_INSTALL_DIR})
  install(TARGETS libstaq EXPORT staq_targets)
  install(EXPORT staq_targets DESTINATION "lib/cmake/${PROJECT_NAME}")
  include(CMakePackageConfigHelpers)
  configure_package_config_file(
    "cmake/staqConfig.cmake.in" "${CMAKE_CURRENT_BINARY_DIR}/staqConfig.cmake"
    INSTALL_DESTINATION "lib/cmake/${PROJECT_NAME}")
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/staqConfig.cmake"
          DESTINATION "lib/cmake/${PROJECT_NAME}")
  install(FILES "${CMAKE_SOURCE_DIR}/cmake/staq_msvc.cmake"
          DESTINATION "lib/cmake/${PROJECT_NAME}")
endif()

# Uninstall
# https://gitlab.kitware.com/cmake/community/-/wikis/FAQ#can-i-do-make-uninstall-with-cmake
#
# UNIX/Linux: sudo cmake --build build --target uninstall
#
# Windows: cmake --build build --target uninstall
if(NOT TARGET uninstall)
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/staq_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake" IMMEDIATE @ONLY)
  if(NOT MSVC)
    if(INSTALL_SOURCES)
      add_custom_target(
        uninstall
        COMMAND ${CMAKE_COMMAND} -P
                "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
        COMMAND ${CMAKE_COMMAND} -E remove_directory
                "${CMAKE_INSTALL_PREFIX}/lib/cmake/${PROJECT_NAME}"
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${STAQ_INSTALL_DIR}"
        COMMENT "Uninstall staq and its sources")
    else()
      add_custom_target(
        uninstall
        COMMAND ${CMAKE_COMMAND} -P
                "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
        COMMENT "Uninstall staq")
    endif()
  else()
    add_custom_target(
      uninstall
      COMMAND ${CMAKE_COMMAND} -P
              "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
      COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_INSTALL_PREFIX}"
      COMMENT "Uninstall staq")
  endif()
endif()
