#!/usr/bin/env perl

#
# utilities functions for regression tests
# Don't launch directly: use the run-*.pl scripts
#

use File::Copy qw(move);

sub moveOutput {
	my ($base, $dest) = ($_[0], $_[1]);
	my @results = <"$base/*.cdvs" "$base/*.sift" "$base/*.log" "$base/*.local" "$base/*.global">;
	foreach my $file(@results) {
		move($file, $dest) or die "Could not move $file to $dest\n";
	}
}


sub clean {
	my $dest = $_[0];
	my @results = <"$dest/*.cdvs" "$dest/*.sift" "$dest/*.log" "$base/*.local" "$base/*.global">;
	foreach my $file(@results) {
		unlink($file) or die "Could not delete $file\n";
	}
}


sub cleanLogLine {
	my $line = $_[0];
	chomp $line;
	$line =~ s/(\,|\:) \d(.*)\[s(?:|ec|ecs)\]/ ---/;
	
	return $line;
}


sub compareLogs {
	my ($reference, $result) = ($_[0],  $_[1]);
	
	open my $ref_file, '<', $reference;
	open my $res_file, '<', $result;
	
	my $n_line = 0;
	while( defined(my $ref_line = <$ref_file>) and
		   defined(my $res_line = <$res_file>) ) {
		
		if ($res_line =~ m/Average(.*)time: ((\d|.)*)\[s(?:|ec|ecs)\]/) {
			my $avg_res_time = $2;
		}
		
		$ref_line = &cleanLogLine($ref_line);
		$res_line = &cleanLogLine($res_line);
		$n_line++;
		
		if ($ref_line ne $res_line) {
			print "In: $result\tLine $n_line\n";
			print "Changed: $res_line\n";
			print "Was    : $ref_line\n\n";
		}
	}

	close $ref_file;
	close $res_file;
}

# module has to return a value:
1;
