#! /bin/bash
#
# Create documentation from source files using Doxygen.
# (to install doxygen on Ubuntu: apt-get install doxygen doxygen-gui)
#
doxygen -s -u cdvslib.cfg
