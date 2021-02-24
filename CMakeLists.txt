cmake_minimum_required(VERSION 3.13)

# 
# Project setup
#
project(somhunter)

add_compile_definitions("NOMINMAX")

# Disable "unsafe" warnings in the MSVC toolchain
add_compile_definitions("_CRT_SECURE_NO_WARNINGS")

set(BUILD_PLUGINS_STATIC ON
    CACHE BOOL "prevent problems with dynamic builds")

set(CMAKE_INCLUDE_CURRENT_DIR ON)

SET(WITH_CUDA 0 CACHE STRING "Set to 1 if you have CUDA installed.")

#
# Prepare dependencies
#
# ####################################
# Download & extract libTorch

set(LIBTORCH_INSTALL_DIR "${CMAKE_HOME_DIRECTORY}/3rdparty/")

# Prebuilt binaries URLs:
#   More here: https://pytorch.org/get-started/locally/

if (WITH_CUDA)
    
    if (CMAKE_BUILD_TYPE MATCHES Debug)
        set(LIBTORCH_URL_PREBUILT_WIN "https://download.pytorch.org/libtorch/cu102/libtorch-win-shared-with-deps-debug-1.7.1.zip")
    else ()
        set(LIBTORCH_URL_PREBUILT_WIN "https://download.pytorch.org/libtorch/cu102/libtorch-win-shared-with-deps-1.7.1.zip")
    endif ()
    
    set(LIBTORCH_URL_PREBUILT_UNIX "https://download.pytorch.org/libtorch/cu102/libtorch-cxx11-abi-shared-with-deps-1.7.1.zip")
else (WITH_CUDA)
    if (CMAKE_BUILD_TYPE MATCHES Debug)
        set(LIBTORCH_URL_PREBUILT_WIN "https://download.pytorch.org/libtorch/cpu/libtorch-win-shared-with-deps-debug-1.7.1%2Bcpu.zip")
    else ()
        set(LIBTORCH_URL_PREBUILT_WIN "https://download.pytorch.org/libtorch/cpu/libtorch-win-shared-with-deps-1.7.1%2Bcpu.zip")
    endif ()

    set(LIBTORCH_URL_PREBUILT_UNIX "https://download.pytorch.org/libtorch/cpu/libtorch-cxx11-abi-shared-with-deps-1.7.1%2Bcpu.zip")
endif (WITH_CUDA)


if (UNIX)
    execute_process (
        COMMAND sh "${CMAKE_HOME_DIRECTORY}/scripts/install_libtorch.sh" "${LIBTORCH_URL_PREBUILT_UNIX}" "${LIBTORCH_INSTALL_DIR}"
        ECHO_OUTPUT_VARIABLE
        ECHO_ERROR_VARIABLE
        RESULTS_VARIABLE INSTALL_LIBTORCH_RES
    )
endif (UNIX)

if (WIN32)

    message(WARNING "On Windows, libtorch Debug and Release binaries are not ABI compatible. If you're switching Rel/Deb settings make sure you delete the libtorch directory so the appropriate new type is downloaded.")

    execute_process (
        COMMAND powershell -ExecutionPolicy Bypass -File "${CMAKE_HOME_DIRECTORY}/scripts/install_libtorch.ps1" "${LIBTORCH_URL_PREBUILT_WIN}" "${LIBTORCH_INSTALL_DIR}"
        ECHO_OUTPUT_VARIABLE
        ECHO_ERROR_VARIABLE
        RESULTS_VARIABLE INSTALL_LIBTORCH_RES
    )
endif (WIN32)

# If the installation failed
if (INSTALL_LIBTORCH_RES)
    
    message(FATAL_ERROR "Unable to install `libTorch`!")

endif (INSTALL_LIBTORCH_RES)

set(CMAKE_PREFIX_PATH "${CMAKE_HOME_DIRECTORY}/3rdparty/libtorch/")

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-march=native" COMPILER_SUPPORTS_MARCH_NATIVE)
if(COMPILER_SUPPORTS_MARCH_NATIVE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
endif()

####################################

# 
# Include dependencies
#

# pthreads
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# cURL
find_package(CURL REQUIRED) 

# OpenCV - preparations
#find_package(OpenCV CONFIG REQUIRED)

# libTorch
find_package(Torch REQUIRED) 
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}") 

if (WIN32)
    # Don't know why!
    set_target_properties(torch PROPERTIES
    IMPORTED_IMPLIB "${CMAKE_HOME_DIRECTORY}/3rdparty/libtorch//lib/torch.lib"
    IMPORTED_LOCATION "${CMAKE_HOME_DIRECTORY}/3rdparty/libtorch//lib/torch.dll"
    )
endif (WIN32)

#
# Subdirectories
#
add_subdirectory(src)
add_subdirectory(3rdparty)
add_subdirectory(tests)
