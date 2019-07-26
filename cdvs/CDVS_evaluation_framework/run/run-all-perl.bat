::
:: runs all scripts
::

:: pairwise

echo == Extract descriptors ==
run-extract.pl

echo == Pairwise matching ==
run-match.pl


:: retrieval

echo == Extract database descriptors ==
run-extract-db.pl

echo == Make index ==
run-mkindex.pl

echo == Retrieval ==
run-retrieval.pl

pause
exit
