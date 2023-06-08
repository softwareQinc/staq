# Installation instructions

**staq** is a full-stack quantum processing toolkit that uses
[CMake](https://cmake.org/) as its build/install system. **staq** is
platform-independent, supporting
[UNIX](https://www.opengroup.org/membership/forums/platform/unix)
(including
[macOS](https://www.apple.com/macos/)) and UNIX-like operating systems
(e.g., [Linux](https://www.linux.org)), as well
as [Windows](https://www.microsoft.com/en-us/windows).

## Pre-requisites

- [CMake](https://cmake.org/)
- C++17 compliant compiler, e.g.,
  [gcc](https://gcc.gnu.org/),
  [clang](https://clang.llvm.org),
  [MSVC](https://visualstudio.microsoft.com/vs/)

## UNIX/UNIX-like/Windows

To build both tool suite and the **staq** executable, execute
(in a terminal/console/command prompt) under the project's root directory

```shell
cmake -B build
```

followed by

```shell
cmake --build build --parallel 8
```

The `--parallel 8` instructs CMake to build in parallel using 8 threads, modify
accordingly.

To build only the **staq** tool suite, execute

```shell
cmake --build build --target tools --parallel 8
````

To build only the **staq** executable, execute

```shell
cmake --build build --target staq --parallel 8
```

Unit tests can be built with the command

```shell
cmake --build build --target unit_tests --parallel 8
```

and run with

```shell
ctest --test-dir build
```

To (un)install, execute in a terminal/console (UNIX/UNIX-like systems)

```shell
sudo cmake --build build --target (un)install
```

or in an Administrator Command Prompt (Windows)

```shell
cmake --build build --target (un)install
```

## macOS/Linux

If you are running macOS or Linux, you can install **staq** via
[Homebrew](https://brew.sh) with

```shell
brew install staq
```

## Python 3 wrapper

[pystaq](https://github.com/softwareQinc/staq/blob/main/pystaq/) is a Python 3
wrapper for **staq**. pystaq can be installed using `pip`

```shell
pip install git+https://github.com/softwareQinc/staq
```

For more details, please see
[pystaq/README.md](https://github.com/softwareQinc/staq/blob/main/pystaq/README.md).
