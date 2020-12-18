#!/bin/bash

mkdir -p build
cd build

gcc -Wall -Wextra -o pdp8 ../src/pdp8.c

cd ..
