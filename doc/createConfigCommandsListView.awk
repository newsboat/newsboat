BEGIN {
	# Use two pipe symbols as input field separator
	FS="\\|\\|"
}

{
    print "[[" link_prefix $1 "]]_" $1 "_ (parameters: " $2 "; default value: _" $3 "_)::"
    print "         " $4 " (example: " $5 ")"
    print ""
}
