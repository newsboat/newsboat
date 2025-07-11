BEGIN {
	# Use two pipe symbols as input field separator
	FS="\\|\\|"
}

{
    print "_" $1 "_ (default key: _" $2 "_)::"
    print "         " $3
    print ""
}
