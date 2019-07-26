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
my $checkExtract = (defined($ARGV[0]) and ($ARGV[0] eq "--extract"));
my $checkMatch = (defined($ARGV[0]) and ($ARGV[0] eq "--match"));
my $checkRetrieve = (defined($ARGV[0]) and ($ARGV[0] eq "--retrieve"));

if (not defined($ARGV[0]))
{
  print "usage: run-speed-check [--extract | --match | --retrieve]\n";
  print "example: run-speed-check --match\n";
  exit 1;
}

my $valgrindCall = "valgrind --tool=callgrind ";

my $memDir = "speed/";
mkdir $memDir or die "Can't create $memDir\n" unless(-d $memDir);

my $testLog = "results.log";
my $memoryLog = "error.log";

clean(".");
clean($memDir);

print "Running... \n";

foreach my $mode(0, 6) {
	if (!$checkExtract) {
	      # generate .sift files without checks
	      print  "$extractBin images.txt $mode $parameters \n";
	      system "$extractBin images.txt $mode $parameters ";
	}
	else
	{
	      print  "$valgrindCall $extractBin images.txt $mode $parameters\n";
	      system "$valgrindCall $extractBin images.txt $mode $parameters ";
	}
}

if ($checkMatch) {
    print  "$valgrindCall $matchBin one_pair_matching.txt one_pair_non_matching.txt 1 $parameters -l \n";
    system "$valgrindCall $matchBin one_pair_matching.txt one_pair_non_matching.txt 1 $parameters -l";
}

if ($checkRetrieve) {   
    print  "$mkIndexBin db_one_image.txt  db_one.bin 0 $parameters \n";
    system "$mkIndexBin db_one_image.txt  db_one.bin 0 $parameters ";
    print  "$mkIndexBin db_two_images.txt db_two.bin 0 $parameters \n";
    system "$mkIndexBin db_two_images.txt db_two.bin 0 $parameters ";
    print  "$joinBin indices.txt db.hm.bin $parameters \n";
    system "$joinBin indices.txt db.hm.bin $parameters ";
    print  "$valgrindCall $retrieveBin db.hm.bin retrieval.txt 1 $parameters \n";
    system "$valgrindCall $retrieveBin db.hm.bin retrieval.txt 1 $parameters ";
}
	   
print "done.\n";

moveOutput(".", $memDir);
