#!/bin/sh
# build the CDVS Test Model
# with debugging options:
DEBUGFLAGS="-g -O0 -pipe"
# run configure with debugging flags and prepending "db-" to all binaries (e.g. db-extract, db-match, etc.)
mkdir -p build_debug
cd build_debug
../configure --disable-shared --prefix=$HOME --program-prefix="db-" CFLAGS="${DEBUGFLAGS}" CXXFLAGS="${DEBUGFLAGS}"
# build all binaries
make
# install all binaries in $HOME/bin (no need of admin priviledges)
make install
