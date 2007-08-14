#!/usr/bin/perl

if ($#ARGV != 0) {
  print STDOUT "usage: $0 <stflfile>\n";
  exit(1);
}

$id = `basename $ARGV[0] .stfl`;
chomp($id);
chomp($ucid);

open(FILE,$ARGV[0]) or die "couldn't open $ARGV[0]: $!\n";

print "#ifndef ${id}__h_included\n";
print "#define ${id}__h_included\n";
print "\n";
print "static char ${id}_str[] = \"\" ";

while ($line = <FILE>) {
  $line =~ s/"/\\"/g;
  chomp($line);
  print "\"$line\\n\"\n";
}

close(FILE);


print ";\n";
print "#endif\n";
