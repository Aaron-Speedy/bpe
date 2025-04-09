#!/bin/sh

set -xe

CFLAGS="-Wall -Wextra -ggdb -O3 -std=gnu11"

# cc bpe.c -o bpe $CFLAGS
# cc fast_bpe.c -o fast_bpe $CFLAGS
cc fast.c -o fast $CFLAGS
