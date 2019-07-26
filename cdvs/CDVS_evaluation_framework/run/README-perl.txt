========================
CORE EXPERIMENTS SCRIPTS
========================

------------
PERL SCRIPTS
------------
Perl scripts are cross-platform. The parallel execution of multiple processes is automatically supported
in the platforms which support the forking mechanism.

INSTALLATION:
update the paths in parameters.txt; under windows, also change in conf.pl the variable '$binDir' to match
the actual binary installation path for the CDVS framework.

-------
Scripts
-------
create-sift-cache.pl    create/renew the SIFT cache from all JPEG images in the CDVS Dataset.

run-extract.pl          extract descriptors for all profiles. The SIFT cache is enabled.

run-extract-db.pl       extract descriptors for database profile and for all database images and distracters.
                        SIFT cache enabled for database images, disabled for distracters.

run-match.pl            Pairwise matching between images, for all profiles and mixed (1K_4K, 2K_4K).

run-mkindex.pl          Create database indices for database images and distracters, and join them.

run-retrieval.pl        Test retrieval results for all profiles.


run-all.sh              Scripts for linux and windows respectively, run all the previous scripts in sequence
run-all.bat


run-localization.pl     Runs only the localization part of the pairwise matching experiment to test the geometry
                        estimation stage.
                        Option:
                          --only-check      perform only the matching step (descriptors should already be extracted)

---------------
Utility scripts
---------------
conf.pl                 Contains variables shared by two or more scripts, check paths.

concurrency.pl          Concurrency utilies: contains functions to launch parallel processes and wait for their
                        termination. 'launch' is a drop-in replacement for concurrent program invocation in bash and
                        'waitAll' is a drop-in replacement for 'wait'.
