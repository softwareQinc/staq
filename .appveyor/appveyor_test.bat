@echo off
IF %COMPILER%==msvc2019 (
    ctest
)
IF %COMPILER%==msys2 (
    ctest
)
