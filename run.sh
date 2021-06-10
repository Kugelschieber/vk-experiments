#!/bin/bash

echo "Building..."
rm -rf ./build
mkdir -p ./build
gcc -c src/util.c -o build/util.o || exit
gcc -c src/log.c -o build/log.o || exit
gcc `pkg-config --static --libs glfw3` -c src/window.c -o build/window.o || exit
gcc `pkg-config --static --libs glfw3` \
    `pkg-config --static --libs vulkan` \
    build/log.o \
    build/window.o \
    build/util.o \
    -o main src/main.c || exit
echo "Done. Starting app..."
./main
