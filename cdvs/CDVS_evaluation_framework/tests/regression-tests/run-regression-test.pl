#!/usr/bin/env perl

use warnings;
use File::Basename;
use File::Compare qw(compare);

require "utilities.pl";
sub moveOutput;
sub clean;
sub compareLogs;

require "conf.pl";

require "../../run/concurrency.pl";
sub launch;
sub waitAll;

sub printOnly {
	
	print "Printing...\n";
	
	# extract - do not use the -f option anymore because is not supported in fast mode
	foreach my $mode(0..6) {
		print "$extractBin images.txt $mode $parameters \n";
	}
	
	# match simple and mixed
	foreach my $mode(1..6) {
		print  "$matchBin one_pair_matching.txt one_pair_non_matching.txt $mode $parameters -j \n";
	}
	print "$matchBin one_pair_matching.txt one_pair_non_matching.txt 2 $parameters -j -r 4 \n";
	
	# mk_index and joinIndices	
	print "$mkIndexBin db_one_image.txt  db_one.bin 0 $parameters \n";
	print "$mkIndexBin db_two_images.txt db_two.bin 0 $parameters \n";
	print "$joinBin indices.txt db.hm.bin $parameters \n";
	
	# retrieve
	foreach my $mode(1..6) {
		print "$retrieveBin db.hm.bin retrieval.txt $mode $parameters \n";
	}	

	print "Done.\n";
}


sub computeTest {
	
	print "Running...\n";
	
	# extract - do not use the -f option anymore because is not supported in fast mode
	foreach my $mode(0..6) {
		launch "$extractBin images.txt $mode . . > ./extract.mode-$mode\.$rates[$mode].log 2>> ./error.log";
	}
	waitAll;
	
	# match simple and mixed
	foreach my $mode(1..6) {
		launch  "$matchBin one_pair_matching.txt one_pair_non_matching.txt $mode $parameters -j".
			    " > ./match.mode-$mode\.$rates[$mode]\.log 2>> ./error.log";
	}
	launch "$matchBin one_pair_matching.txt one_pair_non_matching.txt 2 $parameters -j -r 4 > ./match.hm.1K_4K.log 2>> ./error.log";
	waitAll;
	
	# mk_index and joinIndices	
	launch "$mkIndexBin db_one_image.txt  db_one.bin 0 $parameters > ./mkindex.hm.one.log 2>> ./error.log";
	launch "$mkIndexBin db_two_images.txt db_two.bin 0 $parameters > ./mkindex.hm.two.log 2>> ./error.log";
	waitAll;
	launch "$joinBin indices.txt db.hm.bin $parameters > ./joinIndices.hm.log 2>> ./error.log";
	waitAll;
	
	# retrieve
	foreach my $mode(1..6) {
		print  "$retrieveBin db.hm.bin retrieval.txt $mode $parameters > ./retrieval.hm.mode-$mode\.$rates[$mode].log 2>> ./error.log\n";
		system "$retrieveBin db.hm.bin retrieval.txt $mode $parameters > ./retrieval.hm.mode-$mode\.$rates[$mode].log 2>> ./error.log";
	}

	print "Done.\n";
}

sub avoidWarnings {
  print "$joinBin \n";
  print "$retrieveBin \n";
}

sub checkResults {
	my ($reference, $result) = ( $_[0],  $_[1]);

	my @resBin = <"$result/*.cdvs" "$result/*.sift" "$result/*.bin" "$result/*.local" "$result/*.global">;
	foreach my $file(@resBin) {
		my $reference_file = basename $file;
		if (compare($file, "$reference".$reference_file) != 0) {
			print "changed: $file\n";
		}
	}
	
	my @resLogs = <$result/*.log>;
	foreach my $file(@resLogs) {
		my $reference_file = basename $file;
		compareLogs("$reference".$reference_file, $file);
	}
	print "Finished result check.\n";
}


sub avgTime {
	my $log = $_[0];
	
	open my $log_file, '<', $log;
	my @log_lines = <$log_file>;
	my $text = join('', @log_lines);
	
	if ($text =~ m/Average(.*)time: ((\d|.)*)\[s(?:|ec|ecs)\]/) {
		return $2;
	}
	close $log_file;
}


sub speedup {
	my ($reference, $result) = ($_[0],  $_[1]);
	
	my $avg_ref_time = avgTime($reference);
	my $avg_res_time = avgTime($result);
	
	if ($avg_res_time > 0) {
		return $avg_ref_time / $avg_res_time;
	}
	else {
		return 0;
	}
}


sub checkTimings {
	my ($reference, $result, $filter) = ( $_[0],  $_[1], $_[2]);

	my @resLogs = <$result/*.log>;
	my $file_count = 0;
	my $speedup = 0;
	foreach my $file(@resLogs) {
		if ($file =~ m/$filter/) {
			my $reference_file = basename $file;
			my $curr_speedup = &speedup("$reference".$reference_file, $file);
			if ($curr_speedup != 0) {
				$speedup = $speedup + $curr_speedup;
				$file_count++;
			}
		}
	}
	my $avg_speedup = $speedup / $file_count;
	print "Average speedup: $avg_speedup\n";
}


my $buildReference = (defined($ARGV[0]) and ($ARGV[0] eq "--rebuild"));
my $check = (defined($ARGV[0]) and ($ARGV[0] eq "--check"));
my $onlyCheck = (defined($ARGV[0]) and ($ARGV[0] eq "--onlycheck"));
my $timings = (defined($ARGV[0]) and ($ARGV[0] eq "--timings"));
my $onlyPrint = (defined($ARGV[0]) and ($ARGV[0] eq "--print"));

my $timings_filter = '';
$timings_filter = $ARGV[1] unless !defined($ARGV[1]);

my $refDir = "reference/";
my $resDir = "result/";

mkdir $refDir or die "Can't create $refDir\n" unless(-d $refDir);
mkdir $resDir or die "Can't create $resDir\n" unless(-d $resDir);
clean(".");

if ($buildReference) {
	clean($refDir);
	computeTest;
	moveOutput(".", $refDir);
}
elsif ($check) {
	clean($resDir);
	computeTest;
	moveOutput(".", $resDir);
	checkResults($refDir, $resDir);
}
elsif ($onlyCheck) {
	checkResults($refDir, $resDir);
}
elsif ($onlyPrint) {
	printOnly;
}
elsif ($timings) {
	checkTimings($refDir, $resDir, $timings_filter);
}
else {
    print "usage:   run-regression-test [--rebuild] [--check] [--onlycheck] [--timings] [--print]\n";
    print "example: run-regression-test --check\n";
}
