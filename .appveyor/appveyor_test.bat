@echo off
IF %COMPILER%==msvc2022 (
    ctest --test-dir build
)
IF %COMPILER%==msys2 (
    ctest --test-dir build
)
