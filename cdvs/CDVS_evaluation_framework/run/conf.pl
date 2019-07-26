#!/usr/bin/env perl
#
# Common configuration variables and sanity checks.
# Don't run this directly: use run-*.pl instead.
#

# experiment configuration
$flag = '';
$lflag = '-j';

@hmodes = (1..6);
@rates = qw(db 512 1024 2048 4096 8192 16384);

@localization = qw(1a 1b 1c);
@no_localization = qw(2 3 4 5);
@experiments = (@localization, @no_localization);
@distracters = qw(01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17 18 19 20
		  21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40
		  41 42 43 44 45 46 47 48 49 50);
$database = "database_index.cdvs";
$datasetPath = "./Dataset-20120210";
$annotationPath = "./Dataset-20120210/annotations";
$logs = "logs";

# set and check binaries
$isWindows = ("$^O" eq "MSWin32");  #OS check

# put your actual bin directory here:
$binDir = $isWindows ? '..\\bin\\' : "/usr/local/bin/";
$binExt = $isWindows ? ".exe" : "";

$extractBin  = $binDir.$flag."extract"    .$binExt;     
$matchBin    = $binDir.$flag."match"      .$binExt;
$mkIndexBin  = $binDir.$flag."makeIndex"  .$binExt;
$joinBin     = $binDir.$flag."joinIndices".$binExt;
$retrieveBin = $binDir.$flag."retrieve"   .$binExt;

-x $extractBin  or die "$extractBin not found.\n";
-x $matchBin    or die "$matchBin not found.\n";
-x $mkIndexBin  or die "$mkIndexBin not found.\n";
-x $matchBin    or die "$joinBin not found.\n";
-x $retrieveBin or die "$retrieveBin not found.\n";
