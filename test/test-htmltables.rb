#!/usr/bin/env ruby
# auto-generated tuitest script
require 'tuitest'

Tuitest.init
verifier = Tuitest::Verifier.new("test-htmltables.rb.log", "RESULT-test-htmltables.rb.xml")


Tuitest.run("../newsbeuter -c cache -C /dev/null -u urls-tables")


Tuitest.wait_until_idle
Tuitest.keypress("r"[0])
Tuitest.keypress(10)
Tuitest.keypress(10)

Tuitest.wait_until_idle

verifier.expect(6, 0, "+--------+--------+--------+")
verifier.expect(7, 0, "|Column 1|Column 2|Column 3|")
verifier.expect(8, 0, "+--------+--------+--------+")
verifier.expect(9, 0, "|data 1  |data 2  |data 3  |")
verifier.expect(10, 0, "+--------+--------+--------+")
verifier.expect(11, 0, "|data 1-2         |data 3  |")
verifier.expect(12, 0, "+--------+--------+--------+")
verifier.expect(13, 0, "|data 1  |data 2-3         |")
verifier.expect(14, 0, "+--------+--------+--------+")
verifier.expect(15, 0, "|data 1-3                  |")
verifier.expect(16, 0, "+--------+--------+--------+")

Tuitest.keypress("q"[0])
Tuitest.keypress("q"[0])
Tuitest.keypress("q"[0])

Tuitest.wait_until_idle

Tuitest.close
verifier.finish

Kernel.system("rm -f cache cache.lock")

# EOF
