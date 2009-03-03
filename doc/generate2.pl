#!/usr/bin/perl

if ($#ARGV != 0) {
	print STDOUT "usage: $0 <dsv-file>\n";
	exit(1);
}

open(FILE,$ARGV[0]) or die "couldn't open $ARGV[0]: $!\n";

while ($line = <FILE>) {
	chomp($line);
	@fields = split(/:/, $line, 3);
	print "'$fields[0]' (default key: '$fields[1]')::\n         $fields[2]\n\n";
}

close(FILE);
