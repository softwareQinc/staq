file(GLOB FILENAMES *.cpp)

add_custom_target(tools COMMENT "Build all binary tools.")

include(${CMAKE_SOURCE_DIR}/cmake/grid_synth.cmake)

set(RESOURCE_ESTIMATOR_HUBBARD_SOURCES
    ${CMAKE_SOURCE_DIR}/experimental/src/hubbard/builders.cpp
    ${CMAKE_SOURCE_DIR}/experimental/src/hubbard/layout.cpp
    ${CMAKE_SOURCE_DIR}/experimental/src/hubbard/model_params.cpp
    ${CMAKE_SOURCE_DIR}/experimental/src/hubbard/operators.cpp
    ${CMAKE_SOURCE_DIR}/experimental/src/hubbard/program_io.cpp)

foreach(file ${FILENAMES})
  get_filename_component(basename ${file} NAME_WE)
  if(NOT ${BUILD_GRID_SYNTH} AND ("${basename}" STREQUAL "grid_synth"
                                  OR "${basename}" STREQUAL "qasm_synth"))
    continue()
  endif()

  set(target_sources ${file})
  if("${basename}" STREQUAL "resource_estimator")
    list(APPEND target_sources ${RESOURCE_ESTIMATOR_HUBBARD_SOURCES})
  endif()

  add_executable("staq_${basename}" ${target_sources})

  if(${BUILD_GRID_SYNTH})
    if(MSVC)
      target_link_libraries(staq_${basename} PUBLIC PkgConfig::gmp
                                                    PkgConfig::gmpxx)
    else()
      target_link_libraries(staq_${basename} PUBLIC gmp gmpxx)
    endif()
  endif()

  if("${basename}" STREQUAL "resource_estimator")
    target_include_directories(staq_${basename}
                               PRIVATE ${CMAKE_SOURCE_DIR}/experimental/include)
    set_property(TARGET staq_${basename} PROPERTY CXX_STANDARD 20)
    set_property(TARGET staq_${basename} PROPERTY CXX_STANDARD_REQUIRED ON)
  endif()

  target_link_libraries(staq_${basename} PUBLIC libstaq)
  add_dependencies(tools staq_${basename})
  install(TARGETS staq_${basename} DESTINATION ${CMAKE_INSTALL_BINDIR})
endforeach()
