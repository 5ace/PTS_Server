#!/usr/bin/env perl
#
# retrieve images at all rates defined by MPEG
# NB: this code needs 8 GB of free RAM to execute. Do not run it in parallel (each process would require 8 GB of RAM).
#

require "conf-interop.pl";

$error_log = "retrieval-error.log";

# check the logs dir
mkdir $logs or die "Can't create $logs\n" unless(-d $logs);

if (-r $logs) {
  foreach $mode(@hmodes) {
    foreach $x(@experiments) {
	  print  "$retrieveBin $database $x\_retrieval.txt $mode $datasetPath $annotationPath -p $parameters ".
             "> ./$logs/$x\_retrieval\.$rates[$mode].log 2>> ./$logs/$error_log\n";
	  system "$retrieveBin $database $x\_retrieval.txt $mode $datasetPath $annotationPath -p $parameters ".
             "> ./$logs/$x\_retrieval\.$rates[$mode].log 2>> ./$logs/$error_log";
    }
  }
}

