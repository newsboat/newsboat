#!/usr/bin/env ruby
# auto-generated tuitest script
require 'tuitest'

Tuitest.init
verifier = Tuitest::Verifier.new("test-feedlist3.rb.log", "RESULT-test-feedlist3.rb.xml")


Tuitest.run("../newsbeuter -c cache -C /dev/null -u urls-feedlist")


Tuitest.wait_until_idle
Tuitest.keypress("r"[0])
Tuitest.keypress(258)
Tuitest.keypress("r"[0])
Tuitest.keypress(258)
Tuitest.keypress("r"[0])

Tuitest.wait_until_idle

verifier.expect(1, 3, "1 N       (3/3) HTML rendering testbed feed")
verifier.expect(2, 3, "2 N       (1/1) html table rendering testbed feed")
verifier.expect(3, 3, "3 N       (3/3) RSS 2.0 testbed feed")
verifier.expect(23, 0, "q:Quit ENTER:Open n:Next Unread r:Reload R:Reload All A:Mark Read C:Catchup All")

Tuitest.keypress(":"[0])
Tuitest.keypress("g"[0])
Tuitest.keypress("o"[0])
Tuitest.keypress("t"[0])
Tuitest.keypress("o"[0])
Tuitest.keypress(" "[0])
Tuitest.keypress("h"[0])
Tuitest.keypress("t"[0])
Tuitest.keypress("m"[0])
Tuitest.keypress("l"[0])
Tuitest.keypress(" "[0])
Tuitest.keypress("r"[0])
Tuitest.keypress("e"[0])
Tuitest.keypress("n"[0])
Tuitest.keypress("d"[0])
Tuitest.keypress("e"[0])
Tuitest.keypress("r"[0])
Tuitest.keypress("i"[0])
Tuitest.keypress("n"[0])
Tuitest.keypress("g"[0])
Tuitest.keypress(10)
Tuitest.keypress(10)

Tuitest.wait_until_idle

verifier.expect(0, 17, "Articles in feed 'HTML rendering testbed feed' (3 unread, 3 tot")

Tuitest.keypress("q"[0])
Tuitest.keypress(":"[0])
Tuitest.keypress("g"[0])
Tuitest.keypress("o"[0])
Tuitest.keypress("t"[0])
Tuitest.keypress("o"[0])
Tuitest.keypress(" "[0])
Tuitest.keypress("r"[0])
Tuitest.keypress("s"[0])
Tuitest.keypress("s"[0])
Tuitest.keypress(10)
Tuitest.keypress(10)

Tuitest.wait_until_idle

verifier.expect(0, 35, "RSS 2.0 testbed feed' (3 unread, 3 total) - h")

Tuitest.keypress("q"[0])
Tuitest.keypress(":"[0])
Tuitest.keypress("g"[0])
Tuitest.keypress("o"[0])
Tuitest.keypress("t"[0])
Tuitest.keypress("o"[0])
Tuitest.keypress(" "[0])
Tuitest.keypress("t"[0])
Tuitest.keypress("a"[0])
Tuitest.keypress("b"[0])
Tuitest.keypress("l"[0])
Tuitest.keypress("e"[0])
Tuitest.keypress(10)
Tuitest.keypress(10)

Tuitest.wait_until_idle

verifier.expect(0, 35, "html table rendering testbed feed' (1 unread,")

Tuitest.keypress("q"[0])
Tuitest.keypress("q"[0])

Tuitest.wait_until_idle

Tuitest.close
verifier.finish

Kernel.system("rm -f cache cache.lock")

# EOF
