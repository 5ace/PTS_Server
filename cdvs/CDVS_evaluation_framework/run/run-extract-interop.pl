#!/usr/bin/env perl
#
# extract descriptors from images at all rates defined by MPEG
#

require "conf-interop.pl";
require "concurrency.pl";
sub launch;
sub launchsp;
sub waitAll;

$error_log = "extract-error.log";

# check the logs dir
mkdir $logs or die "Can't create $logs \n" unless(-d $logs);

# run extraction
# example: tm-extract 1a_images.txt 1 parameters.txt

if (-r $logs) {
	foreach $x(@experiments) {
		foreach $mode(@hmodes) {
			launchsp "$extractBin $x\_images\.txt $mode  $datasetPath $annotationPath -p $parameters > ./$logs/$x\_extract\.$rates[$mode]\.log 2>> ./$logs/$error_log";
		}
	}
}

print "Extraction is running. Results will be stored in $logs \n";
