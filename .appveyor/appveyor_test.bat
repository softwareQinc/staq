@echo off
IF %COMPILER%==msvc2019 (
    ctest --test-dir build
)
IF %COMPILER%==msys2 (
    ctest --test-dir build
)
