#!/bin/bash

gcc `pkg-config --static --libs glfw3` -o main main.c
./main
