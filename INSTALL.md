# Installation instructions

**staq** is a full-stack quantum processing toolkit that uses
[CMake](https://cmake.org/) as its build/install system. **staq** is
platform-independent, supporting
[UNIX](https://www.opengroup.org/membership/forums/platform/unix) (including
[macOS](https://www.apple.com/macos/)) and UNIX-like operating systems (e.g.,
[Linux](https://www.linux.org)), as well as
[Windows](https://www.microsoft.com/en-us/windows).

---

## Pre-requisites

- [CMake](https://cmake.org/)
- pp+17 compliant compiler, e.g.,
  [gcc](https://gcc.gnu.org/),
  [clang](https://clang.llvm.org),
  [MSVC](https://visualstudio.microsoft.com/vs/)

- Optional, [GNU MP](https://gmplib.org/) library for building the grid synth
  tools `staq_grid_synth` and `staq_qasm_synth`

---

## UNIX/UNIX-like/Windows

To build both tool suite and the **staq** executable, execute (in a
terminal/console/command prompt) under the project's root directory

```shell
cmake -B build
```

To be able to install **staq**'s source code in addition to the binaries,
configure the system with

```shell
cmake -B build -DINSTALL_SOURCES=ON
```

**Important**: If you want to build the grid synth tools `staq_grid_synth` and
`staq_qasm_synth`, install the [GNU MP library](https://gmplib.org/); `cmake`
will take care of the rest. If `cmake` cannot detect GNU MP, then the grid
synth tools will not be part of the build. To install GNU MP on Windows
systems, please follow the [platform-specific instructions below](#windows).

For more details about how to install and configure GNU MP on various platforms,
see the
[GitHub Actions configuration file](https://github.com/softwareQinc/staq/blob/main/.github/workflows/cmake.yml).

Next, build the **staq** system by executing

```shell
cmake --build build --parallel 8
```

The `--parallel 8` flag instructs CMake to build in parallel using 8 threads,
modify accordingly.

To build only the **staq** tool suite, execute

```shell
cmake --build build --target tools --parallel 8
```

To build only the **staq** executable, execute

```shell
cmake --build build --target staq --parallel 8
```

Unit tests can be built with the command

```shell
cmake --build build/unit_tests --target unit_tests --parallel 8
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

If you configured the system with `-DINSTALL_SOURCES=ON`, **staq**'s source
code will be installed in `/usr/local/include/staq` (UNIX/UNIX-like systems), or
in `C:\Program Files (x86)\staq` on Windows systems. The paths may differ on
your system. To use **staq**'s source code, precede all include paths by `staq`
in your own code, i.e.,

```cpp
#include <staq/qasmtools/parser/parser.hpp>
```

Third party header-only libraries used internally by **staq** need to be
preceded by `third_party` when including their corresponding header file(s),
i.e.,

```cpp
#include <staq/third_party/CLI/CLI.hpp>
```

See the
[standalone example](https://github.com/softwareQinc/staq/tree/main/examples/standalone)
for more details.

---

## macOS/Linux

If you are running macOS or Linux, you can install **staq** via
[Homebrew](https://brew.sh) with

```shell
brew install staq
```

---

## Windows

On Windows, we recommend to install GNU MP via
[vcpkg](https://vcpkg.io/en/index.html) and pkgconf. Install vcpkg according to
the instructions from https://vcpkg.io/en/getting-started by executing in a
Command Prompt

```shell
git clone https://github.com/Microsoft/vcpkg.git
.\vcpkg\bootstrap-vcpkg.bat
```

Next, install pkgconf and GNU MP by executing

```shell
.\vcpkg\vcpkg install pkgconf:x64-windows gmp:x64-windows
```

This may take a while...

Finally, configure the system with the additional flag
`-DCMAKE_TOOLCHAIN_FILE=[path to vcpkg]/scripts/buildsystems/vcpkg.cmake`, e.g.,

```shell
cmake -B build -DCMAKE_TOOLCHAIN_FILE=./vckpg/scripts/buildsystems/vcpkg.cmake -DINSTALL_SOURCES=ON
```

followed by building the system as usual.

---

## Python 3 wrapper

[**pystaq**](https://github.com/softwareQinc/staq/blob/main/pystaq/) is a
Python 3 wrapper for **staq**. **pystaq** can be installed using `pip`

```shell
pip install git+https://github.com/softwareQinc/staq
```

For more details, please see
[pystaq/README.md](https://github.com/softwareQinc/staq/blob/main/pystaq/README.md).
