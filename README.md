# SOMHunter-Core

Use cmake for build


# Build

## Prerequisites
- `libcurl` (see below for installation on various platforms)
- `libtorch` (this will be downloaded and installed during CMake script execution, you shouldn't worry about this)

## UNIX systems

You should be able to install all dependencies from the package management. On
Debian-based systems (including Ubuntu and Mint) the following should work:

```sh
apt-get install build-essential libcurl4-openssl-dev 
```

```sh
mkdir build && cd build

# Debug
cmake .. -DCMAKE_CXX_FLAGS="${CMAKE_CXX_FLAGS} -march=native"-DCMAKE_BUILD_TYPE="Debug"

# Release
cmake .. -DCMAKE_CXX_FLAGS="${CMAKE_CXX_FLAGS} -march=native"-DCMAKE_BUILD_TYPE="Release"
```

Similar (similarly named) packages should be available on most other distributions.

## Windows

The build systems expects dependency installed at "usual" locations. That is usually `c:\Program Files\curl\` directory. 
But to avoid any problems is is good to use [vcpkg](https://docs.microsoft.com/en-us/cpp/build/vcpkg?view=vs-2019) for managing those for you.

1) Download and install `vcpkg`
2) Install libcurl (dependecncies will be handled by the package manager)
```sh
vcpkg install curl:x64-windows
```
This will output how to tell CMake about this. Something like `-DCMAKE_TOOLCHAIN_FILE="c:\vcpkg\scripts\buildsystems\vcpkg.cmake"`).

Now just run CMake as usual with this additional option. Example for VS 2019 solution may look like this:
```sh
mkdir build && cd build

# Debug
cmake .. -G "Visual Studio 16 2019" -A x64  -DCMAKE_TOOLCHAIN_FILE="c:\vcpkg\scripts\buildsystems\vcpkg.cmake" -DCMAKE_BUILD_TYPE="Debug"

# Release
cmake .. -G "Visual Studio 16 2019" -A x64  -DCMAKE_TOOLCHAIN_FILE="c:\vcpkg\scripts\buildsystems\vcpkg.cmake" -DCMAKE_BUILD_TYPE="Release"
```
