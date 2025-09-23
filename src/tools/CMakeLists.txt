file(GLOB FILENAMES *.cpp)

add_custom_target(tools COMMENT "Build all binary tools.")

include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/grid_synth.cmake)

foreach(file ${FILENAMES})
  get_filename_component(basename ${file} NAME_WE)
  if(${BUILD_GRID_SYNTH})
    add_executable("staq_${basename}" ${file})
    if(MSVC)
      target_link_libraries(staq_${basename} PUBLIC PkgConfig::gmp
                                                    PkgConfig::gmpxx)
      # Add GMP include directories for MSVC
      if(gmp_INCLUDE_DIRS)
        target_include_directories(staq_${basename} PUBLIC ${gmp_INCLUDE_DIRS})
      endif()
    else()
      target_link_libraries(staq_${basename} PUBLIC gmp gmpxx)
      # Add GMP include directories for non-MSVC
      if(GMP_INCLUDE_DIR)
        target_include_directories(staq_${basename} PUBLIC ${GMP_INCLUDE_DIR})
      endif()
    endif()
  else()
    if(${basename} STREQUAL "grid_synth" OR ${basename} STREQUAL "qasm_synth")
      continue()
    endif()
    add_executable("staq_${basename}" ${file})
  endif()
  target_link_libraries(staq_${basename} PUBLIC libstaq)
  add_dependencies(tools staq_${basename})
  install(TARGETS staq_${basename} DESTINATION ${CMAKE_INSTALL_BINDIR})
endforeach()
