BEGIN {
	# Switch field separator from space to tabulator.
	FS="\t"	
}

{
	print "'''"
	print "[[" $1 "]]"
	print "****"
	print "*Syntax:* <<" $1 "," $1 ">>", $2, "+"
	sub("\"\"", "n/a", $3)
	print "*Default:*", $3, "+"
	print "*Example:*", $5, "+"
	print "****"
	print "\n" $4, "+"
	print "\n\n"
}
