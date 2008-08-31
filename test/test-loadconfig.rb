#!/usr/bin/env ruby
# auto-generated tuitest script
require 'tuitest'

Kernel.system("rm -f cache")

Tuitest.init
verifier = Tuitest::Verifier.new("test-loadconfig.rb.log")


Tuitest.run("../newsbeuter -c cache -C config-example -u urls-tuitest1")


Tuitest.wait_until_idle

Tuitest.wait_until_idle
# begin auto-generated verification #1 
verifier.expect(0, 17, "Your feeds (0 unread, 1 total)")
verifier.expect(1, 3, "1         (0/0) http://testbed.newsbeuter.org/rss20.xml")
verifier.expect(23, 0, "q:Quit ENTER:Open n:Next Unread r:Reload R:Reload All A:Mark Read C:Catchup All")
# end auto-generated verification #1 

Tuitest.keypress("q"[0])

Tuitest.wait_until_idle

Tuitest.close
verifier.finish

Kernel.system("rm -f cache")

# EOF
