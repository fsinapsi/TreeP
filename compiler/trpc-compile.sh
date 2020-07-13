#!/bin/sh
gcc -O3 -pipe -Wno-pointer-sign -D_REENTRANT -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -fPIC -I/usr/local/include -I/opt/local/include -o trpc trpc.c -L/usr/local/lib -L/opt/local/lib -ltrppix `pkg-config libpng --libs` -ljpeg -lgif -ltrp -lgmp `pkg-config bdw-gc --libs` -lz -lm
strip -s trpc
