include(GoogleTest)
set(TARGET_NAME "unit_tests")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# GoogleTest
include(FetchContent)
message(STATUS "Fetching GoogleTest...")
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG main
  GIT_SHALLOW TRUE
  GIT_PROGRESS TRUE)
# For Windows: Prevent overriding the parent project's compiler/linker settings,
# and add bigobj option
if(MSVC)
  add_compile_options(-bigobj)
  set(gtest_force_shared_crt
      ON
      CACHE BOOL "" FORCE)
  if(MSVC_VERSION GREATER_EQUAL 1914)
    add_compile_options("/Zc:__cplusplus")
  endif()
endif()
FetchContent_MakeAvailable(googletest)

aux_source_directory(tests TEST_FILES)
aux_source_directory(tests/parser TEST_FILES)
aux_source_directory(tests/utils TEST_FILES)
aux_source_directory(tests/gates TEST_FILES)
aux_source_directory(tests/optimization TEST_FILES)
aux_source_directory(tests/transformations TEST_FILES)
aux_source_directory(tests/mapping TEST_FILES)
aux_source_directory(tests/synthesis TEST_FILES)

include(${CMAKE_SOURCE_DIR}/cmake/grid_synth.cmake)
if(${BUILD_GRID_SYNTH})
  aux_source_directory(tests/grid_synth TEST_FILES)
endif()

add_executable(${TARGET_NAME} EXCLUDE_FROM_ALL tests/main.cpp)
add_dependencies(unit_tests ${TARGET_NAME})

if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.13")
  cmake_policy(SET CMP0076 NEW)
endif()

# Build all tests in ${TEST_FILES}
foreach(file ${TEST_FILES})
  get_filename_component(basename ${file} NAME_WE)
  target_sources(${TARGET_NAME} PUBLIC ${file})
endforeach()

target_link_libraries(${TARGET_NAME} PUBLIC gmock libstaq)

if(${BUILD_GRID_SYNTH})
  if(MSVC)
    target_link_libraries(${TARGET_NAME} PUBLIC PkgConfig::gmp PkgConfig::gmpxx)
  else()
    target_link_libraries(${TARGET_NAME} PUBLIC gmp gmpxx)
  endif()
endif()

gtest_discover_tests(${TARGET_NAME})
