#!/usr/bin/env ruby
# auto-generated tuitest script
require 'tuitest'

Tuitest.init
verifier = Tuitest::Verifier.new("test-feedlist2.rb.log", "RESULT-test-feedlist2.rb.xml")


Tuitest.run("../newsbeuter -c cache -C /dev/null -u urls-feedlist")


Tuitest.wait_until_idle
Tuitest.keypress("n"[0])

Tuitest.wait_until_idle

verifier.expect(24, 0, "No feeds with unread items.")

Tuitest.keypress("r"[0])

Tuitest.wait_until_idle

verifier.expect(1, 5, "N       (3/3) HTML rendering testbed feed                 ")

Tuitest.keypress(258)
Tuitest.keypress("r"[0])

Tuitest.wait_until_idle

verifier.expect(2, 5, "N       (1/1) html table rendering testbed feed       ")

Tuitest.keypress(258)
Tuitest.keypress("r"[0])

Tuitest.wait_until_idle

verifier.expect(3, 5, "N       (3/3) RSS 2.0 testbed feed                   ")

Tuitest.keypress("p"[0])
Tuitest.keypress("p"[0])
Tuitest.keypress(10)

Tuitest.wait_until_idle

verifier.expect(0, 17, "Articles in feed 'HTML rendering testbed feed' (3 unread, 3 tot")
verifier.expect(1, 8, "Aug 30   405   HTML Item 1            ")
verifier.expect(2, 8, "Aug 29   394   HTML Item 2                  ")
verifier.expect(3, 8, "Aug 28   187   HTML Item 3     ")

Tuitest.keypress("q"[0])
Tuitest.keypress("n"[0])
Tuitest.keypress(10)

Tuitest.wait_until_idle

verifier.expect(0, 35, "html table rendering testbed feed' (1 unread,")
verifier.expect(1, 17, "291   table item 1")
verifier.expect(2, 3, "                               ")
verifier.expect(3, 3, "                               ")

Tuitest.keypress("q"[0])
Tuitest.keypress("n"[0])
Tuitest.keypress(10)

Tuitest.wait_until_idle

verifier.expect(0, 35, "RSS 2.0 testbed feed' (3 unread, 3 total) - h")
verifier.expect(1, 18, "12   RSS 2.0 Item 1")
verifier.expect(2, 3, "2 N  Aug 29    67   RSS 2.0 Item 2")
verifier.expect(3, 3, "3 N  Aug 28   170   RSS 2.0 Item 3")

Tuitest.keypress("q"[0])
Tuitest.keypress("g"[0])
Tuitest.keypress("t"[0])

Tuitest.wait_until_idle

verifier.expect(0, 17, "Your feeds (3 unread, 3 total)                                 ")
verifier.expect(1, 8, "     (3/3) HTML rendering testbed feed")
verifier.expect(2, 8, "     (1/1) html table rendering testbed feed")
verifier.expect(3, 8, "     (3/3) RSS 2.0 testbed feed")
verifier.expect(23, 18, "n:Next Unread r:Reload R:Reload All A:Mark Read C:Catchup All")

Tuitest.keypress("g"[0])
Tuitest.keypress("a"[0])

Tuitest.wait_until_idle

verifier.expect(1, 14, "1/1) html table rendering testbed feed")
verifier.expect(2, 14, "3/3) HTML rendering testbed feed      ")

Tuitest.keypress("g"[0])
Tuitest.keypress("n"[0])

Tuitest.wait_until_idle

verifier.expect(1, 14, "3/3) HTML rendering testbed feed      ")
verifier.expect(2, 14, "1/1) html table rendering testbed feed")

Tuitest.keypress("g"[0])
Tuitest.keypress("t"[0])
Tuitest.keypress("G"[0])
Tuitest.keypress("t"[0])

Tuitest.wait_until_idle

verifier.expect(1, 19, "RSS 2.0 testbed feed       ")
verifier.expect(3, 19, "HTML rendering testbed feed")

Tuitest.keypress("G"[0])
Tuitest.keypress("a"[0])

Tuitest.wait_until_idle

verifier.expect(1, 19, "HTML rendering testbed feed")
verifier.expect(2, 14, "3/3) RSS 2.0 testbed feed             ")
verifier.expect(3, 14, "1/1) html table rendering testbed feed")

Tuitest.keypress("F"[0])
Tuitest.keypress("f"[0])
Tuitest.keypress("e"[0])
Tuitest.keypress("e"[0])
Tuitest.keypress("d"[0])
Tuitest.keypress("t"[0])
Tuitest.keypress("i"[0])
Tuitest.keypress("t"[0])
Tuitest.keypress("l"[0])
Tuitest.keypress("e"[0])
Tuitest.keypress(" "[0])
Tuitest.keypress("="[0])
Tuitest.keypress("~"[0])
Tuitest.keypress(" "[0])
Tuitest.keypress("\""[0])
Tuitest.keypress("h"[0])
Tuitest.keypress("t"[0])
Tuitest.keypress("m"[0])
Tuitest.keypress("l"[0])
Tuitest.keypress("\""[0])
Tuitest.keypress(10)

Tuitest.wait_until_idle

verifier.expect(0, 29, "2 unread, 2")
verifier.expect(2, 3, "3 N       (1/1) html table rendering testbed feed")
verifier.expect(3, 3, "                                                 ")

Tuitest.keypress(6)

Tuitest.wait_until_idle

verifier.expect(0, 29, "3 unread, 3")
verifier.expect(2, 3, "2 N       (3/3) RSS 2.0 testbed feed             ")
verifier.expect(3, 3, "3 N       (1/1) html table rendering testbed feed")

Tuitest.keypress("F"[0])
Tuitest.keypress(259)
Tuitest.keypress(260)
Tuitest.keypress(260)
Tuitest.keypress(260)
Tuitest.keypress(260)
Tuitest.keypress(260)
Tuitest.keypress(260)
Tuitest.keypress(260)
Tuitest.keypress(260)
Tuitest.keypress(263)
Tuitest.keypress(260)
Tuitest.keypress(330)
Tuitest.keypress("!"[0])
Tuitest.keypress(10)

Tuitest.wait_until_idle

verifier.expect(0, 29, "1 unread, 1")
verifier.expect(1, 3, "2 N       (3/3) RSS 2.0 testbed feed       ")
verifier.expect(2, 3, "                                    ")
verifier.expect(3, 3, "                                                 ")

Tuitest.keypress(6)

Tuitest.wait_until_idle

verifier.expect(0, 29, "3 unread, 3")
verifier.expect(1, 3, "1 N       (3/3) HTML rendering testbed feed")
verifier.expect(2, 3, "2 N       (3/3) RSS 2.0 testbed feed")
verifier.expect(3, 3, "3 N       (1/1) html table rendering testbed feed")

Tuitest.keypress("?"[0])

Tuitest.wait_until_idle

verifier.expect(0, 17, "Help                          ")

Tuitest.keypress("q"[0])

Tuitest.wait_until_idle

verifier.expect(0, 17, "Your feeds (3 unread, 3 total)")
verifier.expect(1, 0, "   1 N       (3/3) HTML rendering testbed feed           ")
verifier.expect(2, 0, "   2 N       (3/3) RSS 2.0 testbed feed                               ")
verifier.expect(3, 0, "   3 N       (1/1) html table rendering testbed feed                  ")
verifier.expect(4, 0, "                                                                      ")

Tuitest.keypress(":"[0])
Tuitest.keypress("1"[0])
Tuitest.keypress(10)
Tuitest.keypress("p"[0])
Tuitest.keypress(10)

Tuitest.wait_until_idle

verifier.expect(0, 17, "Articles in feed 'html table rendering testbed feed' (1 unread,")

Tuitest.keypress("q"[0])
Tuitest.keypress("q"[0])

Tuitest.wait_until_idle

Tuitest.close
verifier.finish

Kernel.system("rm -f cache cache.lock")

# EOF
