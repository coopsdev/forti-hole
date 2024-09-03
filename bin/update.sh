#!/bin/bash

BUILD_DIR="../build"

# pull updates from Github
git pull

# remove the build directory
sudo rm -r $BUILD_DIR

# build the application
./build.sh
