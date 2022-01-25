# staq

## Version 2.1 - 17 January 2022

**Build status:**

[![Build status - CircleCI Linux/macOS](https://circleci.com/gh/softwareQinc/staq.svg?style=svg)](https://circleci.com/gh/softwareQinc/staq)
[![Build status](https://ci.appveyor.com/api/projects/status/gwc8fde2jdp3tol7?svg=true)](https://ci.appveyor.com/project/vsoftco/staq)

---

## About

**staq** is a modern C++17 library for the synthesis, transformation,
optimization and compilation of quantum circuits. It is usable either through
the provided binary tools, or as a header-only library that can be included to
provide direct support for parsing & manipulating circuits written in
the [OpenQASM](https://github.com/Qiskit/openqasm) circuit description language.

Inspired by Clang, **staq** is designed to manipulate OpenQASM syntax trees
directly, rather than through an intermediate representation which makes
retrieving the original source code impossible. In particular, OpenQASM circuits
can be inspected and transformed (in most cases) without losing the original
source structure. This makes **staq** ideally suited for source-to-source
transformations, where only specific changes are desired. Likewise, this allows
translations to other common circuit description languages and libraries to
closely follow the OpenQASM source.

Check out the [Wiki](https://github.com/softwareQinc/staq/wiki) for more
information about the library and included tools.

---

## License

**staq** is distributed under the MIT license. Please see the
[`LICENSE`](https://github.com/softwareQinc/staq/blob/main/LICENSE) file for more details.

---

## Installation instructions

### Linux/UNIX

**staq** uses [CMake](https://cmake.org/) for its build system. To build both tool suite and the **staq** executable, execute under the root directory

```bash
mkdir build && cd build
cmake ..
make -j8
  ```

To build only the **staq** tool suite, from the `build` directory, enter
`make -j8 tools`. To build only the **staq** executable, type `make -j8 staq`
Unit tests can be built with the command `make -j8 unit_tests`.

To (un)install, type 

```bash
sudo make (un)install
```

### Windows

Building on Windows requires [Visual Studio](https://www.visualstudio.com) 2017
or later for cmake support. In Visual Studio, open
[CMakeLists.txt](https://github.com/softwareQinc/staq/blob/main/CMakeLists.txt)
as a cmake project, then simply build as a regular Visual Studio project or,
from a Developer Command Prompt, execute under the root directory 

```
mkdir build && cd build
cmake .. 
msbuild -m:8 -p:Configuration=Release staq.sln
```

assuming you are building from an out of source directory. 

To (un)install, type 

```
cmake --build . --target (UN)INSTALL
```

## Python 3 wrapper
pystaq is a Python 3 wrapper for staq.
pystaq can be installed using `pip`:
```
pip install git+https://github.com/softwareQinc/staq
```
For more details, please see 
[pystaq/README.md](https://github.com/softwareQinc/staq/blob/main/pystaq/README.md).

---

## Acknowledgements

Thanks to the
excellent [EPFL logic synthesis libraries](https://github.com/lsils/lstools-showcase)
which are used to perform logic synthesis in **staq**, and in particular Bruno
Schmitt's
[tweedledum](https://github.com/boschmitt/tweedledum) library, from which the
OpenQASM parser was adapted.
