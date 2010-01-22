#!/usr/bin/env ruby
# script to test HTML rendering.
require 'tuitest'

Kernel.system("rm -f cache cache.lock")

Tuitest.init
verifier = Tuitest::Verifier.new("test-html.rb.log", "RESULT-test-html.rb.xml")


if ENV["OFFLINE"] then
	Tuitest.run("../newsbeuter -c cache -C /dev/null -u urls-html-offline")
else
	Tuitest.run("../newsbeuter -c cache -C /dev/null -u urls-html")
end


Tuitest.wait_until_expected_text(0, 0, "newsbeuter ", 5000)
Tuitest.keypress("r"[0])
Tuitest.keypress(10)
Tuitest.keypress(10)

Tuitest.wait_until_expected_text(0, 24, " 'HTML Item 1'                                          ", 5000)
# begin auto-generated verification #1 
verifier.expect(0, 24, " 'HTML Item 1'                                          ")
verifier.expect(1, 0, "Feed: HTML rendering testbed feed")
verifier.expect(2, 0, "Title: HTML Item 1          ")
verifier.expect(3, 0, "Author: Andreas Krennmair")
verifier.expect(4, 0, "Link: http://testbed.newsbeuter.org/htmlitem1.html")
verifier.expect(5, 0, "Date: Sat, 30 Aug 2008 09:40:10")
verifier.expect(7, 0, "normal link[1]")
verifier.expect(8, 0, "invalid link without href.")
verifier.expect(9, 0, "[embedded flash: 2]")
verifier.expect(10, 0, "hello world")
verifier.expect(11, 0, "hello world")
verifier.expect(12, 0, "hello world")
verifier.expect(13, 1, "-------------------------------------------------------------------------")
verifier.expect(14, 0, "[image 3]")
verifier.expect(15, 1, "-------------------------------------------------------------------------")
verifier.expect(17, 2, "To be or not to be, that is the question.")
verifier.expect(20, 0, "Links:")
verifier.expect(21, 0, "[1]: http://www.newsbeuter.org/ (link)")
verifier.expect(22, 0, "[2]: http://www.youtube.com/v/BLjDXxjkrWc (embedded flash)")
verifier.expect(23, 14, "n:Next Unread o:Open in Browser e:Enqueue ?:Help      ")
# end auto-generated verification #1 

Tuitest.keypress("n"[0])

Tuitest.wait_until_expected_text(0, 36, "2", 5000)
# begin auto-generated verification #2 
verifier.expect(0, 36, "2")
verifier.expect(2, 17, "2")
verifier.expect(4, 44, "2")
verifier.expect(5, 6, "Fri, 29")
verifier.expect(7, 0, "Lorem ipsum dolor sit amet, consectetuer sadipscing elitr, sed diam nonumy")
verifier.expect(8, 0, "eirmod tempor invidunt ut labore et dolore magna aliquyam erat, sed diam")
verifier.expect(9, 0, "voluptua.          ")
verifier.expect(10, 0, "           ")
verifier.expect(11, 0, "At vero eos et accusam et justo duo dolores et ea rebum. Stet clita kasd")
verifier.expect(12, 0, "gubergren, no sea takimata sanctus est Lorem ipsum dolor sit amet. Lorem")
verifier.expect(13, 0, "ipsum dolor sit amet, consetetur sadipscing elitr, ...                    ")
verifier.expect(14, 0, "~        ")
verifier.expect(15, 0, "~                                                                         ")
verifier.expect(16, 0, "~")
verifier.expect(17, 0, "~                                          ")
verifier.expect(18, 0, "~")
verifier.expect(19, 0, "~")
verifier.expect(20, 0, "~     ")
verifier.expect(21, 0, "~                                     ")
verifier.expect(22, 0, "~                                                         ")
# end auto-generated verification #2 

Tuitest.keypress("n"[0])

Tuitest.wait_until_expected_text(0, 36, "3", 5000)
# begin auto-generated verification #3 
verifier.expect(0, 36, "3")
verifier.expect(2, 17, "3")
verifier.expect(4, 44, "4")
verifier.expect(5, 6, "Thu, 28")
verifier.expect(7, 0, "                                                                          ")
verifier.expect(8, 0, " 1.one                                                                  ")
verifier.expect(9, 0, " 2.two   ")
verifier.expect(10, 1, "3.three")
verifier.expect(11, 0, "                                                                        ")
verifier.expect(12, 0, " -------------------------------------------------------------------------")
verifier.expect(13, 0, "                                                      ")
verifier.expect(14, 0, "  * hello")
verifier.expect(15, 0, "  * world")
verifier.expect(16, 0, " ")
verifier.expect(17, 0, " -------------------------------------------------------------------------")
verifier.expect(18, 0, "a[0] = 23")
verifier.expect(19, 0, "a[1] = 42")
verifier.expect(20, 0, "E = mc^2")
verifier.expect(21, 0, " -------------------------------------------------------------------------")
# end auto-generated verification #3 

Tuitest.keypress("q"[0])
Tuitest.keypress("q"[0])
Tuitest.keypress("q"[0])

Tuitest.wait_until_idle

Tuitest.close
verifier.finish

Kernel.system("rm -f cache cache.lock")

# EOF
