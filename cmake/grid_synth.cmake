set(BUILD_GRID_SYNTH OFF)
if(MSVC)
  find_package(PkgConfig)
  if(PkgConfig_FOUND)
    # gmp, gmpxx
    pkg_check_modules(gmp IMPORTED_TARGET gmp)
    pkg_check_modules(gmpxx IMPORTED_TARGET gmpxx)
    if(gmp_FOUND AND gmpxx_FOUND)
      set(BUILD_GRID_SYNTH ON)
      add_compile_options("-DEXPR_GMP")
    endif()
  endif()
else()
  find_package(GMP)
  if(GMP_FOUND)
    set(BUILD_GRID_SYNTH ON)
    add_compile_options("-DEXPR_GMP")
  endif()
endif()
