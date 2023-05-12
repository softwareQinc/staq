# Installation instructions

## UNIX/UNIX-like

**staq** uses [CMake](https://cmake.org/) for its build system. To build both 
tool suite and the **staq** executable, execute under the root directory

```bash
cmake -B build
cmake --build build --parallel 8
  ```

To build only the **staq** tool suite, execute 
`cmake --build build --target tools --parallel 8`. To build only the **staq** 
executable, execute `cmake --build build --target staq --parallel 8`.
Unit tests can be built with the command 
`cmake --build build --target unit_tests --parallel 8`.

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
