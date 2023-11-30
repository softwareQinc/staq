#### MSVC lack of pthread.h
if (MSVC)
    include_directories(SYSTEM ${STAQ_INSTALL_DIR}/third_party/pthreadwin32)
endif ()

#### MSVC bigobj
if (MSVC)
    add_compile_options(-bigobj)
endif ()
