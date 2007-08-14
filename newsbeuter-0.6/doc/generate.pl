#!/usr/bin/perl

if ($#ARGV != 0) {
	print STDOUT "usage: $0 <dsv-file>\n";
	exit(1);
}

open(FILE,$ARGV[0]) or die "couldn't open $ARGV[0]: $!\n";

while ($line = <FILE>) {
	chomp($line);
	@fields = split(/\|/, $line);
	print "'$fields[0]' (parameters: $fields[1]; default value: '$fields[2]')::\n         $fields[3] (example: $fields[4])\n\n";
}

close(FILE);
