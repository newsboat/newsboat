#!/usr/bin/env ruby
# script to test misc. functions of itemlist.
require 'tuitest'

Kernel.system("rm -f cache")

Tuitest.init
verifier = Tuitest::Verifier.new("test-itemlist.rb.log")


Tuitest.run("../newsbeuter -c cache -C /dev/null -u urls-tuitest1")


Tuitest.wait_until_idle
Tuitest.keypress("r"[0])
Tuitest.keypress(10)
Tuitest.keypress(5)
Tuitest.keypress("a"[0])
Tuitest.keypress("b"[0])
Tuitest.keypress("c"[0])

Tuitest.wait_until_idle
# begin auto-generated verification #1 
verifier.expect(24, 0, "Flags: abc")
# end auto-generated verification #1 

Tuitest.keypress(10)

Tuitest.wait_until_idle
# begin auto-generated verification #2 
verifier.expect(1, 6, "!")
verifier.expect(24, 5, " updated.")
# end auto-generated verification #2 

Tuitest.keypress(10)
Tuitest.keypress(5)
Tuitest.keypress("q"[0])
Tuitest.keypress(259)
Tuitest.keypress(258)
Tuitest.keypress("q"[0])
Tuitest.keypress("q"[0])
Tuitest.keypress("q"[0])
Tuitest.keypress(259)
Tuitest.keypress(259)
Tuitest.keypress(258)
Tuitest.keypress(258)
Tuitest.keypress(258)
Tuitest.keypress(259)
Tuitest.keypress(259)
Tuitest.keypress(259)
Tuitest.keypress(261)
Tuitest.keypress(261)
Tuitest.keypress(261)
Tuitest.keypress(261)
Tuitest.keypress(261)
Tuitest.keypress(8)
Tuitest.keypress(260)
Tuitest.keypress(260)
Tuitest.keypress(260)
Tuitest.keypress(260)
Tuitest.keypress(260)
Tuitest.keypress(260)
Tuitest.keypress(260)
Tuitest.keypress(330)
Tuitest.keypress(330)
Tuitest.keypress(330)
Tuitest.keypress(330)
Tuitest.keypress(330)
Tuitest.keypress(330)
Tuitest.keypress(330)
Tuitest.keypress(330)
Tuitest.keypress(10)

Tuitest.wait_until_idle
# begin auto-generated verification #3 
verifier.expect(6, 0, "          ")
verifier.expect(7, 0, "This is some example content.")
verifier.expect(8, 0, "                             ")
verifier.expect(9, 0, "Here's a line break.")
verifier.expect(10, 0, "Second line. And now we try an unordered list.")
verifier.expect(11, 1, "                                              ")
verifier.expect(12, 2, "* first entry")
verifier.expect(13, 4, "second entry")
verifier.expect(14, 4, "third entry ")
verifier.expect(15, 2, "             ")
verifier.expect(16, 0, "~")
verifier.expect(24, 0, "Flags updated.")
# end auto-generated verification #3 

Tuitest.keypress("q"[0])
Tuitest.keypress("s"[0])
Tuitest.keypress(259)
Tuitest.keypress("q"[0])
Tuitest.keypress("?"[0])
Tuitest.keypress("q"[0])
Tuitest.keypress("r"[0])
Tuitest.keypress("n"[0])
Tuitest.keypress("n"[0])
Tuitest.keypress("p"[0])
Tuitest.keypress(14)

Tuitest.wait_until_idle
# begin auto-generated verification #4 
# end auto-generated verification #4 

Tuitest.keypress("l"[0])

Tuitest.wait_until_idle
# begin auto-generated verification #5 
verifier.expect(1, 3, "2 N  Aug 29   RSS 2.0 Item 2")
verifier.expect(2, 3, "3 N  Aug 28   RSS 2.0 Item 3")
verifier.expect(3, 3, "                            ")
# end auto-generated verification #5 

Tuitest.keypress("l"[0])

Tuitest.wait_until_idle
# begin auto-generated verification #6 
verifier.expect(1, 3, "1    Aug 30   RSS 2.0 Item 1")
verifier.expect(2, 3, "2 N  Aug 29   RSS 2.0 Item 2")
verifier.expect(3, 3, "3 N  Aug 28   RSS 2.0 Item 3")
# end auto-generated verification #6 

Tuitest.keypress("A"[0])

Tuitest.wait_until_idle
# begin auto-generated verification #7 
verifier.expect(0, 58, "0")
verifier.expect(2, 5, " ")
verifier.expect(3, 5, " ")
# end auto-generated verification #7 

Tuitest.keypress("/"[0])

Tuitest.wait_until_idle
# begin auto-generated verification #8 
verifier.expect(24, 0, "Search for:")
# end auto-generated verification #8 

Tuitest.keypress("d"[0])
Tuitest.keypress("e"[0])
Tuitest.keypress("s"[0])
Tuitest.keypress("c"[0])
Tuitest.keypress(10)

Tuitest.wait_until_idle
# begin auto-generated verification #9 
verifier.expect(0, 17, "Search result (0 unread, 2 total)                              ")
verifier.expect(1, 12, "29   |RSS 2.0 testbed f|  RSS 2.0 Item 2")
verifier.expect(2, 13, "8   |RSS 2.0 testbed f|  RSS 2.0 Item 3")
verifier.expect(3, 3, "                            ")
verifier.expect(24, 0, "           ")
# end auto-generated verification #9 

Tuitest.keypress("q"[0])
Tuitest.keypress("f"[0])

Tuitest.wait_until_idle
# begin auto-generated verification #10 
verifier.expect(24, 0, "No filters defined.")
# end auto-generated verification #10 

Tuitest.keypress("F"[0])
Tuitest.keypress("c"[0])
Tuitest.keypress("o"[0])
Tuitest.keypress("n"[0])
Tuitest.keypress("t"[0])
Tuitest.keypress("e"[0])
Tuitest.keypress("n"[0])
Tuitest.keypress("t"[0])
Tuitest.keypress(" "[0])
Tuitest.keypress("="[0])
Tuitest.keypress("~"[0])
Tuitest.keypress(" "[0])
Tuitest.keypress("\""[0])
Tuitest.keypress("d"[0])
Tuitest.keypress("e"[0])
Tuitest.keypress("s"[0])
Tuitest.keypress("c"[0])
Tuitest.keypress("\""[0])

Tuitest.wait_until_idle
# begin auto-generated verification #11 
verifier.expect(24, 0, "Filter: content =~ \"desc\"")
# end auto-generated verification #11 

Tuitest.keypress(10)

Tuitest.wait_until_idle
# begin auto-generated verification #12 
verifier.expect(1, 3, "2    Aug 29   RSS 2.0 Item 2")
verifier.expect(2, 3, "3    Aug 28   RSS 2.0 Item 3")
verifier.expect(3, 3, "                            ")
verifier.expect(24, 0, "                         ")
# end auto-generated verification #12 

Tuitest.keypress(6)

Tuitest.wait_until_idle
# begin auto-generated verification #13 
verifier.expect(1, 3, "1    Aug 30   RSS 2.0 Item 1")
verifier.expect(2, 3, "2    Aug 29   RSS 2.0 Item 2 ")
verifier.expect(3, 3, "3    Aug 28   RSS 2.0 Item 3")
# end auto-generated verification #13 

Tuitest.keypress("q"[0])
Tuitest.keypress("q"[0])

Tuitest.wait(1000)

Tuitest.wait_until_idle

Tuitest.close
verifier.finish

Kernel.system("rm -f cache")

# EOF
