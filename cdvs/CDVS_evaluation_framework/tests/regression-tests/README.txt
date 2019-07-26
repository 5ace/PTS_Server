REGRESSION TESTS SUITE FOR CDVS TEST MODEL
------------------------------------------

For testing purposes are available the following scripts:

	run-regression-test.pl		Runs the regression test suite. See /tests/regression-tests/README.txt

	run-memory-check.pl		Runs a memory check using valgrind. See /tests/regression-tests/README.txt
	
NB: the scripts depend on conf.pl and on ../run/concurrency.pl

----------------
Regression tests
----------------

Use run-regression-tests.pl to run the tests. The test simulates a very small run of the experiments (3 images)
in order to check different version for binary compatibility.
The script allows to create a reference dump (using --rebuild) in the 'reference/' directory, against which
subsequent runs (using --check) will be checked. A run generate a similar dump in the 'result/' directory.
During check, the following files are compared at binary level: *.sift, *.cdvs, *.bin.
The logs are compared line by line, discarding information about timings (which always change).

Tests enabled
-------------
- extract: check all profiles on one image
- matchpp: check all profiles on a matching pair and a non matching pair
	   check mixed rates 1K_4K.5M
- makeIndex: create an index with one image, an index with two images and join them
- retrieve: check retrieval for all profiles on one image

Options
-------
 --rebuild:       create new reference results. To be used when a change breaks binary compatibility
                  in sift caches, descriptor files or database indices.
 --check:         check the current version of the Test Model against the reference. Checks if generated
                  binary files (.sift, .cdvs, .bin) are identical and if output logs are identical
                  (timings excluded)
 --only-check:	  check an already generated result against the reference
 --timings <set>: compare timings between the current result and the current references and returns a speedup
                  value. <set> allows restricting the calculation only to the logs which match the string
                  (e.g. --timings match)

------------
Memory check
------------

Use run-memory-check.pl to check for the memory leaks using valgrind. The results are saved in the 'memory/'
directory. The test is limited only to the 0, 1, 7, 13 profiles.

Options:

 --check-sift: 	  check also the sift extraction code (in VlFeat). Very slow.
