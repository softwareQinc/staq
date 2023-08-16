file(GLOB FILENAMES *.cpp)

add_custom_target(tools COMMENT "Build all binary tools.")

foreach (filename ${FILENAMES})
    get_filename_component(basename ${filename} NAME_WE)
    # disable grid_synth for MSVC
    if (MSVC AND ${basename} STREQUAL "grid_synth")
        continue()
    else ()
        add_executable("staq_${basename}" ${filename})
        target_link_libraries(staq_${basename} PUBLIC gmp gmpxx)
    endif ()
    target_link_libraries(staq_${basename} PUBLIC libstaq)
    add_dependencies(tools staq_${basename})
    install(TARGETS staq_${basename} DESTINATION ${CMAKE_INSTALL_BINDIR})
endforeach ()
