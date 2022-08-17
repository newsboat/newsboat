function print_key(key) {
    if (key == "n/a") {
        return key
    }

    # TODO: Escape closing brackets `]` -> `\]`

    if (substr(key, 1, 1) == "^") {
        return "kbd:[Ctrl+" substr(key, 2) "]"
    }

    if (length(key) == 1) {
        if (tolower(key) == key) {
            return "kbd:[" toupper(key) "]"
        } else {
            return "kbd:[Shift+" key "]"
        }
    }

    return "kbd:[" key "]"
}

BEGIN {
	# Switch field separator from space to tabulator.
	FS="\t"
    print ":experimental:"
    print ""
}

{
	print "'''"
	print "[[" $1 "]]"
	print "****"
	print "*Operation:* <<" $1 "," $1 ">> +"
	print "*Default key:*", print_key($2), "+"
	print "****"
	print "\n" $3, "+"
	print "\n\n"
}
