file(GLOB FILENAMES *.cpp)

add_custom_target(tests COMMENT "Build all tests.")

foreach(filename ${FILENAMES})
  get_filename_component(basename ${filename} NAME_WE)
  add_executable(${basename} ${filename})
  target_link_libraries(${basename} PUBLIC synthewareQ)
  add_dependencies(tests ${basename})
endforeach()
