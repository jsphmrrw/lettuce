#!/bin/bash
if [ ! -d "build" ]; then
  mkdir build
fi
pushd build
gcc -g ../source/lettuce_main.c -o lettuce
popd