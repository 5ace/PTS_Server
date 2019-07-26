#! /bin/bash
#
# Create documentation from source files using Doxygen.
# (to install doxygen on Ubuntu: apt-get install doxygen doxygen-gui)
#
doxygen doxylinux.cfg > doxygen.log 2> error.log
echo "Done. Check results in doxygen.log and errors in error.log"
