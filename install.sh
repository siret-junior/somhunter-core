#!/bin/bash
ABSOLUTE_PATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
cd ${ABSOLUTE_PATH}
printf "\tInstalling 'somhunter-core'...\n"

BUILD_TYPE=Release

if [ -z ${1} ]; then 
    echo ".."
else
    BUILD_TYPE=${1}
fi

echo "Building with build type: ${BUILD_TYPE}"
exit 3
mkdir build
cmake -B ./build -DCMAKE_BUILD_TYPE=${BUILD_TYPE}
cmake --build ./build --config ${BUILD_TYPE}

printf "\tDone installing 'somhunter-core'...\n"
