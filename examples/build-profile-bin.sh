#!/bin/sh

 #gcc -std=c99 -pg -g -ggdb -O0 -I ../src/ -L ../lib/.libs/ -L /usr/local/lib -I /usr/local/include/ -lcollections -lglut -lGL -lm  pathfinding.c -o pathfinding ../lib/.libs/libcsearch.a
 gcc -std=c99 -pg -O2 -I ../src/ -L ../lib/.libs/ -L /usr/local/lib -I /usr/local/include/ -lcollections -lglut -lGL -lm  pathfinding.c -o pathfinding ../lib/.libs/libcsearch.a
