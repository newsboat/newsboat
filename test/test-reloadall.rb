#!/usr/bin/env ruby
# test "reload all" (which starts a background thread)
require 'tuitest'

Kernel.system("rm -f cache")

Tuitest.init
verifier = Tuitest::Verifier.new("test-reloadall.rb.log")


Tuitest.run("../newsbeuter -c cache -C /dev/null -u urls-tuitest1")
Tuitest.wait_until_idle

Tuitest.keypress("R"[0])

Tuitest.wait_until_idle
verifier.expect(0, 29, "1")
verifier.expect(1, 5, "N       (3/3) RSS 2.0 testbed feed                   ")

Tuitest.keypress("q"[0])
Tuitest.wait(1000)
Tuitest.wait_until_idle

Tuitest.close
verifier.finish

Kernel.system("rm -f cache")

# EOF
