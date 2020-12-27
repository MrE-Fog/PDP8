#!/bin/bash

mkdir -p build

#gcc -Wall -Wextra -o pdp8 ../src/pdp8.c
g++ -o ./build/pdp8 ./src/main.cpp ./src/pdp8.c -lX11 -lGL -lpthread -lpng -lstdc++fs -std=c++17

