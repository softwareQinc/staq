file(GLOB FILENAMES *.cpp)

add_custom_target(tools COMMENT "Build all binary tools.")

foreach(filename ${FILENAMES})
  get_filename_component(basename ${filename} NAME_WE)
  add_executable("staq_${basename}" ${filename})
  target_link_libraries(staq_${basename} PUBLIC libstaq Eigen3::Eigen gmp gmpxx)
  add_dependencies(tools staq_${basename})
  install(TARGETS staq_${basename} DESTINATION ${CMAKE_INSTALL_BINDIR})
endforeach()
