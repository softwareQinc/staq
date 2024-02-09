# LSP and CMake support for pystaq

# pybind11
include_directories(SYSTEM libs/)
target_include_directories(
  libstaq INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/libs/>)

# Python development
target_include_directories(libstaq
                           INTERFACE $<BUILD_INTERFACE:${Python3_INCLUDE_DIRS}>)

# pystaq
target_include_directories(
  libstaq
  INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/pystaq/include/>)
