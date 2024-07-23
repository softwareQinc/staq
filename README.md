# staq

## Version 3.5 - 8 March 2024

[![GitHub actions](https://github.com/softwareqinc/staq/actions/workflows/cmake.yml/badge.svg)](https://github.com/softwareQinc/staq/actions)

---

## About

**staq** is a modern C++ library for the synthesis, transformation,
optimization and compilation of quantum circuits.
**staq** is written in standard C++17 and has very low external dependencies.
It is usable either through the provided binary tools, or as a header-only
library that can be included to provide direct support for
parsing & manipulating circuits written in the
[OpenQASM 2.0](https://github.com/openqasm/openqasm/tree/OpenQASM2.x) circuit
description language.

Inspired by Clang, **staq** is designed to manipulate OpenQASM 2.0 syntax trees
directly, rather than through an intermediate representation which makes
retrieving the original source code impossible. In particular, OpenQASM 2.0
circuits can be inspected and transformed (in most cases) without losing the
original source structure. This makes **staq** ideally suited for
source-to-source transformations, where only specific changes are desired.
Likewise, this allows translations to other common circuit description
languages and libraries to closely follow the OpenQASM 2.0 source.

Check out the [Wiki](https://github.com/softwareQinc/staq/wiki) for more
information about the library and included tools.

Copyright (c) 2019 - 2024 softwareQ Inc. All rights reserved.

---

## License

**staq** is distributed under the MIT license. Please see the
[`LICENSE.txt`](https://github.com/softwareQinc/staq/blob/main/LICENSE.txt)
file for more details.

---

## Installation instructions

Please see the installation guide
[`INSTALL.md`](https://github.com/softwareQinc/staq/blob/main/INSTALL.md) and
the comprehensive [Wiki](https://github.com/softwareQinc/staq/wiki) for further
documentation and detailed examples.

---

## Python 3 wrapper

[**pystaq**](https://github.com/softwareQinc/staq/blob/main/pystaq/) is a
Python 3 wrapper for **staq**. **pystaq** can be installed using `pip`

```shell
pip install git+https://github.com/softwareQinc/staq
```

For more details, please see
[pystaq/README.md](https://github.com/softwareQinc/staq/blob/main/pystaq/README.md).

---

## Acknowledgements

Thanks to the excellent
[EPFL logic synthesis libraries](https://github.com/lsils/lstools-showcase)
which are used to perform logic synthesis in **staq**, and in particular Bruno
Schmitt's [tweedledum](https://github.com/boschmitt/tweedledum) library, from
which the OpenQASM 2.0 parser was adapted.
