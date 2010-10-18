#!/usr/bin/env ruby
# auto-generated tuitest script
require 'tuitest'

Tuitest.init
verifier = Tuitest::Verifier.new("test-formaction.rb.log", "RESULT-test-formaction.rb.xml")


Tuitest.run("../newsbeuter -c cache -C /dev/null -u urls-tuitest1")


Tuitest.wait_until_idle
Tuitest.keypress(":"[0])
Tuitest.keypress("s"[0])
Tuitest.keypress(9)

Tuitest.wait_until_idle

# begin auto-generated verification #1 
verifier.expect(0, 0, "newsbeuter 2.4 - Your feeds (0 unread, 1 total)")
verifier.expect(1, 3, "1         (0/0) http://testbed.newsbeuter.org/rss20.xml")
verifier.expect(23, 0, "q:Quit ENTER:Open n:Next Unread r:Reload R:Reload All A:Mark Read C:Catchup All")
verifier.expect(24, 0, ":set")
# end auto-generated verification #1 

Tuitest.keypress(9)

Tuitest.wait_until_idle

# begin auto-generated verification #2 
verifier.expect(24, 2, "ource")
# end auto-generated verification #2 

Tuitest.keypress(9)

Tuitest.wait_until_idle

# begin auto-generated verification #3 
verifier.expect(24, 2, "et   ")
# end auto-generated verification #3 

Tuitest.keypress(" "[0])
Tuitest.keypress(9)

Tuitest.wait_until_idle

# begin auto-generated verification #4 
verifier.expect(24, 5, "always-display-description")
# end auto-generated verification #4 

Tuitest.keypress(9)

Tuitest.wait_until_idle

# begin auto-generated verification #5 
verifier.expect(24, 6, "rticle-sort-order        ")
# end auto-generated verification #5 

Tuitest.keypress(9)

Tuitest.wait_until_idle

# begin auto-generated verification #6 
verifier.expect(24, 12, "list-format")
# end auto-generated verification #6 

Tuitest.keypress(7)

Tuitest.wait_until_idle

# begin auto-generated verification #7 
verifier.expect(24, 0, "                       ")
# end auto-generated verification #7 

Tuitest.keypress("q"[0])

Tuitest.wait_until_idle

Tuitest.close
verifier.finish

# EOF
