#!/usr/bin/env perl

use warnings;
use strict;

if (scalar(@ARGV) < 1) {
	print STDERR "usage: $0 <dsv-file>\n";
	exit(1);
}

open(my $fh, '<', $ARGV[0]) or die "couldn't open $ARGV[0]: $!\n";

while (my $line = <$fh>) {
	chomp($line);
	my @fields = split(/:/, $line, 3);
	print "'$fields[0]' (default key: '$fields[1]')::\n         $fields[2]\n\n";
}

close($fh);
