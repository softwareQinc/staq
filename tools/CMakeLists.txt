file(GLOB FILENAMES *.cpp)

add_custom_target(tools COMMENT "Build all binary tools.")

foreach(filename ${FILENAMES})
  get_filename_component(basename ${filename} NAME_WE)
  add_executable(${basename} ${filename})
  target_link_libraries(${basename} PUBLIC libstaq)
  add_dependencies(tools ${basename})
endforeach()
