#!/usr/bin/perl -w

use strict;
use File::Basename;

if (scalar(@ARGV) < 1) {
  print STDOUT "usage: $0 <stflfile> [<extension>]\n";
  exit(1);
}

my ($filename, $extension) = @ARGV;
my $id = basename($filename, $extension || "");
chomp($id);

open(my $fh, '<', $filename) or die "couldn't open $filename: $!\n";

print "#ifndef ${id}__h_included\n";
print "#define ${id}__h_included\n";
print "\n";
print "static char ${id}_str[] = \"\" ";

while (my $line = <$fh>) {
  $line =~ s/"/\\"/g;
  chomp($line);
  print "\"$line\\n\"\n";
}

close($fh);


print ";\n";
print "#endif\n";
