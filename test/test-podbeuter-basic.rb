#!/usr/bin/env ruby
# auto-generated tuitest script
require 'tuitest'

Tuitest.init
verifier = Tuitest::Verifier.new("test-podbeuter-basic.rb.log", "RESULT-test-podbeuter-basic.rb.xml")


Tuitest.run("../podbeuter -C config-tuitest2 -q queue")


Tuitest.wait_until_idle

Tuitest.wait_until_idle

# begin auto-generated verification #1 
verifier.expect(0, 0, "Queue (0 downloads in progress, 1 total) - 0.00 kb/s total")
verifier.expect(1, 4, "1 [   0.0MB/   0.0MB] [  0.0 %] [   0.00 kb/s] incomplete")
verifier.expect(23, 0, "q:Quit d:Download c:Cancel D:Delete P:Purge Finished a:Toggle Automatic Download")
# end auto-generated verification #1 

Tuitest.keypress("?"[0])

Tuitest.wait_until_idle

# begin auto-generated verification #2 
verifier.expect(0, 0, "Help                                                      ")
verifier.expect(1, 0, "ENTER   open                    Open feed/article                               ")
verifier.expect(2, 0, "q       quit                    Return to previous dialog/Quit")
verifier.expect(3, 0, "Q       hard-quit               Quit program,  no confirmation")
verifier.expect(4, 0, "?       help                    Open help dialog")
verifier.expect(5, 0, "d       pb-download             Download file")
verifier.expect(6, 0, "c       pb-cancel               Cancel download")
verifier.expect(7, 0, "D       pb-delete               Mark download as deleted")
verifier.expect(8, 0, "P       pb-purge                Purge finished and deleted downloads from queue")
verifier.expect(9, 0, "a       pb-toggle-download-all  Toggle automatic download on/off")
verifier.expect(10, 0, "p       pb-play                 Start player with currently selected download")
verifier.expect(11, 0, "+       pb-increase-max-dls     Increase the number of concurrent downloads")
verifier.expect(12, 0, "-       pb-decreate-max-dls     Decrease the number of concurrent downloads")
verifier.expect(13, 0, "~")
# end auto-generated verification #2 

Tuitest.keypress("q"[0])
Tuitest.keypress("q"[0])

Tuitest.wait_until_idle

Tuitest.close
verifier.finish

# EOF
