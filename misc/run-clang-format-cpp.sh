#!/bin/bash

ABSOLUTE_PATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
cd $ABSOLUTE_PATH # CD to script dir

echo "Running the script from ${ABSOLUTE_PATH}..." 

echo "Formatting `src` directory recursively..."
find ../src/ -regex '.*\.\(cpp\|hpp\|cc\|cxx\)' -exec clang-format -verbose -style=file -i {} \;

echo "Formatting `tests` directory recursively..."
find ../tests/ -regex '.*\.\(cpp\|hpp\|cc\|cxx\)' -exec clang-format -verbose -style=file -i {} \;

echo "Done."

echo "Press ENTER to continue..."
read -p "$*"