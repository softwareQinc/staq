cmake_minimum_required(VERSION 3.20)
project(ProjectName)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Include FetchContent module
include(FetchContent)

# Configure staq dependency
FetchContent_Declare(
  staq
  GIT_REPOSITORY https://github.com/BorissovAnton/staq.git
  GIT_TAG fetchcontent # Use the fetchcontent branch
)

# Make staq available
FetchContent_MakeAvailable(staq)

# Executable that uses staq
add_executable(ExecutableName main.cpp)

# Link against staq library
target_link_libraries(ExecutableName PRIVATE libstaq)

# If you need to include staq headers directly
target_include_directories(ExecutableName PRIVATE ${staq_SOURCE_DIR}/include)
