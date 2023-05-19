@echo off
IF %COMPILER%==msvc2022 (
    @echo on
    CALL "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"

    cd %APPVEYOR_BUILD_FOLDER%
    cmake -B build
    cmake --build build --parallel 4
    cmake --build build --target unit_tests --parallel 4
)
IF %COMPILER%==msys2 (
    @echo on
    SET "PATH=C:\msys64\mingw64\bin;%PATH%"

    cd %APPVEYOR_BUILD_FOLDER%
    bash -lc "cmake -B build"
    bash -lc "cmake --build build --parallel 4"
    bash -lc "cmake --build build --target unit_tests --parallel 4"
)
