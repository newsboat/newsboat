# basic operations test:
# - reload
# - mark feed read
# - mark articles read/unread
# - some HTML rendering verifications
# - delete article
# - search prompt
# - bookmarking (unconfigured), i.e. prompts and error messages
# - select filter with no filter defined, i.e error message

require 'tuitest'

Tuitest.init
verifier = Tuitest::Verifier.new("test-basic-ops.rb.log", "RESULT-test-basic-ops.rb.xml")

Kernel.system("rm -f cache")

Tuitest.run("../newsbeuter -c cache -C /dev/null -u urls-tuitest1")

Tuitest.wait_until_idle

Tuitest.keypress("r"[0])
Tuitest.keypress(10)
Tuitest.keypress("A"[0])

Tuitest.wait_until_expected_text(0, 58, "0", 5000)
verifier.expect(0, 58, "0")
verifier.expect(1, 5, " ")
verifier.expect(2, 5, " ")
verifier.expect(3, 5, " ")

Tuitest.keypress("q"[0])
Tuitest.keypress(10)
Tuitest.keypress("N"[0])
Tuitest.keypress("N"[0])
Tuitest.keypress("N"[0])

Tuitest.wait_until_expected_text(0, 58, "3", 5000)
verifier.expect(0, 58, "3")
verifier.expect(1, 5, "N")
verifier.expect(2, 5, "N")
verifier.expect(3, 5, "N")

Tuitest.keypress(259)
Tuitest.keypress(259)
Tuitest.keypress(10)
Tuitest.keypress(12)
Tuitest.keypress("n"[0])

Tuitest.wait_until_idle
Tuitest.wait_until_expected_text(0, 39, "2", 5000)
verifier.expect(0, 39, "2")
verifier.expect(2, 20, "2")
verifier.expect(4, 40, "2")
verifier.expect(5, 6, "Fri, 29 Aug 2008 08:41:3")
verifier.expect(7, 9, "econd item, this time only with a <description> tag.")
verifier.expect(8, 0, "~")
verifier.expect(9, 0, "~                   ")
verifier.expect(10, 0, "~                                              ")
verifier.expect(11, 0, "~")
verifier.expect(12, 0, "~              ")
verifier.expect(13, 0, "~               ")
verifier.expect(14, 0, "~              ")
verifier.expect(15, 0, "~")

Tuitest.keypress("n"[0])

Tuitest.wait_until_expected_text(0, 39, "3", 5000)
verifier.expect(0, 39, "3")
verifier.expect(2, 20, "3")
verifier.expect(4, 40, "3")
verifier.expect(5, 6, "Thu, 28 Aug 2008 18:27:5")
verifier.expect(7, 0, "And finally a third item, also description, but with some HTML...")
verifier.expect(8, 0, "Yes, there was a line break. And here is a [link to slashdot][1]")
verifier.expect(9, 0, " ")
verifier.expect(10, 0, "Links:")
verifier.expect(11, 0, "[1]: http://slashdot.org/ (link)")

Tuitest.keypress("n"[0])

Tuitest.wait_until_expected_text(0, 24, "s in feed 'RSS 2.0 testbed feed' (0 unread, 3 total) - h")
verifier.expect(0, 24, "s in feed 'RSS 2.0 testbed feed' (0 unread, 3 total) - h")
verifier.expect(1, 0, "   1    Aug 30   RSS 2.0 Item 1")
verifier.expect(2, 0, "   2    Aug 29   RSS 2.0 Item 2")
verifier.expect(3, 0, "   3    Aug 28   RSS 2.0 Item 3              ")
verifier.expect(4, 0, "                                              ")
verifier.expect(5, 0, "                               ")
verifier.expect(7, 0, "                                                                 ")
verifier.expect(8, 0, "                                                               ")
verifier.expect(10, 0, "      ")
verifier.expect(11, 0, "                                ")
verifier.expect(12, 0, " ")
verifier.expect(13, 0, " ")
verifier.expect(14, 0, " ")
verifier.expect(15, 0, " ")
verifier.expect(16, 0, " ")
verifier.expect(17, 0, " ")
verifier.expect(18, 0, " ")
verifier.expect(19, 0, " ")
verifier.expect(20, 0, " ")
verifier.expect(21, 0, " ")
verifier.expect(22, 0, " ")
verifier.expect(23, 25, "r:Reload n:Next Unread A:Mark All Read /:Search ?:Help")
verifier.expect(24, 0, "No unread items.")

Tuitest.keypress("q"[0])
Tuitest.keypress(10)
Tuitest.keypress(258)
Tuitest.keypress(258)
Tuitest.keypress("d"[0])
Tuitest.keypress("D"[0])
Tuitest.keypress("$"[0])

Tuitest.wait_until_expected_text(0, 68, "2", 5000)
verifier.expect(0, 68, "2")
verifier.expect(3, 3, "                            ")

Tuitest.keypress("/"[0])

Tuitest.wait_until_expected_text(24, 0, "Search for:", 5000)
verifier.expect(24, 0, "Search for:")

Tuitest.keypress(10)

Tuitest.wait_until_expected_text(24, 0, "           ", 5000)
verifier.expect(24, 0, "           ")

Tuitest.keypress("f"[0])

Tuitest.wait_until_expected_text(24, 0, "No filters defined.", 5000)
verifier.expect(24, 0, "No filters defined.")

Tuitest.keypress(2)

Tuitest.wait_until_expected_text(24, 0, "URL: http://testbed.newsbeuter.org/item2.html", 5000)
verifier.expect(24, 0, "URL: http://testbed.newsbeuter.org/item2.html")

Tuitest.keypress(10)

Tuitest.wait_until_expected_text(24, 0, "Title: RSS 2.0 Item 2", 5000)
verifier.expect(24, 0, "Title: RSS 2.0 Item 2                        ")

Tuitest.keypress(10)

Tuitest.wait_until_expected_text(24, 0, "Description:", 5000)
verifier.expect(24, 0, "Description:         ")

Tuitest.keypress(10)

Tuitest.wait_until_expected_text(24, 0, "Error while saving bookmark: bookmarking support is not configured. Please set t", 5000)
verifier.expect(24, 0, "Error while saving bookmark: bookmarking support is not configured. Please set t")

Tuitest.keypress(259)
Tuitest.keypress(10)
Tuitest.keypress("u"[0])
Tuitest.keypress("q"[0])
Tuitest.keypress("q"[0])
Tuitest.keypress("q"[0])
Tuitest.wait_until_idle

Tuitest.close
verifier.finish

Kernel.system("rm -f cache")

# EOF
