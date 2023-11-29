#### Windows issues with Microsoft Visual Studio
if (MSVC)
    message(STATUS "MSVC -- including the cmake")
    include_directories(SYSTEM
            ${CMAKE_INSTALL_PREFIX}/include/staq/third_party/pthreadwin32)
    message(WARNING ${CMAKE_INSTALL_PREFIX})
    message(WARNING @STAQ_INSTALL_DIR@)
    add_compile_options(-bigobj)
    add_compile_definitions(NOMINMAX)
    if (MSVC_VERSION GREATER_EQUAL 1914)
        add_compile_options("/Zc:__cplusplus")
    endif ()
endif ()
