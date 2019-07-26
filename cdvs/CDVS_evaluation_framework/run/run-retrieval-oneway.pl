#!/usr/bin/env perl
#
# retrieve images at all rates defined by MPEG
# NB: this code needs 8 GB of free RAM to execute. Do not run it in parallel (each process would require 8 GB of RAM).
#

require "conf.pl";
require "concurrency.pl";
sub launch;
sub launchsp;
sub waitAll;

$traces = "traces.oneway";
$logs   = "logs.oneway";

$error_log = "retrieval-error.log";

# check the logs dir
mkdir $logs or die "Can't create $logs\n" unless(-d $logs);

if (-r $logs) {
  foreach $mode(@hmodes) {
    foreach $x(@experiments) {
	  launchsp "$retrieveBin $database $x\_retrieval.txt $mode $datasetPath $annotationPath -o ".
             "> ./$logs/$x\_retrieval\.$rates[$mode].log 2>> ./$logs/$error_log";
    }
  }
}

