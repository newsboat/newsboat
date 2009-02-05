#!/usr/bin/env ruby
# test "reload all" (which starts a background thread)
require 'tuitest'

Kernel.system("rm -f cache cache.lock")

Tuitest.init
verifier = Tuitest::Verifier.new("test-reloadall-retry.rb.log", "RESULT-test-reloadall-retry.rb.xml")


if ENV["OFFLINE"] then
	Tuitest.run("../newsbeuter -c cache -C config-retry -u urls-tuitest1-offline")
else
	Tuitest.run("../newsbeuter -c cache -C config-retry -u urls-tuitest1")
end
Tuitest.wait_until_expected_text(0, 0, "newsbeuter ", 5000)

Tuitest.keypress("R"[0])

Tuitest.wait_until_expected_text(0, 29, "1", 5000)
verifier.expect(0, 29, "1")
verifier.expect(1, 5, "N       (3/3) RSS 2.0 testbed feed                   ")

Tuitest.keypress("q"[0])
Tuitest.wait(1000)
Tuitest.wait_until_idle

Tuitest.close
verifier.finish

Kernel.system("rm -f cache cache.lock")

# EOF
