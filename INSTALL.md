### Linux & Mac OS

**staq** uses CMake for its build system. To build the main **staq**
executable, from the root directory execute

  ```bash
  mkdir build && cd build
  cmake ..
  make -j8 staq
  ```

To build the **staq** tool suite, from the `build` directory, enter
`make -j8 tools`. To build both tool suite and the **staq** executable, type `make -j8`
Unit tests can be built with the command `make unit_tests`.

To (un)install, type `sudo make (un)install`.

### Windows

Building on Windows requires [Visual Studio](https://www.visualstudio.com) 2017
or later for cmake support. In Visual Studio, open
[CMakeLists.txt](https://github.com/softwareQinc/staq/blob/main/CMakeLists.txt)
as a cmake project, then simply build as a regular Visual Studio project or,
from a Developer Command Prompt, from the root directory execute 

  ```
  mkdir build && cd build
  cmake .. 
  msbuild -m:8 -p:Configuration=Release staq.sln
````

assuming you are building from an out of source directory. 

To (un)install, type `cmake --build . --target (UN)INSTALL`.
