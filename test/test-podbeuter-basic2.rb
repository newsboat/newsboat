#!/usr/bin/env ruby
# auto-generated tuitest script
require 'tuitest'

Tuitest.init
verifier = Tuitest::Verifier.new("test-podbeuter-basic2.rb.log", "RESULT-test-podbeuter-basic2.rb.xml")


Tuitest.run("../podbeuter -C config-example -q queue")


Tuitest.wait_until_idle

Tuitest.wait_until_idle

# begin auto-generated verification #1 
verifier.expect(0, 0, "Queue (0 downloads in progress, 1 total) - 0.00 kb/s total")
verifier.expect(1, 4, "1 [   0.0MB/   0.0MB] [  0.0 %] [   0.00 kb/s] queued")
# end auto-generated verification #1 

Tuitest.keypress("q"[0])

Tuitest.wait_until_idle

Tuitest.close
verifier.finish

# EOF
