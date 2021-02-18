# SOMHunter-Core

Use cmake for build


## Building from source

Prerequisites:
- `libcurl` (see below for installation on various platforms)
- `libopencv`
- `libtorch` (libraries will be downloaded by the CMake build script)

### Getting the dependencies on UNIX systems

You should be able to install all dependencies from the package management. On
Debian-based systems (including Ubuntu and Mint) the following should work:

```
apt-get install build-essential libcurl4-openssl-dev nodejs yarnpkg
```

The build system uses `pkg-config` to find libCURL -- if that fails, either
install the CURL pkgconfig file manually, or customize the build configuration
in `core/binding.gyp` to fit your setup.

Similar (similarly named) packages should be available on most other distributions.

### Getting the dependencies on Windows

The build systems expects dependency libraries to reside in the `c:\Program Files\curl\` directory. You
may want to install it using
[vcpkg](https://docs.microsoft.com/en-us/cpp/build/vcpkg?view=vs-2019) as
follows:

- download and install `vcpkg`
- install and export `libcurl` & `libopencv`:
```
vcpkg install curl:x64-windows opencv3:x64-windows
vcpkg export --raw curl:x64-windows opencv3:x64-windows
```
- copy the directories with the exported libs to `c:\Program Files\curl\` & `c:\Program Files\opencv\` so that directories `include` and `bin` lie directly inside those.

Second option is to provide CMake the appropriate toolchain path to your `vcpkg` directory (e.g. `-DCMAKE_TOOLCHAIN_FILE="c:\vcpkg\scripts\buildsystems\vcpkg.cmake"`). With this you won't need to do the exporting.
