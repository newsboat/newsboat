#!/usr/bin/env ruby
# auto-generated tuitest script
require 'tuitest'

Tuitest.init
verifier = Tuitest::Verifier.new("test-dumpconfig.rb.log", "RESULT-test-dumpconfig.rb.xml")


Tuitest.run("../newsbeuter -c cache -C /dev/null -u urls-tuitest1")


Tuitest.wait_until_idle
Tuitest.keypress(":"[0])
Tuitest.keypress("d"[0])
Tuitest.keypress("u"[0])
Tuitest.keypress(9)
Tuitest.keypress(" "[0])
Tuitest.keypress("c"[0])
Tuitest.keypress("o"[0])
Tuitest.keypress("n"[0])
Tuitest.keypress("f"[0])
Tuitest.keypress("i"[0])
Tuitest.keypress("g"[0])
Tuitest.keypress("."[0])
Tuitest.keypress("d"[0])
Tuitest.keypress("u"[0])
Tuitest.keypress("m"[0])
Tuitest.keypress("p"[0])
Tuitest.keypress(10)
Tuitest.keypress("q"[0])

Tuitest.wait_until_idle

Tuitest.close
verifier.finish

Kernel.system("rm -f config.dump cache")

# EOF
