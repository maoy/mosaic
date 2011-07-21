#!/bin/bash

make -C build -j3 && make && make -C release -j3

