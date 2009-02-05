#!/usr/bin/env ruby
# auto-generated tuitest script
require 'tuitest'

Kernel.system("rm -f cache cache.lock")

Tuitest.init
verifier = Tuitest::Verifier.new("test-reloadthread.rb.log", "RESULT-test-reloadthread.rb.xml")


if ENV["OFFLINE"] then
	Tuitest.run("../newsbeuter -c cache -C config-reloadthread -u urls-tuitest1-offline")
else
	Tuitest.run("../newsbeuter -c cache -C config-reloadthread -u urls-tuitest1")
end


Tuitest.wait_until_expected_text(0, 0, "newsbeuter ", 5000)
Tuitest.keypress(271)

Tuitest.wait_until_expected_text(1, 5, "N       (3/3) RSS 2.0 testbed feed                   ", 5000)
verifier.expect(1, 5, "N       (3/3) RSS 2.0 testbed feed                   ")

Tuitest.keypress("q"[0])

Tuitest.wait_until_idle

Tuitest.close
verifier.finish

Kernel.system("rm -f cache cache.lock")

# EOF
