#!/usr/bin/env ruby
# script to test pre-defined filters
require 'tuitest'

Kernel.system("rm -f cache")

Tuitest.init
verifier = Tuitest::Verifier.new("test-filters.rb.log")


Tuitest.run("../newsbeuter -c cache -C config-filter -u urls-tuitest1")


Tuitest.wait_until_expected_text(0, 0, "newsbeuter ", 5000)
Tuitest.keypress("r"[0])
Tuitest.keypress("f"[0])
Tuitest.keypress(10)

Tuitest.wait_until_expected_text(24, 0, "Error: applying the filter failed: attribute `unread' is not available.", 5000)
# begin auto-generated verification #1 
verifier.expect(24, 0, "Error: applying the filter failed: attribute `unread' is not available.")
# end auto-generated verification #1 

Tuitest.keypress(10)
Tuitest.keypress(258)
Tuitest.keypress(258)
Tuitest.keypress("f"[0])
Tuitest.keypress(10)

Tuitest.keypress("N"[0])

Tuitest.wait_until_expected_text(0, 58, "2", 5000)
# begin auto-generated verification #3 
verifier.expect(0, 58, "2")
verifier.expect(3, 5, " ")
# end auto-generated verification #3 

Tuitest.keypress("f"[0])
Tuitest.keypress(10)

Tuitest.wait_until_expected_text(3, 3, "                            ", 5000)
# begin auto-generated verification #4 
verifier.expect(3, 3, "                            ")
# end auto-generated verification #4 

Tuitest.keypress("q"[0])
Tuitest.keypress("q"[0])

Tuitest.wait_until_idle

Tuitest.close
verifier.finish

Kernel.system("rm -f cache")

# EOF
