#!/bin/bash
echo "Running 'somhunter-core'..."
ABSOLUTE_PATH="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/"
cd $ABSOLUTE_PATH # CD to script dir
cd .. 
cd ./somhunter-core/build/ # Go to buil dir

./Debug/somhunter*
./Release/somhunter*