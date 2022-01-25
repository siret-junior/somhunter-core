#!/bin/bash

# Get the absolute path to the directory this script lies in
ABSOLUTE_PATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
cd ${ABSOLUTE_PATH}


BUILD_TYPE=RelWithDebInfo
if [ -z ${1} ]; then 
    printf "Usage: install.sh <BUILD_TYPE> \n\t BUILD_TYPE \in { Release, RelWithDebInfo, Debug }\n"
    exit 1
else
    BUILD_TYPE=${1}
    echo "Building with build type: ${BUILD_TYPE}"
fi

echo "Installing 'somhunter-core'..."

# Make sure that build directory is gone
# if [ -d "build" ]; then
#     rm -rf build
#     rm -rf ./third-party/libtorch*
# fi

mkdir build

# Configure
cmake -B ./build -DCMAKE_BUILD_TYPE=${BUILD_TYPE}
# Build
cmake --build ./build --config ${BUILD_TYPE} -j4

echo "Done installing 'somhunter-core'..."
