#!/bin/sh

./configure --enable-win64
make -j4 2>&1 | grep -A2 -B2 make
cd loader
ln -s wine64 wine
cd ..
./wine wineboot
make test -k 2>&1 | grep -A2 -B2 make
