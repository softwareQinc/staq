file(GLOB FILENAMES *.cpp)

add_custom_target(examples COMMENT "Build all examples.")

foreach(filename ${FILENAMES})
  get_filename_component(basename ${filename} NAME_WE)
  add_executable(${basename} ${filename})
  target_link_libraries(${basename} PUBLIC synthewareQ)
  add_dependencies(examples ${basename})
endforeach()
