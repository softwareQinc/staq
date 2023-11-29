include(GoogleTest)
set(TARGET_NAME "unit_tests")

#### Link Google Test dynamically if using MSVC
if (MSVC)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    if (MSVC_VERSION GREATER_EQUAL 1914)
        add_compile_options("/Zc:__cplusplus")
    endif ()
endif ()

set(BUILD_GRID_SYNTH OFF)
if (MSVC)
    find_package(PkgConfig)
    if (PkgConfig_FOUND)
        # gmp, gmpxx
        pkg_check_modules(gmp IMPORTED_TARGET gmp)
        pkg_check_modules(gmpxx IMPORTED_TARGET gmpxx)
        if (GMP_FOUND AND GMPXX_FOUND)
            set(BUILD_GRID_SYNTH ON)
            add_compile_options("-DEXPR_GMP")
        endif ()
    endif ()
else ()
    find_package(GMP REQUIRED)
    if (GMP_FOUND)
        set(BUILD_GRID_SYNTH ON)
        add_compile_options("-DEXPR_GMP")
    endif ()
endif ()

add_subdirectory(libs/googletest-release-1.12.1 EXCLUDE_FROM_ALL SYSTEM)

aux_source_directory(tests TEST_FILES)
aux_source_directory(tests/parser TEST_FILES)
aux_source_directory(tests/utils TEST_FILES)
aux_source_directory(tests/gates TEST_FILES)
aux_source_directory(tests/optimization TEST_FILES)
aux_source_directory(tests/transformations TEST_FILES)
aux_source_directory(tests/mapping TEST_FILES)
aux_source_directory(tests/synthesis TEST_FILES)

message(WARNING "Build grid synth: ${BUILD_GRID_SYNTH}")

if (${BUILD_GRID_SYNTH})
    aux_source_directory(tests/grid_synth TEST_FILES)
endif ()

add_executable(${TARGET_NAME} EXCLUDE_FROM_ALL tests/main.cpp)
add_dependencies(unit_tests ${TARGET_NAME})

if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.13")
    CMAKE_POLICY(SET CMP0076 NEW)
endif ()

#### Build all tests in ${TEST_FILES}
foreach (filename ${TEST_FILES})
    get_filename_component(basename ${filename} NAME_WE)
    target_sources(${TARGET_NAME} PUBLIC ${filename})
endforeach ()

target_link_libraries(${TARGET_NAME} PUBLIC gmock libstaq)

if (${BUILD_GRID_SYNTH})
    if (MSVC)
        target_link_libraries(${TARGET_NAME} PUBLIC PkgConfig::gmp PkgConfig::gmpxx)
    else ()
        target_link_libraries(${TARGET_NAME} PUBLIC gmp gmpxx)
    endif ()
endif ()

gtest_discover_tests(${TARGET_NAME})
