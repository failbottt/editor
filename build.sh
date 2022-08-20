#!/bin/sh

TARGET="editor"
LIBS="-lX11 -lm -lGL -lfreetype"
FLAGS="-std=c99 -g -Wall -Wextra"
INCLUDE=-I/usr/include/freetype2

gcc $FLAGS -o $TARGET main.c $LIBS $INCLUDE
