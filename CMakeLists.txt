cmake_minimum_required(VERSION 3.15)
set(STAQ_VERSION_NUM 3.2.3)
set(STAQ_VERSION_STR "${STAQ_VERSION_NUM}")
project(staq VERSION ${STAQ_VERSION_NUM} LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}")
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/")
enable_testing()

#### staq version
add_compile_definitions(STAQ_VERSION_NUM=${STAQ_VERSION_NUM})
add_compile_definitions(STAQ_VERSION_STR="${STAQ_VERSION_STR}")

#### staq root directory
add_compile_definitions(PROJECT_ROOT_DIR="${PROJECT_SOURCE_DIR}")

#### grid synth
option(BUILD_GRID_SYNTH "Builds staq_grid_synth, requires GMP" OFF)
if (${BUILD_GRID_SYNTH})
    message(STATUS "Builds staq_grid_synth - ON")
else ()
    message(STATUS "Builds staq_grid_synth - OFF")
endif ()

#### Force clang to use libc++
if (${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif ()

#### Windows issues with Microsoft Visual Studio
if (MSVC)
    include_directories(SYSTEM libs/pthreadwin32)
    add_compile_options(-bigobj)
    add_compile_definitions(NOMINMAX)
endif ()

#### MinGW or Cygwin have issues with object files that are too large
if (MINGW OR CYGWIN)
    add_compile_options("-Wa,-mbig-obj")
endif ()

#### Libs
include_directories(SYSTEM libs)

#### staq headers
add_library(libstaq INTERFACE)
target_include_directories(libstaq INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>)
#### qasmtools library
target_include_directories(libstaq INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/qasmtools/include/>)

#### Enable OpenQASM 2.0 Specs
option(USE_OPENQASM2_SPECS "Use OpenQASM 2.0 standard instead of Qiskit gate specifications" OFF)
if (${USE_OPENQASM2_SPECS})
    target_compile_definitions(libstaq INTERFACE -DUSE_OPENQASM2_SPECS=true)
    message(STATUS "OpenQASM2 specs - ON")
else ()
    target_compile_definitions(libstaq INTERFACE -DUSE_OPENQASM2_SPECS=false)
    message(STATUS "OpenQASM2 specs - OFF")
endif ()

#### Compiler
set(COMPILER "staq")
add_executable(${COMPILER} ${PROJECT_SOURCE_DIR}/staq/main.cpp)
target_link_libraries(${COMPILER} PUBLIC libstaq)

#### Additional command line tools
add_subdirectory(tools)

#### Unit testing
include(cmake/staq_unit_tests.cmake)

#### Enable all warnings for GNU gcc and Clang/AppleClang
if (${CMAKE_CXX_COMPILER_ID} MATCHES "Clang" OR ${CMAKE_CXX_COMPILER_ID}
        STREQUAL "GNU")
    add_compile_options("-pedantic" "-Wall" "-Wextra" "-Weffc++")
endif ()

#### Default build type
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING
            "Choose the type of build, options are: \
         None Debug Release MinSizeRel RelWithDebInfo."
            FORCE)
endif ()

#### Installation
install(TARGETS ${COMPILER} DESTINATION ${CMAKE_INSTALL_BINDIR})

#### Uninstall
#### https://gitlab.kitware.com/cmake/community/-/wikis/FAQ#can-i-do-make-uninstall-with-cmake
#### UNIX/Linux: sudo cmake --build build --target uninstall
#### Windows:    cmake --build build --target uninstall
if (NOT TARGET uninstall)
    configure_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/cmake/staq_uninstall.cmake.in"
            "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
            IMMEDIATE @ONLY)
    if (NOT MSVC)
        add_custom_target(uninstall
                COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
        )
    else ()
        add_custom_target(uninstall
                COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
                COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_INSTALL_PREFIX}"
        )
    endif ()
endif ()
