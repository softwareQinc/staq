file(GLOB FILENAMES *.cpp)

add_custom_target(tools EXCLUDE_FROM_ALL COMMENT "Build all binary tools.")

foreach(filename ${FILENAMES})
  get_filename_component(basename ${filename} NAME_WE)
  add_executable(${basename} EXCLUDE_FROM_ALL ${filename})
  target_link_libraries(${basename} PUBLIC libsynthewareQ)
  add_dependencies(tools ${basename})
endforeach()
