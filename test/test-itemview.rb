#!/usr/bin/env ruby
# test misc. functions of itemview.
require 'tuitest'

Kernel.system("rm -f cache")

Tuitest.init
verifier = Tuitest::Verifier.new("test-itemview.rb.log")


Tuitest.run("../newsbeuter -c cache -C /dev/null -u urls-tuitest1")


Tuitest.wait_until_idle
Tuitest.keypress("r"[0])
Tuitest.keypress(10)
Tuitest.keypress(10)
Tuitest.keypress(" "[0])
Tuitest.keypress("b"[0])
Tuitest.keypress(21)

Tuitest.wait_until_idle
# begin auto-generated verification #1 
verifier.expect(7, 0, "                             ")
verifier.expect(8, 0, "<p>This is some example content. </p>")
verifier.expect(9, 0, "<p>Here's a line break.<br />")
verifier.expect(10, 0, "</p>                                           ")
verifier.expect(11, 0, "<ul>")
verifier.expect(12, 0, "<li>first entry</li>")
verifier.expect(13, 0, "<li>second entry</li>")
verifier.expect(14, 0, "<li>third entry</li>")
verifier.expect(15, 0, "</ul>")
verifier.expect(16, 0, " ")
# end auto-generated verification #1 

Tuitest.keypress(21)

Tuitest.wait_until_idle
# begin auto-generated verification #2 
verifier.expect(7, 0, "This is some example content.")
verifier.expect(8, 0, "                                     ")
verifier.expect(9, 0, "Here's a line break.         ")
verifier.expect(10, 0, "Second line. And now we try an unordered list.")
verifier.expect(11, 0, "    ")
verifier.expect(12, 0, "  * first entry     ")
verifier.expect(13, 0, "  * second entry     ")
verifier.expect(14, 0, "  * third entry     ")
verifier.expect(15, 0, "     ")
verifier.expect(16, 0, "~")
# end auto-generated verification #2 

Tuitest.keypress("s"[0])
Tuitest.keypress(259)
Tuitest.keypress("q"[0])
Tuitest.keypress(2)
Tuitest.keypress(10)
Tuitest.keypress(10)
Tuitest.keypress(10)

Tuitest.wait_until_idle
# begin auto-generated verification #3 
verifier.expect(24, 0, "Error while saving bookmark: bookmarking support is not configured. Please set t")
# end auto-generated verification #3 

Tuitest.keypress("p"[0])
Tuitest.keypress(":"[0])
Tuitest.keypress("s"[0])
Tuitest.keypress("a"[0])
Tuitest.keypress("v"[0])
Tuitest.keypress("e"[0])
Tuitest.keypress(" "[0])
Tuitest.keypress("a"[0])
Tuitest.keypress("r"[0])
Tuitest.keypress("t"[0])
Tuitest.keypress("i"[0])
Tuitest.keypress("c"[0])
Tuitest.keypress("l"[0])
Tuitest.keypress("e"[0])
Tuitest.keypress("."[0])
Tuitest.keypress("t"[0])
Tuitest.keypress("x"[0])
Tuitest.keypress("t"[0])
Tuitest.keypress(10)
Tuitest.keypress("q"[0])
Tuitest.keypress("q"[0])
Tuitest.keypress("q"[0])

Tuitest.wait_until_idle

Tuitest.close
verifier.finish

Kernel.system("rm -f cache article.txt")

# EOF
