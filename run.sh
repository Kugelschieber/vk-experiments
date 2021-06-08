#!/bin/bash

mkdir -p build
gcc `pkg-config --static --libs glfw3` -c src/window.c -o build/window.o
gcc `pkg-config --static --libs glfw3` build/window.o -o main src/main.c
./main
