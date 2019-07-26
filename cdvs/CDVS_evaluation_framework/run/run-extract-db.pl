#!/usr/bin/env perl
#
# extract descriptors from database images
#

require "conf.pl";
require "concurrency.pl";
sub launch;
sub launchsp;
sub waitAll;

$hmode = 0;

$all_images = "database_images.txt";
$error_log = "extract-error.log";

# check logs dir
mkdir "logs" or die "Can't create logs\n" unless(-d "logs");

# run the extraction on db and distracters images
if (-r "logs") {
    print "running the extraction \n";
    launchsp "$extractBin $all_images $hmode $datasetPath $annotationPath > ./logs/extract-db-images.log 2>> ./logs/$error_log";
    foreach $x(@distracters) {
        launchsp "$extractBin distracters_$x\.txt $hmode $datasetPath $annotationPath > ./logs/extract-db-distracters_$x\.log 2>> ./logs/$error_log";
    }
    print "extraction is running. Results will be stored in ./logs \n";
}
