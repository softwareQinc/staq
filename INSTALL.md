### Linux/UNIX

**staq** uses [CMake](https://cmake.org/) for its build system. To build both tool suite and the **staq** executable, from the root directory execute

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
from a Developer Command Prompt, from the root directory execute 

  ```
  mkdir build && cd build
  cmake .. 
  msbuild -m:8 -p:Configuration=Release staq.sln
````

assuming you are building from an out of source directory. 

To (un)install, type 

```
cmake --build . --target (UN)INSTALL
```