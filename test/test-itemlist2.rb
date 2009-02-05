#!/usr/bin/env ruby
# auto-generated tuitest script
require 'tuitest'

Tuitest.init
verifier = Tuitest::Verifier.new("test-itemlist2.rb.log", "RESULT-test-itemlist2.rb.xml")

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
Tuitest.keypress("3"[0])
Tuitest.keypress(10)
Tuitest.keypress("N"[0])
Tuitest.keypress(12)
Tuitest.keypress(10)
Tuitest.keypress("q"[0])

Tuitest.wait_until_idle

# begin auto-generated verification #1 
verifier.expect(0, 58, "2")
verifier.expect(3, 5, " ")
# end auto-generated verification #1 

Tuitest.keypress(":"[0])
Tuitest.keypress("1"[0])
Tuitest.keypress(10)
Tuitest.keypress("N"[0])

Tuitest.wait_until_idle

# begin auto-generated verification #2 
verifier.expect(0, 58, "1")
verifier.expect(1, 5, " ")
# end auto-generated verification #2 

Tuitest.keypress(259)
Tuitest.keypress("N"[0])
Tuitest.keypress(258)
Tuitest.keypress("N"[0])
Tuitest.keypress(":"[0])
Tuitest.keypress("1"[0])
Tuitest.keypress(10)
Tuitest.keypress(":"[0])
Tuitest.keypress("s"[0])
Tuitest.keypress("e"[0])
Tuitest.keypress("t"[0])
Tuitest.keypress(" "[0])
Tuitest.keypress("m"[0])
Tuitest.keypress("a"[0])
Tuitest.keypress("r"[0])
Tuitest.keypress("k"[0])
Tuitest.keypress("-"[0])
Tuitest.keypress("a"[0])
Tuitest.keypress("s"[0])
Tuitest.keypress("-"[0])
Tuitest.keypress("r"[0])
Tuitest.keypress("e"[0])
Tuitest.keypress("a"[0])
Tuitest.keypress("d"[0])
Tuitest.keypress("-"[0])
Tuitest.keypress("o"[0])
Tuitest.keypress("n"[0])
Tuitest.keypress("-"[0])
Tuitest.keypress("h"[0])
Tuitest.keypress("o"[0])
Tuitest.keypress("v"[0])
Tuitest.keypress("e"[0])
Tuitest.keypress("r"[0])
Tuitest.keypress("="[0])
Tuitest.keypress("t"[0])
Tuitest.keypress("r"[0])
Tuitest.keypress("u"[0])
Tuitest.keypress("e"[0])
Tuitest.keypress(10)
Tuitest.keypress(258)

Tuitest.wait_until_idle

# begin auto-generated verification #3 
verifier.expect(0, 58, "1")
verifier.expect(2, 5, " ")
# end auto-generated verification #3 

Tuitest.keypress(258)

Tuitest.wait_until_idle

# begin auto-generated verification #4 
verifier.expect(0, 58, "0")
verifier.expect(3, 5, " ")
# end auto-generated verification #4 

Tuitest.keypress(":"[0])
Tuitest.keypress("1"[0])
Tuitest.keypress(10)
Tuitest.keypress("N"[0])
Tuitest.keypress("N"[0])
Tuitest.keypress("N"[0])

Tuitest.wait_until_idle

# begin auto-generated verification #5 
verifier.expect(0, 58, "2")
verifier.expect(1, 5, "N")
verifier.expect(2, 5, "N")
# end auto-generated verification #5 

Tuitest.keypress(":"[0])
Tuitest.keypress("s"[0])
Tuitest.keypress("e"[0])
Tuitest.keypress("t"[0])
Tuitest.keypress(" "[0])
Tuitest.keypress("m"[0])
Tuitest.keypress("a"[0])
Tuitest.keypress("r"[0])
Tuitest.keypress("k"[0])
Tuitest.keypress("-"[0])
Tuitest.keypress("a"[0])
Tuitest.keypress("s"[0])
Tuitest.keypress("-"[0])
Tuitest.keypress("r"[0])
Tuitest.keypress("e"[0])
Tuitest.keypress("a"[0])
Tuitest.keypress("d"[0])
Tuitest.keypress("-"[0])
Tuitest.keypress("o"[0])
Tuitest.keypress("n"[0])
Tuitest.keypress("-"[0])
Tuitest.keypress("h"[0])
Tuitest.keypress("o"[0])
Tuitest.keypress("v"[0])
Tuitest.keypress("e"[0])
Tuitest.keypress("r"[0])
Tuitest.keypress("="[0])
Tuitest.keypress("f"[0])
Tuitest.keypress("a"[0])
Tuitest.keypress("l"[0])
Tuitest.keypress("s"[0])
Tuitest.keypress("e"[0])
Tuitest.keypress(10)
Tuitest.keypress("N"[0])
Tuitest.keypress(":"[0])
Tuitest.keypress("1"[0])
Tuitest.keypress(10)
Tuitest.keypress("N"[0])

Tuitest.wait_until_idle

# begin auto-generated verification #6 
verifier.expect(0, 58, "2")
verifier.expect(1, 5, " ")
# end auto-generated verification #6 

Tuitest.keypress("N"[0])

Tuitest.wait_until_idle

# begin auto-generated verification #7 
verifier.expect(0, 58, "1")
verifier.expect(2, 5, " ")
# end auto-generated verification #7 

Tuitest.keypress("N"[0])

Tuitest.wait_until_idle

# begin auto-generated verification #8 
verifier.expect(0, 58, "0")
verifier.expect(3, 5, " ")
# end auto-generated verification #8 

Tuitest.keypress("q"[0])
Tuitest.keypress("q"[0])

Tuitest.wait_until_idle

Tuitest.close
verifier.finish

Kernel.system("rm -f cache cache.lock")

# EOF
