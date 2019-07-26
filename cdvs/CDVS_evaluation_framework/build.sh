#!/bin/sh
# build the CDVS Test Model
# with full optimizations and multithreading:
CDVSFLAGS="-mpopcnt -O2 -DNDEBUG -fopenmp -pipe"
# run configure with optimization flags
mkdir -p build
cd build
#
# add --with-bflog and --with-lowmem to the following line
# if you want to build the BFLOG and Low-Memory CDVS library variants
#
../configure CFLAGS="${CDVSFLAGS}" CXXFLAGS="${CDVSFLAGS}"
if [ $? -ne 0 ]; then exit; fi
# build all binaries
make
# install binaries in /usr/local/bin
# install CDVS library in /use/local/include, /usr/local/lib (need root privileges)
sudo make install
sudo ldconfig

