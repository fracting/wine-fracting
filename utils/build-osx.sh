#!/bin/sh

./configure --without-x
make -j4 2>&1 | grep -A2 -B2 make
make test -k 2>&1 | grep -A2 -B2 make
