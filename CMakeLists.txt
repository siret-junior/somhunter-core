
# ####################################
# General project setup
# ####################################

cmake_minimum_required(VERSION 3.13)

project(somhunter)

set(BUILD_PLUGINS_STATIC ON
    CACHE BOOL "prevent problems with dynamic builds")

set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Specify build targets
set(SOMHUNTER_TARGET somhunter)
set(SOMHUNTER_TARGET_TESTS somhunter-tests)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

SET(WITH_CUDA 0 CACHE STRING "Set to 1 if you have CUDA installed.")
set(THIRD_PARTY_DIR "${CMAKE_HOME_DIRECTORY}/third-party")

# Output dirs
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR} )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR} )
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR} )

if (MSVC)
# Set warning level 0 to everything included with angle brackets
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /MP /experimental:external /external:anglebrackets /external:W0" )

    foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
        string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
        set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR} )
        set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR} )
        set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR} )
    endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )

else (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp -frtti -D_GLIBCXX_USE_CXX11_ABI=1")
endif (MSVC)

# -march=native option
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-march=native" COMPILER_SUPPORTS_MARCH_NATIVE)
if(COMPILER_SUPPORTS_MARCH_NATIVE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
endif()

if (CMAKE_BUILD_TYPE)
else ()
    message("Setting default CMAKE_BUILD_TYPE")
    set(CMAKE_BUILD_TYPE Debug)
endif ()

if (WIN32)
    set(PLATFORM "win")
else ()
    set(PLATFORM "UNIX")
endif()
message("Platform is '${PLATFORM}'...")

if (WITH_CUDA)
    set(CUDA_SCRIPT_OPT "--cuda")
else ()
endif ()




# ####################################
# Global preprocessor defines
# ####################################
add_compile_definitions("NOMINMAX")
# Disable "unsafe" warnings in the MSVC toolchain
add_compile_definitions("_CRT_SECURE_NO_WARNINGS")

# ####################################
# Preparation scripts
# ####################################

#
# Configure python script
#
message("vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv")
message("Running 'config.py' script, this may take a while (it's downloading large files)...")
message("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^")
execute_process (
    COMMAND python ${CMAKE_HOME_DIRECTORY}/scripts/configure.py 
            ${PLATFORM}                                 # What platform \in {win, UNIX}
            ${CMAKE_HOME_DIRECTORY}/config/config-core.json         # Config file
            ${CMAKE_HOME_DIRECTORY}/config/config-install.json # Install config file
            ${CMAKE_HOME_DIRECTORY}/config/config-auth.json # Auth config
            ${CMAKE_BINARY_DIR}                         # Build dir
            ${THIRD_PARTY_DIR}                          # Third-party dir
            "${CMAKE_BUILD_TYPE}"                       # Build type \in { Release, Debug }
            ${CUDA_SCRIPT_OPT}
        WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}
        RESULTS_VARIABLE conf_res
)
if (conf_res)
    message(FATAL_ERROR "Configure script failed!")
else ()
    message("Configure completed.")
endif ()

# ####################################
# Dependencies
# ####################################

find_package(cpprestsdk REQUIRED)

# libTorch
set(Torch_DIR "${THIRD_PARTY_DIR}/libtorch-${CMAKE_BUILD_TYPE}/libtorch/share/cmake/Torch/")
message(${Torch_DIR})
find_package(Torch REQUIRED) 
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}") 
if (WIN32)
    # Don't know why!
    set_target_properties(torch PROPERTIES
        IMPORTED_IMPLIB "${THIRD_PARTY_DIR}/libtorch-${CMAKE_BUILD_TYPE}/libtorch//lib/torch.lib"
        IMPORTED_LOCATION "${THIRD_PARTY_DIR}/libtorch-${CMAKE_BUILD_TYPE}/libtorch//lib/torch.dll"
    )
    set_target_properties(torch_cpu PROPERTIES
        IMPORTED_IMPLIB "${THIRD_PARTY_DIR}/libtorch-${CMAKE_BUILD_TYPE}/libtorch//lib/torch_cpu.lib"
        IMPORTED_LOCATION "${THIRD_PARTY_DIR}/libtorch-${CMAKE_BUILD_TYPE}/libtorch//lib/torch_cpu.dll"
    )
    set_target_properties(c10 PROPERTIES
        IMPORTED_IMPLIB "${THIRD_PARTY_DIR}/libtorch-${CMAKE_BUILD_TYPE}/libtorch//lib/c10.lib"
        IMPORTED_LOCATION "${THIRD_PARTY_DIR}/libtorch-${CMAKE_BUILD_TYPE}/libtorch//lib/c10.dll"
    )
endif (WIN32)

# pthreads
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# cURL
find_package(CURL REQUIRED) 

# OpenCV - preparations
find_package(OpenCV REQUIRED)

find_package(OpenSSL REQUIRED)

#
# Add structure to the project (as the FS structure)
#
file(
  GLOB_RECURSE SOMHUNTER_SOURCE_FILEPATHS
  "*.h"
  "*.hpp"
  "*.cpp"
  "*.cc"
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOMHUNTER_SOURCE_FILEPATHS}) 

# ####################################
# Create the target
# ####################################
add_executable(${SOMHUNTER_TARGET})
target_compile_features(${SOMHUNTER_TARGET} PRIVATE cxx_std_17)
set_target_properties(${SOMHUNTER_TARGET} PROPERTIES CXX_STANDARD 17)


add_executable(${SOMHUNTER_TARGET_TESTS})
target_compile_features(${SOMHUNTER_TARGET_TESTS} PRIVATE cxx_std_17)
set_target_properties(${SOMHUNTER_TARGET_TESTS} PROPERTIES CXX_STANDARD 17)

# Add testing definition
target_compile_definitions(${SOMHUNTER_TARGET_TESTS} PRIVATE TESTING)

# ####################################
# Linking
# ####################################

# SET_TARGET_PROPERTIES(${SOMHUNTER_TARGET} PROPERTIES LINK_FLAGS    "/MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\"")
# SET_TARGET_PROPERTIES(${SOMHUNTER_TARGET_TESTS} PROPERTIES LINK_FLAGS    "/MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\"")

target_include_directories(${SOMHUNTER_TARGET} 
	PRIVATE
		${CURL_INCLUDE_DIR}
        ${OpenCV_INCLUDE_DIRS}
        ${cpprestsdk_INCLUDE_DIRS}
)
target_include_directories(${SOMHUNTER_TARGET_TESTS} 
	PRIVATE
		${CURL_INCLUDE_DIR}
        ${OpenCV_INCLUDE_DIRS}
        ${cpprestsdk_INCLUDE_DIRS}
)

if (MSVC)
    set(ADDITIONAL OpenSSL::SSL OpenSSL::Crypto cpprestsdk::cpprest)
else ()
    set(ADDITIONAL OpenSSL::SSL OpenSSL::Crypto cpprest)
endif ()

target_link_libraries(${SOMHUNTER_TARGET} 
    PRIVATE
        ${OpenCV_LIBS}
        ${CURL_LIBRARIES}
		${TORCH_LIBRARIES}
        ${cpprestsdk_LIBRARIES}
        ${ADDITIONAL}
)

target_link_libraries(${SOMHUNTER_TARGET_TESTS} 
    PRIVATE
        ${OpenCV_LIBS}
        ${CURL_LIBRARIES}
		${TORCH_LIBRARIES}
        ${cpprestsdk_LIBRARIES}
        ${ADDITIONAL}
)

# ####################################
# Custom commands
# ####################################
if (MSVC) 
  file(GLOB TORCH_DLLS "${TORCH_INSTALL_PREFIX}/lib/*.dll") 
  add_custom_command(TARGET somhunter 
                     POST_BUILD 
                     COMMAND ${CMAKE_COMMAND} -E copy_if_different 
                     ${TORCH_DLLS} 
                     $<TARGET_FILE_DIR:somhunter>) 

endif (MSVC)

# ####################################
# Subdirectories
# ####################################
add_subdirectory(src)
add_subdirectory(third-party)
add_subdirectory(tests)
