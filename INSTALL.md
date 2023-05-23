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
- C++17 compliant compiler, e.g., [gcc](https://gcc.gnu.org/)
  , [clang](https://clang.llvm.org)
  , [MSVC](https://visualstudio.microsoft.com/vs/) etc.

## UNIX/UNIX-like

To build both tool suite and the **staq** executable, execute 
(in a terminal/console/command prompt) under the project's root directory

```bash
cmake -B build
```

followed by

```bash
cmake --build build --parallel 8
```

The `--parallel 8` instructs CMake to build in parallel using 8 threads, modify 
accordingly.

To build only the **staq** tool suite, execute

```bash
cmake --build build --target tools --parallel 8
````
To build only the **staq** executable, execute

```bash
cmake --build build --target staq --parallel 8
```

Unit tests can be built with the command

```bash
cmake --build build --target unit_tests --parallel 8
```

Tu run the unit tests, execute

```bash
ctest --test-dir build
```

To (un)install, execute 

```bash
sudo cmake --build build --target (un)install
```

## Windows

Building on Windows requires [Visual Studio](https://www.visualstudio.com) 2017
or later for CMake support. In Visual Studio, open
[CMakeLists.txt](https://github.com/softwareQinc/staq/blob/main/CMakeLists.txt)
as a CMake project, then simply build as a regular Visual Studio project or,
from an Administrator Developer Command Prompt, execute under the root directory 

```
cmake -B build
cmake --build build --parallel 8
```

To (un)install, execute 

```
cmake --build build --target (UN)INSTALL
```

### macOS/Linux

If you are running macOS or Linux, you can install **staq** via
[Homebrew](https://brew.sh) with

    brew install staq

## Python 3 wrapper
[pystaq](https://github.com/softwareQinc/staq/blob/main/pystaq/) is a Python 3 
wrapper for **staq**. pystaq can be installed using `pip`

```
pip install git+https://github.com/softwareQinc/staq
```

For more details, please see 
[pystaq/README.md](https://github.com/softwareQinc/staq/blob/main/pystaq/README.md).
