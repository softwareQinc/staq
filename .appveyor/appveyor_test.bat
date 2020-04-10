@echo off
IF %COMPILER%==msvc2019 (
    %APPVEYOR_BUILD_FOLDER%\build\unit_tests\Debug\unit_tests.exe
)
IF %COMPILER%==msys2 (
    %APPVEYOR_BUILD_FOLDER%\build\unit_tests\unit_tests.exe
)
