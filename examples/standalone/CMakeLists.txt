cmake_minimum_required(VERSION 3.15)
project(standalone)
set(CMAKE_CXX_STANDARD 17)

# If staq's installation path was non-standard, i.e., specified by
#
# cmake -B build -DCMAKE_INSTALL_PREFIX=/path/to/installed/staq
#
# then uncomment the following line and replace the installation path with yours

# set(CMAKE_PREFIX_PATH "/path/to/installed/staq")

find_package(staq REQUIRED)

add_executable(standalone src/main.cpp)
target_link_libraries(standalone PUBLIC libstaq)
