#!/bin/bash

MOSAIC=$HOME/mosaic
MOSAICC=$MOSAIC/build/mosaicc
TP=$HOME/third_party
FULLNAME=$1
MODULENAME="${FULLNAME%%.*}"
BUILD_DIR=build

mkdir -p $BUILD_DIR
echo "Compiling $FULLNAME"
cpp $FULLNAME > $BUILD_DIR/$MODULENAME.mpp || exit 1
$MOSAICC build/$MODULENAME.mpp > $BUILD_DIR/$MODULENAME.hpp || exit 2
cp -f $MOSAIC/src/lib/main.cpp $BUILD_DIR/
cp -f $BUILD_DIR/$MODULENAME.hpp $BUILD_DIR/gen.hpp
g++ -Wall -g -pg -O2 -pipe -static -I $MOSAIC/src/lib -I$TP/include $BUILD_DIR/main.cpp -o $BUILD_DIR/$MODULENAME.exe \
 -L$MOSAIC/build/lib -L$TP/lib -lmoslib -lboost_system -lgc -lpthread

#g++ -Wall -O3 -pipe -static -I $MOSAIC/src/lib -I$TP/include $BUILD_DIR/main.cpp -o $BUILD_DIR/$MODULENAME.release.exe \
# -L$MOSAIC/release/lib -L$TP/lib -lmoslib -lboost_system -lgc -lpthread
#strip build/$MODULENAME.release.exe

