#!/usr/bin/env ruby
# auto-generated tuitest script
require 'tuitest'

Kernel.system("rm -f cache")

Tuitest.init
verifier = Tuitest::Verifier.new("test-reloadthread.rb.log")


Tuitest.run("../newsbeuter -c cache -C config-reloadthread -u urls-tuitest1")


Tuitest.wait_until_idle
Tuitest.keypress(271)

Tuitest.wait_until_idle
verifier.expect(1, 5, "N       (3/3) RSS 2.0 testbed feed                   ")

Tuitest.keypress("q"[0])

Tuitest.wait_until_idle

Tuitest.close
verifier.finish

Kernel.system("rm -f cache")

# EOF
