set(target "staq")

add_executable(${target} main.cpp)
target_link_libraries(${target} PUBLIC staq)
