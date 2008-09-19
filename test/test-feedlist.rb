#!/usr/bin/env ruby
# test misc. functionality of the feedlist dialog
require 'tuitest'

Kernel.system("rm -f cache")

Tuitest.init
verifier = Tuitest::Verifier.new("test-feedlist.rb.log")


Tuitest.run("../newsbeuter -c cache -C /dev/null -u urls-tuitest1")

Tuitest.wait_until_expected_text(0, 0, "newsbeuter")
Tuitest.keypress(18)
Tuitest.keypress("r"[0])
Tuitest.keypress("R"[0])
Tuitest.wait_until_idle

Tuitest.keypress("A"[0])
Tuitest.keypress("l"[0])

Tuitest.wait_until_expected_text(0, 39, "0", 5000)
# begin auto-generated verification #1 
verifier.expect(0, 39, "0");
verifier.expect(1, 3, "                                    ")
# end auto-generated verification #1 


Tuitest.keypress("l"[0])

Tuitest.wait_until_expected_text(0, 39, "1", 5000)
# begin auto-generated verification #3 
verifier.expect(0, 39, "1")
verifier.expect(1, 3, "1         (0/3) RSS 2.0 testbed feed")
# end auto-generated verification #3 

Tuitest.keypress("n"[0])
Tuitest.keypress("p"[0])
Tuitest.keypress("C"[0])
Tuitest.keypress(20)
Tuitest.keypress("t"[0])
Tuitest.keypress(10)
Tuitest.keypress(20)
Tuitest.keypress(10)
Tuitest.keypress("q"[0])
Tuitest.keypress("t"[0])
Tuitest.keypress(10)

Tuitest.wait_until_expected_text(0, 48, "- tag ", 5000)
# begin auto-generated verification #4 
verifier.expect(0, 48, "- tag `mytag'")
# end auto-generated verification #4 

Tuitest.keypress(20)

Tuitest.wait_until_expected_text(0, 48, "             ", 5000)
# begin auto-generated verification #5 
verifier.expect(0, 48, "             ")
# end auto-generated verification #5 

Tuitest.keypress("/"[0])
Tuitest.keypress("d"[0])
Tuitest.keypress("e"[0])
Tuitest.keypress("s"[0])
Tuitest.keypress("c"[0])
Tuitest.keypress(10)
Tuitest.keypress(10)
Tuitest.keypress("q"[0])
Tuitest.keypress("q"[0])
Tuitest.keypress("/"[0])
Tuitest.keypress("t"[0])
Tuitest.keypress("e"[0])
Tuitest.keypress("s"[0])
Tuitest.keypress("t"[0])
Tuitest.keypress(8)
Tuitest.keypress(8)
Tuitest.keypress(10)
Tuitest.keypress(259)
Tuitest.keypress(12)
Tuitest.keypress("/"[0])

Tuitest.wait_until_expected_text(24, 0, "Search for:", 5000)
# begin auto-generated verification #6 
verifier.expect(24, 0, "Search for:")
# end auto-generated verification #6 

Tuitest.keypress("d"[0])
Tuitest.keypress("e"[0])
Tuitest.keypress("s"[0])
Tuitest.keypress("c"[0])
Tuitest.keypress(10)

Tuitest.wait_until_expected_text(0, 17, "Search result (0 unread, 2 total)", 5000)
# begin auto-generated verification #7 
verifier.expect(0, 17, "Search result (0 unread, 2 total)")
verifier.expect(1, 3, "1    Aug 29   |RSS 2.0 testbed f|  RSS 2.0 Item 2")
verifier.expect(2, 3, "2    Aug 28   |RSS 2.0 testbed f|  RSS 2.0 Item 3")
# end auto-generated verification #7 

Tuitest.keypress(10)
Tuitest.keypress("q"[0])
Tuitest.keypress(258)
Tuitest.keypress(10)
Tuitest.keypress("q"[0])
Tuitest.keypress("q"[0])
Tuitest.keypress("q"[0])

Tuitest.wait_until_idle

Tuitest.close
verifier.finish

Kernel.system("rm -f cache")

# EOF
