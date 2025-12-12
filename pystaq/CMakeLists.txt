# pystaq - Python3 bindings for staq

cmake_minimum_required(VERSION 3.20)
project(pystaq LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Define libstaq interface if not already defined
if(NOT TARGET libstaq)
  message(STATUS "Defining local libstaq interface target")
  add_library(libstaq INTERFACE)

  target_include_directories(
    libstaq
    INTERFACE $<BUILD_INTERFACE: ${CMAKE_SOURCE_DIR}/../include
              ${CMAKE_SOURCE_DIR}/../qasmtools/include
              ${CMAKE_SOURCE_DIR}/../libs/third_party>)
endif()

# Detect Python and pybind11
find_package(Python3 REQUIRED COMPONENTS Interpreter Development)
include("../cmake/pybind11.cmake") # adds required dependencies to libstaq

# Add Python extension
pybind11_add_module(pystaq src/pystaq.cpp)

# Include directories
target_include_directories(pystaq PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Link libraries
target_link_libraries(pystaq PRIVATE pybind11::module libstaq)

# Install the module
install(TARGETS pystaq LIBRARY DESTINATION pystaq)

# Generate stubs by running pybind11-stubgen after the module is compiled
set(STUBGEN_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}/stubs")
file(MAKE_DIRECTORY "${STUBGEN_OUTPUT_PATH}")

if(WIN32)
  set(PYTHONPATH_SEP ";")
else()
  set(PYTHONPATH_SEP ":")
endif()

add_custom_command(
  TARGET pystaq
  POST_BUILD
  COMMAND
    ${CMAKE_COMMAND} -E env
    "PYTHONPATH=$<TARGET_FILE_DIR:pystaq>${PYTHONPATH_SEP}$ENV{PYTHONPATH}"
    ${Python3_EXECUTABLE} -m pybind11_stubgen pystaq --output-dir
    "${STUBGEN_OUTPUT_PATH}" --ignore-all-errors
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "Generating Python stubs for pystaq..."
  VERBATIM)

# Install the generated stub directory alongside the module
install(DIRECTORY "${STUBGEN_OUTPUT_PATH}/pystaq/" DESTINATION pystaq)
