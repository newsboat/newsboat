BEGIN {
	# Switch field separator from space to tabulator.
	FS="\t"
}

{
	print "'''"
	print "[[" $1 "]]"
	print "****"
	print "*Operation:* <<" $1 "," $1 ">> +"
	print "*Default key:*", $2, "+"
	print "****"
	print "\n" $3, "+"
	print "\n\n"
}
