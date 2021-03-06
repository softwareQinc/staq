cmake_minimum_required(VERSION 3.12)
project(staq VERSION 1.4 LANGUAGES CXX)
enable_testing()

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
endif ()

#### Source directory
add_compile_definitions(PATH="${CMAKE_SOURCE_DIR}" NOMINMAX)

#### Libs
include_directories(SYSTEM libs)

#### staq headers
add_library(libstaq INTERFACE)
target_include_directories(libstaq INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
        $<INSTALL_INTERFACE:include/>)

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

#### staq installation instructions

#### For both UNIX/Windows, first run from an out-of-source directory
####
#### 'cmake ..'
####
#### If you want to install staq in a non-standard location (e.g., due to lack
#### of admin. rights), then pass the additional flag
#### '-DCMAKE_INSTALL_PREFIX=/path/to/staq' to cmake above, i.e.,
####
#### 'cmake .. -DCMAKE_INSTALL_PREFIX=/path/to/staq'

#### Next, follow the platform-specific instructions below

#### UNIX/Linux: sudo make install (or sudo cmake --build . --target install)
#### Will install in: /usr/local/bin

#### Windows: cmake --build . --target INSTALL (from an Admin. Command Prompt)
#### Will install in: C:\Program Files (x86)\staq\bin

install(TARGETS ${COMPILER} DESTINATION ${CMAKE_INSTALL_BINDIR})

#### Uninstall
#### https://gitlab.kitware.com/cmake/community/-/wikis/FAQ#can-i-do-make-uninstall-with-cmake
#### UNIX/Linux: sudo make uninstall (or sudo cmake --build . --target uninstall)
#### Windows:    cmake --build . --target UNINSTALL
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
