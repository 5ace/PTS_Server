#! /bin/bash
#
# standard Test Model
#


#
# pairwise
#

echo "== Extract descriptors. =="
./run-extract.pl

echo "== Pairwise matching =="
./run-match.pl


#
# retrieval
#

echo "== Extract database and distracter descriptors =="
./run-extract-db.pl

echo "== Make index =="
./run-mkindex.pl

echo "== Retrieval =="
./run-retrieval.pl
