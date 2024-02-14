# LSP and CMake support for pystaq

# pybind11
include_directories(SYSTEM ${PYBIND11_INCLUDE_DIRS})
target_include_directories(
  libstaq INTERFACE $<BUILD_INTERFACE:${PYBIND11_INCLUDE_DIRS}/include/>)

# Python development
target_include_directories(libstaq
                           INTERFACE $<BUILD_INTERFACE:${Python3_INCLUDE_DIRS}>)

# pystaq
target_include_directories(
  libstaq
  INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/pystaq/include/>)
