::
:: runs all scripts
::

:: pairwise

echo == Extract descriptors ==
run-extract-interop.pl

echo == Pairwise matching ==
run-match-interop.pl


:: retrieval

echo == Retrieval ==
run-retrieval-interop.pl

pause
exit
