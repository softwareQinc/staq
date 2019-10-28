set(target "unit_tests")

include_directories(${CMAKE_SOURCE_DIR}/libs/googletest/include ${CMAKE_SOURCE_DIR}/libs/googletest)

aux_source_directory(. FILES)
aux_source_directory(parser FILES)
aux_source_directory(utils FILES)
aux_source_directory(gates FILES)
aux_source_directory(optimization FILES)
aux_source_directory(transformations FILES)
aux_source_directory(mapping FILES)
aux_source_directory(synthesis FILES)
add_executable(${target} main.cpp)
if (NOT ${CMAKE_VERSION} VERSION_LESS "3.13")
    CMAKE_POLICY(SET CMP0076 NEW)
endif ()

foreach(filename ${FILES})
  target_sources(${target} PUBLIC ${filename})
endforeach()

target_link_libraries(${target} PUBLIC gmock)
target_link_libraries(${target} PUBLIC libsynthewareQ)
