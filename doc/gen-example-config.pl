#!/usr/bin/env perl

use warnings;
use strict;

print "####################################\n";
print "# newsbeuter example configuration #\n";
print "####################################\n\n";

while (my $line = <STDIN>) {
	my ($option,$syntax,$defaultparam,$desc,$example) = split(/\|/, $line);
	if ($defaultparam ne "n/a") {
		print "## configuration option: " . $option . "\n";
		if ($desc =~ /limitation in AsciiDoc/) {
			$desc =~ s/ \([^)]*\)\.$/./;
			$defaultparam =~ s/;/|/g;
		}
		print "## description: " . $desc . "\n";
		print "## parameter syntax: " . $syntax . "\n";
		print "# " . $option . " " . $defaultparam . "\n\n";
	}
}

print "# EOF\n"
