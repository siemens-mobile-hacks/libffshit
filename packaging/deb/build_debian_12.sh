#!/bin/env bash

if [ -z $1 ]
then
    echo "Specify build directory"
    exit -1
fi;

if [ -z $2 ]
then
    echo "Specify build type (dev, prod)"
    exit -2
fi;

DEV_BUILD="FALSE"

if [ ! "$2" = "dev" ] && [ ! "$2" = "prod" ]
then
    echo "Build type must be 'dev' or 'prod'"
    exit -3
fi

if [ "$2" = "dev" ]
then
    DEV_BUILD="TRUE"
fi

BUILD_DIR=$1

cmake -DDEV_BUILD=$DEV_BUILD -DCMAKE_BUILD_TYPE=Release -DDIST_NAME="debian-12" -DDIST_DEPS="libfmt9,libfmt-dev" -DDIST_ARCH="amd64" -B $BUILD_DIR
cmake --build $BUILD_DIR --config Release
