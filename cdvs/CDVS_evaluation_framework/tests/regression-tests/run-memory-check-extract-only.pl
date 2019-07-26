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

foreach my $mode(4) {
  print "$extractBin images.txt $mode $parameters\n";
  system "$valgrindCall $extractBin images.txt $mode $parameters >> $testLog 2>> $memoryLog";
}
           	   
print "done.\n";

moveOutput(".", $memDir);
