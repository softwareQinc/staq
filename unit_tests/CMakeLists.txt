set(target "unit_tests")

aux_source_directory(. FILES)
aux_source_directory(parsing FILES)
add_executable(${target} main.cpp)
if (NOT ${CMAKE_VERSION} VERSION_LESS "3.13")
    CMAKE_POLICY(SET CMP0076 NEW)
endif ()

foreach(filename ${FILES})
  target_sources(${target} PUBLIC ${filename})
endforeach()

target_link_libraries(${target} PUBLIC gmock)
target_link_libraries(${target} PUBLIC synthewareQ)
