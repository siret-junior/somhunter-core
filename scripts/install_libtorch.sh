#!/bin/bash

LIB_URL=$1
INSTALL_DIR=$2

printf ">>> Installing libTorch >>>\n"

if ! [ -d ${INSTALL_DIR} ]
then
    mkdir -p ${INSTALL_DIR}
fi

if ! [ -f "${INSTALL_DIR}/libtorch.zip" ]
then
    echo "Downloading the library from '${LIB_URL}'..."
    if ! wget -O "${INSTALL_DIR}/libtorch.zip" "${LIB_URL}"; then
        echo "Downloading failed!"
        exit 1
    fi
else
    echo "File already downloaded."
fi


if ! [ -f "${INSTALL_DIR}/libtorch.zip" ]
then
    echo "Unzipping the library..."
    if ! unzip -o "${INSTALL_DIR}/libtorch.zip" -d "${INSTALL_DIR}"; then
        echo "Unzipping failed!"
        exit 1
    fi
else
    echo "Library already installed."
fi

echo "libTorch successfully installed."

printf "<<< Installing libTorch <<<\n"

exit 0