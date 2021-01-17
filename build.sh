#!/bin/sh
set -eu
cc -Wall -Wextra -pedantic -std=c99 \
    -fsanitize=address -Og -g \
    -o compress \
    compress.c \
    $(pkg-config --libs --cflags liblz4 liblzma libzstd zlib)
