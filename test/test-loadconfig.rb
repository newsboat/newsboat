#!/usr/bin/env ruby
# auto-generated tuitest script
require 'tuitest'

Kernel.system("rm -f cache cache.lock")

Tuitest.init
verifier = Tuitest::Verifier.new("test-loadconfig.rb.log", "RESULT-test-loadconfig.rb.xml")


if ENV["OFFLINE"] then
	Tuitest.run("../newsbeuter -c cache -C config-example -u urls-tuitest1-offline")
else
	Tuitest.run("../newsbeuter -c cache -C config-example -u urls-tuitest1")
end


Tuitest.wait_until_expected_text(0, 17, "Your feeds (0 unread, 1 total)", 5000)
# begin auto-generated verification #1 
verifier.expect(0, 17, "Your feeds (0 unread, 1 total)")
if ENV["OFFLINE"] then
	verifier.expect(1, 3, "1         (0/0) file://./testbed/rss20.xml")
else
	verifier.expect(1, 3, "1         (0/0) http://testbed.newsbeuter.org/rss20.xml")
end

verifier.expect(23, 0, "q:Quit ENTER:Open n:Next Unread r:Reload R:Reload All A:Mark Read C:Catchup All")
# end auto-generated verification #1 

Tuitest.keypress("q"[0])

Tuitest.wait_until_idle

Tuitest.close
verifier.finish

Kernel.system("rm -f cache cache.lock")

# EOF
