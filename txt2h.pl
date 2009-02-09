#!/usr/bin/perl

if (length(@ARGV) < 1) {
  print STDOUT "usage: $0 <stflfile> [<extension>]\n";
  exit(1);
}

$filename = $ARGV[0];
$extension = $ARGV[1];
$id = `basename $filename $extension`;
chomp($id);

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
