#!/bin/sh
echo "Running 'somhunter-core'..."
ABSOLUTE_PATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
cd $ABSOLUTE_PATH # CD to script dir
cd .. 
cd ./somhunter-core/build/ # Go to buil dir

# If windows
if [ -f "./somhunter.exe" ]
then
    ./somhunter.exe
else
#    ./somhunter
	supervise .
fi
