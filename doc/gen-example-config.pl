#!/usr/bin/perl

print "####################################\n";
print "# newsbeuter example configuration #\n";
print "####################################\n\n";

while ($line = <STDIN>) {
	($option,$syntax,$defaultparam,$desc,$example) = split(/\|/, $line);
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
