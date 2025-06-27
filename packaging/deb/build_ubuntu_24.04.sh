#!/bin/env bash

if [ -z $1 ]
then
    echo "Specify build directory"
    exit -1
fi;

if [ -z $2 ]
then
    echo "Specify build type (dev, prod)"
    exit -1
fi;

BUILD_DEV="FALSE"

if [ ! "$2" = "dev" ] && [ ! "$2" = "prod" ]
then
    echo "Build type must be 'dev' or 'prod'"
    exit -3
fi

if [ "$2" = "dev" ]
then
    BUILD_DEV="TRUE"
fi

BUILD_DIR=$1

cmake -DBUILD_DEV=$BUILD_DEV -DCMAKE_BUILD_TYPE=Release -DBUILD_DEB_PACKAGE=TRUE -DDEB_DIST_NAME="ubuntu-24.04" -DDEB_DIST_DEPS="libfmt9,libfmt-dev" -DDEB_DIST_ARCH="amd64" -B $BUILD_DIR
cmake --build $BUILD_DIR --config Release
