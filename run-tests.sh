#!/bin/bash
echo "Running external tests..."
ABSOLUTE_PATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
python ./tests/run-external-tests.py ./config.json ./api.json