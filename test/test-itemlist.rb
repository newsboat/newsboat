#!/usr/bin/env ruby
# auto-generated tuitest script
require 'tuitest'

Tuitest.init
verifier = Tuitest::Verifier.new("test-itemlist.rb.log", "RESULT-test-itemlist.rb.xml")

Kernel.system("rm -f cache cache.lock")

if ENV["OFFLINE"] then
	Tuitest.run("../newsbeuter -c cache -C /dev/null -u urls-tuitest1-offline")
else
	Tuitest.run("../newsbeuter -c cache -C /dev/null -u urls-tuitest1")
end


Tuitest.wait_until_idle
Tuitest.keypress("r"[0])
Tuitest.keypress(10)
Tuitest.keypress(":"[0])
Tuitest.keypress("s"[0])
Tuitest.keypress("e"[0])
Tuitest.keypress("t"[0])
Tuitest.keypress(" "[0])
Tuitest.keypress("b"[0])
Tuitest.keypress("r"[0])
Tuitest.keypress("o"[0])
Tuitest.keypress("w"[0])
Tuitest.keypress("s"[0])
Tuitest.keypress("e"[0])
Tuitest.keypress("r"[0])
Tuitest.keypress("="[0])
Tuitest.keypress("t"[0])
Tuitest.keypress("r"[0])
Tuitest.keypress("u"[0])
Tuitest.keypress("e"[0])
Tuitest.keypress(10)
Tuitest.keypress("o"[0])
Tuitest.keypress("u"[0])

Tuitest.wait_until_idle

# begin auto-generated verification #1 
verifier.expect(24, 0, "URL list empty.")
# end auto-generated verification #1 

Tuitest.keypress(258)
Tuitest.keypress("u"[0])
Tuitest.keypress(258)
Tuitest.keypress("u"[0])

Tuitest.wait_until_idle

# begin auto-generated verification #2 
verifier.expect(0, 17, "URLs")
verifier.expect(1, 1, "1  http://slashdot.org/       ")
verifier.expect(2, 3, "                            ")
# end auto-generated verification #2 

Tuitest.keypress("q"[0])
Tuitest.keypress(2)

Tuitest.wait_until_idle

# begin auto-generated verification #3 
verifier.expect(0, 17, "Articles in feed 'RSS 2.0 testbed feed' (3 unread, 3 total)")
verifier.expect(1, 1, "  1 N  Aug 30   212   RSS 2.0 Item 1")
verifier.expect(2, 3, "2 N  Aug 29    67   RSS 2.0 Item 2")
verifier.expect(3, 3, "3 N  Aug 28   170   RSS 2.0 Item 3")
verifier.expect(24, 0, "URL: http://testbed.newsbeuter.org/item3.html")
# end auto-generated verification #3 

Tuitest.keypress(10)

Tuitest.wait_until_idle

# begin auto-generated verification #4 
verifier.expect(24, 0, "Title: RSS 2.0 Item 3")
# end auto-generated verification #4 

Tuitest.keypress(10)
Tuitest.keypress("t"[0])
Tuitest.keypress("e"[0])
Tuitest.keypress("s"[0])
Tuitest.keypress("t"[0])

Tuitest.wait_until_idle

# begin auto-generated verification #5 
verifier.expect(24, 0, "Description: test    ")
# end auto-generated verification #5 

Tuitest.keypress(10)

Tuitest.wait_until_idle

# begin auto-generated verification #6 
verifier.expect(24, 0, "Error while saving bookmark: bookmarking support is not configured. Please set t")
# end auto-generated verification #6 

Tuitest.keypress(5)
Tuitest.keypress("a"[0])
Tuitest.keypress("b"[0])
Tuitest.keypress("c"[0])

Tuitest.wait_until_idle

# begin auto-generated verification #7 
verifier.expect(24, 0, "Flags: abc                                                                      ")
# end auto-generated verification #7 

Tuitest.keypress(10)

Tuitest.wait_until_idle

# begin auto-generated verification #8 
verifier.expect(3, 6, "!")
verifier.expect(24, 5, " updated.")
# end auto-generated verification #8 

Tuitest.keypress("s"[0])

Tuitest.wait(1000)
Tuitest.wait_until_idle

# begin auto-generated verification #9 
verifier.expect(0, 17, "Save File - ")
verifier.expect(23, 2, "Cancel ENTER:Save                                                            ")
verifier.expect(24, 0, "              ")
# end auto-generated verification #9 

Tuitest.keypress(259)
Tuitest.keypress("q"[0])

Tuitest.wait_until_idle

# begin auto-generated verification #10 
verifier.expect(0, 17, "Articles in feed 'RSS 2.0 testbed feed' (3 unread, 3 total)")
verifier.expect(1, 0, "   1 N  Aug 30   212   RSS 2.0 Item 1             ")
verifier.expect(2, 0, "   2 N  Aug 29    67   RSS 2.0 Item 2                ")
verifier.expect(3, 0, "   3 N! Aug 28   170   RSS 2.0 Item 3                     ")
verifier.expect(4, 0, "                                                ")
verifier.expect(24, 0, "Aborted saving.")
# end auto-generated verification #10 

Tuitest.keypress("?"[0])

Tuitest.wait_until_idle

# begin auto-generated verification #11 
verifier.expect(0, 17, "Help")
# end auto-generated verification #11 

Tuitest.keypress("q"[0])
Tuitest.keypress("n"[0])
Tuitest.keypress("n"[0])
Tuitest.keypress("n"[0])
Tuitest.keypress("p"[0])
Tuitest.keypress("p"[0])
Tuitest.keypress("p"[0])
Tuitest.keypress(16)
Tuitest.keypress("A"[0])
Tuitest.keypress(16)

Tuitest.wait_until_idle

# begin auto-generated verification #12 
verifier.expect(0, 58, "0")
verifier.expect(1, 5, " ")
verifier.expect(2, 5, " ")
verifier.expect(3, 5, " ")
verifier.expect(24, 0, "No unread feeds.")
# end auto-generated verification #12 

Tuitest.keypress("q"[0])
Tuitest.keypress("q"[0])

Tuitest.wait_until_idle

Tuitest.close
verifier.finish

Kernel.system("rm -f cache cache.lock")

# EOF
