#!/usr/bin/env perl

use warnings;

require "utilities.pl";
sub moveOutput;
sub clean;

require "conf.pl";
our($isWindows);

-e '/usr/bin/valgrind' or die "Valgrind not found.\n";
my $nullFile = $isWindows ? "NUL" : "/dev/null";

# check also the SIFT features generation code (slow)
my $checkSift = (defined($ARGV[0]) and ($ARGV[0] eq "--check-sift"));

my $valgrindCall = "valgrind --tool=memcheck --leak-check=yes ".
				   "--show-reachable=yes --num-callers=20 --track-fds=yes ";

my $memDir = "memory/";
mkdir $memDir or die "Can't create $memDir\n" unless(-d $memDir);

my $testLog = "results.log";
my $memoryLog = "memory.log";

clean(".");
clean($memDir);

print "Running... \n";

foreach my $mode(0..6) {
	if (!$checkSift) {
	      # generate .cdvs files without checks
	      print "$extractBin images.txt $mode $parameters (NO CHECK) \n";
	      system("$extractBin images.txt $mode $parameters > $nullFile");
	}
	else
	{
	      print "$extractBin images.txt $mode $parameters\n";
	      system "$valgrindCall $extractBin images.txt $mode $parameters >> $testLog 2>> $memoryLog";
	}
}
           

print "$matchBin one_pair_matching.txt one_pair_non_matching.txt 1 $parameters -j \n";
system "$valgrindCall $matchBin one_pair_matching.txt one_pair_non_matching.txt 1 $parameters -j".
	   " >> $testLog 2>> $memoryLog";
		   
print "$mkIndexBin db_one_image.txt  db_one.bin 0 $parameters \n";
system "$valgrindCall $mkIndexBin db_one_image.txt  db_one.bin 0 $parameters ".
	   ">> $testLog 2>> $memoryLog";
	   
print "$mkIndexBin db_two_images.txt db_two.bin 0 $parameters \n";
system "$valgrindCall $mkIndexBin db_two_images.txt db_two.bin 0 $parameters ".
	   ">> $testLog 2>> $memoryLog";
	   
print "$joinBin indices.txt db.hm.bin $parameters \n";
system "$valgrindCall $joinBin indices.txt db.hm.bin $parameters >> $testLog ".
	   "2>> $memoryLog";
	   
print "$retrieveBin db.hm.bin retrieval.txt 1 $parameters \n";
system("$valgrindCall $retrieveBin db.hm.bin retrieval.txt 1 $parameters >> $testLog 2>> $memoryLog");
	   
print "done.\n";

moveOutput(".", $memDir);
