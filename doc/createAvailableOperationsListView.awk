BEGIN {
	# Use two pipe symbols as input field separator
	FS="\\|\\|"
    print ":experimental:"
    print ""
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
