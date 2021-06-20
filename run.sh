#!/bin/sh
echo "Running 'somhunter-core'..."
ABSOLUTE_PATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
cd $ABSOLUTE_PATH # CD to script dir
cd .. 
cd ./somhunter-core/build/ # Go to buil dir

DATETIME_STR=`date +"%d-%m-%YT%H-%M-%S"`
LOG_FILE="../logs/somhunter-${DATETIME_STR}.stdout"

if [ ! -d "../logs/" ]; then
  mkdir ../logs/
fi

echo "STDOUT & STDERR is also flushed to '${LOG_FILE}'"

# If windows
if [ -f "./somhunter.exe" ]
then
    ./somhunter.exe 2>&1 | tee ${LOG_FILE}
else
    ./somhunter 2>&1 | tee ${LOG_FILE}
fi
