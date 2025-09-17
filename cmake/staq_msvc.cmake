if(MSVC)
  add_definitions(-DNOMINMAX)
  # MSVC lacks <pthread.h>
  include_directories(SYSTEM ${STAQ_INSTALL_DIR}/third_party/pthreadwin32)
  add_compile_options(/bigobj)
  add_compile_options(/utf-8)
endif()
