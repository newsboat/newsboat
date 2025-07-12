function to80Columns(line,    prefix, limit, words, n, i) {
	prefix = "#"
	limit = 80

	n = split(line, words, / +/)

	line = prefix
	for (i = 1; i <= n; i++) {
		if (length(line) + 1 + length(words[i]) >= limit) {
			print line
			line = prefix
		}
		line = line " " words[i]
	}
	print line
}

BEGIN {
	FS="\\|\\|"

	print "#"
	print "# Newsboat's example config"
	print "#"
	print ""
}

{
	print "####  " $1
	print "#"
	to80Columns($4)
	print "#"
	print "# Syntax: " $2
	print "#"
	print "# Default value: " $3
	print "#"
	print "# " $5
	print ""
}
