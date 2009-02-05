#!/usr/bin/env ruby
# auto-generated tuitest script
require 'tuitest'

Kernel.system("rm -f cache cache.lock")

Tuitest.init
verifier = Tuitest::Verifier.new("test-openalldialog.rb.log", "RESULT-test-openalldialog.rb.xml")


if ENV["OFFLINE"] then
	Tuitest.run("../newsbeuter -c cache -C /dev/null -u urls-tuitest1-offline")
else
	Tuitest.run("../newsbeuter -c cache -C /dev/null -u urls-tuitest1")
end

Tuitest.wait_until_expected_text(0, 17, "Your feeds (0 unread, 1 total)", 5000)
verifier.expect(0, 17, "Your feeds (0 unread, 1 total)")

Tuitest.keypress("r"[0])
Tuitest.keypress(10)

Tuitest.wait_until_expected_text(0, 17, "Articles in feed 'RSS 2.0 testbed feed' (3 unread, 3 total)", 5000)
verifier.expect(0, 17, "Articles in feed 'RSS 2.0 testbed feed' (3 unread, 3 total)")

Tuitest.keypress(258)
Tuitest.keypress(258)
Tuitest.keypress(10)

Tuitest.wait_until_expected_text(0, 17, "Article ", 5000)
verifier.expect(0, 17, "Article ")

Tuitest.keypress("?"[0])

Tuitest.wait_until_expected_text(0, 17, "Help                    ", 5000)
verifier.expect(0, 17, "Help                    ")

Tuitest.keypress("q"[0])
Tuitest.keypress("u"[0])

Tuitest.wait_until_expected_text(0, 17, "URLs")
verifier.expect(0, 17, "URLs")

Tuitest.keypress("q"[0])
Tuitest.keypress("u"[0])
Tuitest.keypress("q"[0])
Tuitest.keypress("s"[0])

# begin auto-generated verification #6 
Tuitest.wait_until_expected_text(0, 17, "Save File - ", 5000)
verifier.expect(0, 17, "Save File - ")

Tuitest.keypress(259)
Tuitest.keypress("q"[0])
Tuitest.keypress("q"[0])
Tuitest.keypress("q"[0])
Tuitest.keypress("t"[0])

Tuitest.wait_until_expected_text(0, 17, "Select Tag", 5000)
verifier.expect(0, 17, "Select Tag")

Tuitest.keypress("q"[0])
Tuitest.keypress("q"[0])
Tuitest.wait_until_idle

Tuitest.close
verifier.finish

Kernel.system("rm -f cache cache.lock")

# EOF
