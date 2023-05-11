# staq

## Version 3.0.1 - 2 April 2023

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

Please see the installation guide
[`INSTALL.md`](https://github.com/softwareQinc/staq/blob/main/INSTALL.md) and the
comprehensive [Wiki](https://github.com/softwareQinc/staq/wiki) for further
documentation and detailed examples.

## Python 3 wrapper
[pystaq](https://github.com/softwareQinc/staq/blob/main/pystaq/) is a Python 3
wrapper for **staq**. pystaq can be installed using `pip`

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
