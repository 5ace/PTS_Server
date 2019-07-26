#!/usr/bin/env perl

require "conf.pl";
require "concurrency.pl";
sub launch;
sub launchsp;
sub waitAll;


$hmode = 0;
$indices_h =  "hm_indices.txt";
				  
$error_log  = "mkindex-error.log";
$images_log ="mkindex-db-images.log";
$dist_log   ="mkindex-db-distracters";

# check the logs dir
mkdir "logs" or die "Can't create logs\n" unless(-d "logs");

# create HM indices list 

open FILE, ">$indices_h" or die "Unable to open $indices_h\n";
print FILE "database_images_index.hm.bin\n";
for $i(@distracters) {
    	print FILE "distracters_$i\_index.hm.bin\n";
}
close FILE;


if (-r "logs") {
    # -------------- logs ------------------
    print "running mk_index on database images\n";
    launch "$mkIndexBin database_images.txt database_images_index.hm.bin $hmode $datasetPath $annotationPath ".
           "> ./logs/$images_log 2>> ./logs/$error_log";

    print "running mk_index on distracters\n";
    foreach $x(@distracters) {
	launch "$mkIndexBin distracters_$x\.txt distracters_$x\_index.hm.bin $hmode $datasetPath $annotationPath ".
			  "> ./logs/$dist_log\.$x\.log 2>> ./logs/$error_log";
    }

    # wait all processes to finish
    waitAll;

    print "joining all indices\n";
    launchsp "$joinBin $indices_h $database $datasetPath $annotationPath";
}
