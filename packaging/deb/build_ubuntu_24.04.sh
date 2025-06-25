#!/bin/env bash

if [ -z $1 ]
then
    echo "Specify build directory"
    exit -1
fi;

BUILD_DIR=$1

cmake -DCMAKE_BUILD_TYPE=Release -DDIST_NAME="ubuntu-24.04" -DDIST_DEPS="libfmt9,libfmt-dev" -DDIST_ARCH="amd64" -B $BUILD_DIR
cmake --build $BUILD_DIR --config Release
