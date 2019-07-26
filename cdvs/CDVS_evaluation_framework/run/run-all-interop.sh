#! /bin/bash
#
# run interoperability test against the standard Test Model
# this must be run AFTER a complete run of the Test Model (run-all-perl.sh)
#

#
# pairwise
#

echo "== Extract descriptors. =="
./run-extract-interop.pl

echo "== Pairwise matching =="
./run-match-interop.pl


#
# retrieval (using the Test Model database)
#
echo "== Retrieval =="
./run-retrieval-interop.pl
