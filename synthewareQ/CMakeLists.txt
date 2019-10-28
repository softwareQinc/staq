set(target "synthewareq")

if (NOT ${CMAKE_VERSION} VERSION_LESS "3.13")
    CMAKE_POLICY(SET CMP0076 NEW)
endif ()

add_executable(${target} main.cpp)
target_link_libraries(${target} PUBLIC synthewareQ)
