# staq

## Version 1.4 - 5 December 2020

**Build status:**

[![Build status - CircleCI Linux/macOS](https://circleci.com/gh/softwareQinc/staq.svg?style=svg)](https://circleci.com/gh/softwareQinc/staq)
[![Build status](https://ci.appveyor.com/api/projects/status/gwc8fde2jdp3tol7?svg=true)](https://ci.appveyor.com/project/vsoftco/staq)

---

## About

**staq** is a modern C++17 library for the synthesis, transformation,
optimization and compilation of quantum circuits. It is usable either through
the provided binary tools, or as a header-only library that can be included to
provide direct support for parsing & manipulating circuits written in
the [openQASM](https://github.com/Qiskit/openqasm) circuit description language.

Inspired by Clang, **staq** is designed to manipulate openQASM syntax trees
directly, rather than through an intermediate representation which makes
retrieving the original source code impossible. In particular, openQASM circuits
can be inspected and transformed (in most cases) without losing the original
source structure. This makes **staq** ideally suited for source-to-source
transformations, where only specific changes are desired. Likewise, this allows
translations to other common circuit description languages and libraries to
closely follow the openQASM source.

Check out the [Wiki](https://github.com/softwareQinc/staq/wiki) for more
information about the library and included tools.

---

## Installation

### Linux & Mac OS

**staq** uses CMake for its build system. To build the main **staq**
executable, from the root directory execute

  ```bash
  mkdir build && cd build
  cmake ..
  make -j4 staq
  ```

To build the **staq** tool suite, from the `build` directory, enter
`make -j4 tools`. To build both tool suite and the **staq** executable, type `make -j4`
Unit tests can be built with the command `make unit_tests`.

### Windows

Building on Windows requires [Visual Studio](https://www.visualstudio.com) 2017
or later for cmake support. In Visual Studio, open
[CMakeLists.txt](https://github.com/softwareQinc/staq/blob/main/CMakeLists.txt)
as a cmake project, then simply build as a regular Visual Studio project.

---

## License

**staq** is distributed under the MIT license. Please see the
[`LICENSE`](https://github.com/softwareQinc/staq/blob/main/LICENSE) file for
more details.

---

## Acknowledgements

Thanks to the
excellent [EPFL logic synthesis libraries](https://github.com/lsils/lstools-showcase)
which are used to perform logic synthesis in **staq**, and in particular Bruno
Schmitt's
[tweedledum](https://github.com/boschmitt/tweedledum) library, from which the
openQASM parser was adapted.
