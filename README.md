# SOMHunter-Core
The main SOMHunter Search Tool part.

# Build
## Prerequisites
- `libcurl`- [https://curl.se/libcurl/](https://curl.se/libcurl/)
- `OpenCV` - [https://opencv.org/](https://opencv.org/)
- `cpprestsdk` - [https://github.com/microsoft/cpprestsdk](https://github.com/microsoft/cpprestsdk)
- `libtorch`  - [https://pytorch.org/](https://pytorch.org/)
  - (this will be downloaded and installed during CMake script execution, you shouldn't worry about this)

## Windows
Because Windows does not have a unified package manager with dev packages, you need to provide the dependency libraries manually OR use some other package manager. We recommend using [vcpkg](https://docs.microsoft.com/en-us/cpp/build/vcpkg?view=vs-2019) for dependency management. It works quite well with CMake (and also on Linux distros).

### Dependencies
#### Installing vcpkg
1) Download and install `vcpkg`
2) Install required dependencies for your architecture and system (e.g. for x64 windows)
```sh
./vcpkg install curl:x64-windows
./vcpkg install opencv4:x64-windows
./vcpkg install cpprestsdk:x64-windows
```
3) Get path to the CMake toolchain file

```sh
./vcpkg integrate install

# This will output how to tell CMake about this. Something like 
CMake projects should use: "-DCMAKE_TOOLCHAIN_FILE="c:/vcpkg/scripts/buildsystems/vcpkg.cmake"
```

### Generate the build
Now just run CMake as usual with this additional option. An example for VS 2019 solution may look like this:
```sh
mkdir build
cd build

# Debug
cmake .. -G "Visual Studio 16 2019" -A x64  -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE="~/source/repos/vcpkg/scripts/buildsystems/vcpkg.cmake"
    
# Release
cmake .. -G "Visual Studio 16 2019" -A x64 -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE="~/source/repos/vcpkg/scripts/buildsystems/vcpkg.cmake" 
    
```
### Build the core
Just use Visual Studio, or
```sh
cmake --build . --config Debug
cmake --build . --config Release
```


## UNIX systems

You should be able to install all dependencies from the package management. On
Debian-based systems (including Ubuntu and Mint) the following should work:

> Similar (similarly named) packages should be available on most other distributions.

### Dependencies
```sh
# Install dependencies
apt-get install build-essential libcurl4-openssl-dev libopencv-dev libcpprest-dev
```

### Generate the build
```sh
mkdir build && cd build

# Debug
cmake .. -DCMAKE_BUILD_TYPE="Debug"

# Release
cmake .. -DCMAKE_BUILD_TYPE="Release"
```

### Build the core
```sh
make -j
```

# Core HTTP API
API documentation is available at `<server_address>/api/` (e.g. [http://localhost:8888/api/](http://localhost:8888/api/)). It requires the running core.

# FAQ
## 1.  *I'm am getting an error saying \"python not found\".*
Do you have Python 3 installed? Maybe you don't have it aliased on `python` command. Consider aliasing it (or use something like `apt install python-is-pyton3`).

## 2. *Can I somehow debug the collage queries?*
* Every collage rescore is logged into `logs/collages/<timestamp>/*`.
* There you can find binary dump (e.g. `Collage_instance_serialized.bin`) that you can use to simulate that query when working on `somhunter-core` without the UI.
    * Just look for the `TEST_COLLAGE_QUERIES` define inside `somhunter-core` to see how you can use those.
* Used images are there as well as JFIFs.
* Query info is inside the `query_info.json`



