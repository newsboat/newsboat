#!/usr/bin/env ruby
# auto-generated tuitest script
require 'tuitest'

Tuitest.init
verifier = Tuitest::Verifier.new("test-dialogs.rb.log", "RESULT-test-dialogs.rb.xml")

Kernel.system("rm -f cache cache.lock")

if ENV["OFFLINE"] then
	Tuitest.run("../newsbeuter -c cache -C /dev/null -u urls-tuitest1-offline")
else
	Tuitest.run("../newsbeuter -c cache -C /dev/null -u urls-tuitest1")
end


Tuitest.wait_until_idle
Tuitest.keypress("r"[0])
Tuitest.keypress(10)
Tuitest.keypress(10)
Tuitest.keypress("v"[0])

Tuitest.wait_until_idle

# begin auto-generated verification #1 
verifier.expect(0, 17, "Dialogs                 ")
verifier.expect(1, 0, "   1   Feed List - 1 unread, 1 total")
verifier.expect(2, 0, "   2   Article List - RSS 2.0 testbed feed")
verifier.expect(3, 0, "   3 * Article - RSS 2.0 Item 1              ")
verifier.expect(4, 0, "                                              ")
# end auto-generated verification #1 

Tuitest.keypress(258)
Tuitest.keypress(10)
Tuitest.keypress(258)
Tuitest.keypress(10)
Tuitest.keypress("v"[0])

Tuitest.wait_until_idle

# begin auto-generated verification #2 
verifier.expect(3, 5, " ")
verifier.expect(4, 3, "4 * Article - RSS 2.0 Item 2")
# end auto-generated verification #2 

Tuitest.keypress(258)
Tuitest.keypress(258)
Tuitest.keypress(258)
Tuitest.keypress(24)

Tuitest.wait_until_idle

# begin auto-generated verification #3 
verifier.expect(1, 5, "*")
verifier.expect(4, 3, "                            ")
# end auto-generated verification #3 

Tuitest.keypress(24)

Tuitest.wait_until_idle

# begin auto-generated verification #4 
verifier.expect(3, 3, "                            ")
# end auto-generated verification #4 

Tuitest.keypress(24)

Tuitest.wait_until_idle

# begin auto-generated verification #5 
verifier.expect(2, 3, "                                       ")
# end auto-generated verification #5 

Tuitest.keypress(24)

Tuitest.wait_until_idle

# begin auto-generated verification #6 
verifier.expect(24, 0, "Error: you can't remove the feed list!")
# end auto-generated verification #6 

Tuitest.keypress("q"[0])
Tuitest.keypress("q"[0])

Tuitest.wait_until_idle

Tuitest.close
verifier.finish

Kernel.system("rm -f cache cache.lock")

# EOF
