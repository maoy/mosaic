#!/bin/bash
pushd .
cd ~/mosaic/ && make
popd
~/mosaic/build/mosaicc -t alloy $1.mos > test &&  cat common.als test > $1.als

