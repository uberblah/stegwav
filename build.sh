#!/bin/sh

gcc -Wno-parentheses -Wno-incompatible-pointer-types ensteg.c steg.c -lsndfile -o "ensteg" 
gcc -Wno-parentheses -Wno-incompatible-pointer-types desteg.c steg.c -lsndfile -o "desteg" 
