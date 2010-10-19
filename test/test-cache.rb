#!/usr/bin/env ruby
# auto-generated tuitest script
require 'tuitest'

Tuitest.init
verifier = Tuitest::Verifier.new("test-cache.rb.log", "RESULT-test-cache.rb.xml")


Tuitest.run("../newsbeuter -c cache -C config-reset -u urls-feedlist")


Tuitest.wait_until_idle
Tuitest.keypress("R"[0])

Tuitest.wait_until_idle

# begin auto-generated verification #1 
verifier.expect(0, 29, "3")
verifier.expect(1, 5, "N       (3/3) HTML rendering testbed feed                 ")
verifier.expect(2, 5, "N       (1/1) html table rendering testbed feed       ")
verifier.expect(3, 5, "N       (3/3) RSS 2.0 testbed feed                   ")
# end auto-generated verification #1 

Tuitest.keypress("C"[0])

Tuitest.wait_until_idle

# begin auto-generated verification #2 
verifier.expect(0, 29, "0")
verifier.expect(1, 5, "        (0")
verifier.expect(2, 5, "        (0")
verifier.expect(3, 5, "        (0")
# end auto-generated verification #2 

Tuitest.keypress("R"[0])

Tuitest.wait_until_idle

# begin auto-generated verification #3 
# end auto-generated verification #3 

Tuitest.keypress("q"[0])

Tuitest.wait_until_idle

Tuitest.close
verifier.finish

Kernel.system("rm -f cache cache.lock")

# EOF
