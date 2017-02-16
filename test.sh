#!/bin/sh

set -x

clean
./build.sh
./ensteg input.wav before output.wav
./desteg output.wav after
cat before | wc -c
cat before && echo ""
cat after && echo ""
