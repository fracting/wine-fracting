#!/bin/sh

./configure
make -j4 2>&1 | grep -A2 -B2 make
cd loader
ln -s wine64 wine
cd ..
make test -k 2>&1 | grep -A2 -B2 make
