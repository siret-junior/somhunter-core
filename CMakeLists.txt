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

# 
# Global dependencies
#

# pthreads
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# cURL
find_package(CURL REQUIRED) 

#
# ##########################
#
add_subdirectory(src)
add_subdirectory(3rdparty)
add_subdirectory(tests)
