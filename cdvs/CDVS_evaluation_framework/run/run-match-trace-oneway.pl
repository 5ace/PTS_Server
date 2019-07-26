#!/usr/bin/env perl

require "conf.pl";
require "concurrency.pl";
sub launch;
sub waitAll;

$traces = "traces.oneway";
$logs   = "logs.oneway";
$error_log = "match-error.log";

# check the logs dir
mkdir $logs or die "Can't create $logs\n" unless(-d $logs);

# check the traces dir
mkdir $traces or die "Can't create $traces\n" unless(-d $traces);

if (-r $logs) {
  # run normal matches
  foreach $mode(@hmodes) {
	foreach $x(@localization) {
	  $file1 = "$x\_matching_pairs.txt";
	  $file2 = "$x\_non_matching_pairs.txt";
	  $trace = "$x\_trace.$rates[$mode]\.txt";
	  launch "$matchBin $file1 $file2 $mode $datasetPath $annotationPath $lflag -o -p parameters.txt -t ./$traces/$trace > ./$logs/$x\_match\.$rates[$mode]\.log 2>> ./$logs/$error_log";
    }
    foreach $x(@no_localization) {
	  $file1 = "$x\_matching_pairs.txt";
	  $file2 = "$x\_non_matching_pairs.txt";
	  $trace = "$x\_trace.$rates[$mode]\.txt";
	  launch "$matchBin $file1 $file2 $mode $datasetPath $annotationPath -o -p parameters.txt -t ./$traces/$trace > ./$logs/$x\_match\.$rates[$mode]\.log 2>> ./$logs/$error_log";
    }
  }
	
  #run mixed cases
  foreach $x(@localization) {
	$file1 = "$x\_matching_pairs.txt";
	$file2 = "$x\_non_matching_pairs.txt";
	$trace = "$x\_trace.1K_4K.txt";
	launch "$matchBin $file1 $file2 2 $datasetPath $annotationPath $lflag -o -p parameters.txt -r 4 -t ./$traces/$trace > ./$logs/$x\_match.1K_4K.log 2>> ./$logs/$error_log";
	$trace = "$x\_trace.2K_4K.txt";
	launch "$matchBin $file1 $file2 3 $datasetPath $annotationPath $lflag -o -p parameters.txt -r 4 -t ./$traces/$trace > ./$logs/$x\_match.2K_4K.log 2>> ./$logs/$error_log";
  }
  
  foreach $x(@no_localization) {
	$file1 = "$x\_matching_pairs.txt";
	$file2 = "$x\_non_matching_pairs.txt";
	$trace = "$x\_trace.1K_4K.txt";
	launch "$matchBin $file1 $file2 2 $datasetPath $annotationPath -o -p parameters.txt -r 4 -t ./$traces/$trace > ./$logs/$x\_match.1K_4K.log 2>> ./$logs/$error_log";
	$trace = "$x\_trace.2K_4K.txt";
	launch "$matchBin $file1 $file2 3 $datasetPath $annotationPath -o -p parameters.txt -r 4 -t ./$traces/$trace > ./$logs/$x\_match.2K_4K.log 2>> ./$logs/$error_log";
  }
}

print "Matching experiments running. Results will be stored in ./$logs \n";

waitAll;
