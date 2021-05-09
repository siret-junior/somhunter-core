# SOMHunter-Core

Use cmake for build


# API Documentation
API documentation is available at `<server_address>/api/` (e.g. [http://localhost:8888/api/](http://localhost:8888/api/)). It requires the running core.

# Build

## Prerequisites
- `libcurl` (see below for installation on various platforms)
- `OpenCV` (see below for installation on various platforms)
- `libtorch` (this will be downloaded and installed during CMake script execution, you shouldn't worry about this)

## Windows
Because Windows does not have unified package manager with dev packages, you need to provide the dependency libraries manually OR use some other package manager. We recommend using [vcpkg](https://docs.microsoft.com/en-us/cpp/build/vcpkg?view=vs-2019) for dependency management. It works quite well with CMake (and also on Linux distros).

### Installing vcpkg
1) Download and install `vcpkg`
2) Install required dependencies for your architecture and system (e.g. for x64 windows)
```sh
./vcpkg install curl:x64-windows
./vcpkg install opencv4:x64-windows
```
3) Get path to the CMake toolchain file

```sh
./vcpkg integrate install
```

This will output how to tell CMake about this. Something like 
```
CMake projects should use: "-DCMAKE_TOOLCHAIN_FILE="c:/vcpkg/scripts/buildsystems/vcpkg.cmake"
```

### Building somhunter-core

Now just run CMake as usual with this additional option. Example for VS 2019 solution may look like this:
```sh
mkdir build
cd build

# Debug
cmake .. -G "Visual Studio 16 2019" -A x64  -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE="~/source/repos/vcpkg/scripts/buildsystems/vcpkg.cmake"
    
# Release
cmake .. -G "Visual Studio 16 2019" -A x64 -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE="~/source/repos/vcpkg/scripts/buildsystems/vcpkg.cmake" 
    
```


## UNIX systems

You should be able to install all dependencies from the package management. On
Debian-based systems (including Ubuntu and Mint) the following should work:

```sh
apt-get install build-essential libcurl4-openssl-dev libopencv-dev 
```

```bash
mkdir build && cd build

# Debug
cmake .. -DCMAKE_BUILD_TYPE="Debug"

# Release
cmake .. -DCMAKE_BUILD_TYPE="Release"
```

Similar (similarly named) packages should be available on most other distributions.

