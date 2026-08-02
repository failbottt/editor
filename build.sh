#!/bin/bash

for arg in "$@"; do declare $arg='1';done

if [[ ! -d "./build" ]]; then
    mkdir ./build
else
    rm -rf ./build
    mkdir ./build
fi

cc_sanitize=""
if [[ "${asan:-0}" == "1" ]]; then
    echo "[asan enabled]"
    cc_sanitize="-fsanitize=address"
fi

if [[ "${debug:-0}" == "1" ]]; then
    echo "[debug build]"
    cc_debug="-g"
fi

cc="gcc"
target="build/fed"
flags="${cc_sanitize} ${cc_debug} -std=c89"

${cc} ${flags} -o ${target} \
    src/main.c \
    src/editor.c \
    src/buffer.c
