# Version 3.5 - 8 March 2024

- Replaced ["CHANGES"] by ["CHANGES.md"],
  as we now use Markdown format to keep track of changes in new releases
- Removed pybind11 and GoogleTest dependencies; if not detected, they are
  installed automatically as build dependencies by CMake
- Bumped GoogleTest version to HEAD latest, as
  [recommended by Google](https://github.com/google/googletest?tab=readme-ov-file#live-at-head)
- All header files are moved into ["include/staq"], so to include **staq**
  headers one now must `#include "staq/<header>.hpp"`. This change was made for
  the sake of making the include statements look uniform in both non-installed
  (compiled without having **staq** installed) and installed
  (compiling with **staq** installed headers) modes.
- Integrated the `staq_ionq` OpenQASM2 -> IonQ transpiler into **pystaq**

# Version 3.4 - 1 December 2023

- When configuring **staq** with `cmake -B build -DINSTALL_SOURCES=ON`,
  `cmake --build build --target install` now installs **staq**'s source code in
  addition to the binaries
- Renamed the ["examples"] directory to ["misc"]
- Moved ["qpu_specs"] and ["scripts"] directories to ["misc"]
- Added standalone source code example (requires **staq**'s source
  installation) in the ["examples/standalone"] directory
- Added `staq_ionq` OpenQASM2 -> IonQ transpiler

# Version 3.3 - 6 October 2023

- Implemented the
  [grid synth rotation synthesizer algorithm](https://arxiv.org/abs/1403.2975),
  enabled only when the GNU MP library
  is detected. When enabled, the build will include `staq_grid_synth` and
  `staq_qasm_synth`.

# Version 3.2.3 - 14 August 2023

- Minor bugfix in **pystaq** ["setup.py"] that prevented `pip install`
  from remote
- Docker update, see the ["docker"] directory

# Version 3.2.2 - 12 June 2023

- This is a maintenance release
- Compiling errors fixed on SunOS/OpenIndiana

# Version 3.2.1

- This is a maintenance release
- CMake dependent flag name changes:
  `USE_OPENQASM2_SPECS` -> `QASMTOOLS_QASM2_SPECS`

# Version 3.2 - 26 May 2023

- This is a maintenance release
- Migrated all continuous integration to GitHub actions
- Minor updates in qasmtools

# Version 3.1 - 13 May 2023

- **staq** is now available on Homebrew (macOS/Linux), and can be installed
  with `brew install staq`
- Migrated **pystaq** installation method to ["pyproject.toml"]
- CMake minimum required version bumped to 3.15

# Version 3.0.1 - 2 April 2023

- This is a maintenance release with minor changes
- Updated pybind11 to version 2.10.4

# Version 3.0 - 20 March 2023

- Complete Windows support for **pystaq**
- Bumped
  [GoogleTest version to 1.12.1](https://github.com/google/googletest/commit/58d77fa8070e8cec2dc1ed015d66b454c8d78850)
- Implemented a lattice surgery compiler ["include/output/lattice_surgery.hpp"]

# Version 2.1 - 17 January 2022

- Added **pystaq**, a Python wrapper around **staq**. See
  [this](https://github.com/softwareQinc/staq/wiki/pystaq) for more details.
- Due to phase discrepancies, by default the parser now uses Qiskit definitions
  (which are also the usual ones used in QC textbooks). To switch to standard
  OpenQASM 2.0 gate definitions, configure the project with
  `cmake -DUSE_OPENQASM2_SPECS=ON`.
- When building with CMake, the new `STAQ_VERSION_NUM` (numeric) and
  `STAQ_VERSION_STR` (string) preprocessor definitions are automatically
  injected in the code

# Version 2.0 - 5 October 2021

- Decoupled the OpenQASM parser from the main codebase. A hard copy of
  [qasmtools](https://github.com/softwareQinc/qasmtools) is now common to both
  **staq** and **Quantum++**, and by default uses standard OpenQASM 2.0 gate
  definitions. To switch to Qiskit definitions (which are also the usual ones
  used in QC textbooks), configure the project with
  `cmake -DUSE_QISKIT_SPECS=ON`.
- Added CMake installation support
- Switched continuous integration from Travis CI to CircleCI
- CMake minimum required version bumped to 3.12 for automatic unit tests
  detection by CMake
- Unit testing is now a separate CMake target, one needs to explicitly type
  `make unit_tests` to build the unit testing suite
- Simplified unit testing, now one can run tests with `ctest` or `make test`
  (after explicitly built with `make unit_tests`). Use
  `GTEST_COLOR=1 ARGS="-V" make test` or `GTEST_COLOR=1 ctest -V` for coloured
  verbose testing output.
- QPU specifications (qubits, couplings, fidelities) are now stored in
  [JSON files](https://github.com/softwareQinc/staq/tree/main/misc/qpu_specs)
- Added a device generator to easily create these JSON files under
  ["tools/device_generator.cpp"].

# Version 1.4 - 5 December 2020

- Bugfixes
- Renamed `master` branch to `main`

# Version 1.3 - 11 June 2020

- Bugfix release

# Version 1.2 - 13 May 2020

- Minor incremental improvements/bugfixes

# Version 1.1 - 8 April 2020

- Added complete Windows support (including MSVC) + AppVeyor CI
- Various minor bugfixes

# Version 1.0 - 10 December 2019

- Initial public release
