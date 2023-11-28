#### Windows issues with Microsoft Visual Studio
if (MSVC)
    include_directories(SYSTEM libs/third_party/pthreadwin32)
    add_compile_options(-bigobj)
    add_compile_definitions(NOMINMAX)
    if (MSVC_VERSION GREATER_EQUAL 1914)
        add_compile_options("/Zc:__cplusplus")
    endif ()
endif ()
