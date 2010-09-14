#!/usr/bin/perl -w

use strict;

if (scalar(@ARGV) != 0) {
	print STDOUT "usage: $0 <dsv-file>\n";
	exit(1);
}

open(my $fh, '<', $ARGV[0]) or die "couldn't open $ARGV[0]: $!\n";

while (my $line = <$fh>) {
	chomp($line);
	my @fields = split(/\|/, $line);
	print "'$fields[0]' (parameters: $fields[1]; default value: '$fields[2]')::\n         $fields[3] (example: $fields[4])\n\n";
}

close($fh);
