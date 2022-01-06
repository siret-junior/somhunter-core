#!/bin/bash

ABSOLUTE_PATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
cd ${ABSOLUTE_PATH}

printf "\tInstalling 'somhunter-core'...\n"

BUILD_TYPE=RelWithDebInfo

if [ -z ${1} ]; then 
    echo ".."
else
    BUILD_TYPE=${1}
fi

echo "Building with build type: ${BUILD_TYPE}"
if [ -d "$DIRECTORY" ]; then
    rm -rf build
fi

mkdir build
cmake -B ./build -DCMAKE_BUILD_TYPE=${BUILD_TYPE}
cmake --build ./build --config ${BUILD_TYPE} -j

printf "\tDone installing 'somhunter-core'...\n"
