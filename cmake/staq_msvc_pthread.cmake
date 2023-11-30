#### Microsoft Visual Studio lack of pthread.h
if (MSVC)
    include_directories(SYSTEM ${STAQ_INSTALL_DIR}/third_party/pthreadwin32)
endif ()
