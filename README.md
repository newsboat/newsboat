Newsbeuter [![Build Status](https://travis-ci.org/akrennmair/newsbeuter.svg?branch=master)](https://travis-ci.org/akrennmair/newsbeuter) [![Coverage Status](https://coveralls.io/repos/github/akrennmair/newsbeuter/badge.svg?branch=master)](https://coveralls.io/github/akrennmair/newsbeuter?branch=master)
=====================
by Andreas Krennmair <ak@newsbeuter.org>

<a href="http://newsbeuter.org">
<img
    src="https://newsbeuter.files.wordpress.com/2008/04/newsbeuter_640x640.png"
    align="left" height="80" width="80" vspace="6" /></a>

Newsbeuter is an RSS feed reader for the text console. Zed Shaw
[called](http://zedshaw.com/archive/i-want-the-mutt-of-feed-readers/) it "The
Mutt of Feed Readers".

It is designed to run on Unix-like operating systems such as GNU/Linux and
FreeBSD. NetBSD is currently not supported, due to technical limitations.

Downloading
-----------

You can download the latest version of newsbeuter from the following website:
http://www.newsbeuter.org/

Alternatively, you can check out the latest version from the newsbeuter
Git repository (hosted on GitHub):

    git clone git://github.com/akrennmair/newsbeuter.git

Dependencies
------------

Newsbeuter depends on a number of libraries, which need to be installed before
newsbeuter can be compiled.

- GCC 4.9 or newer, or Clang 3.6 or newer
- [STFL (version 0.21 or newer)](http://www.clifford.at/stfl/)
- [SQLite3 (version 3.5 or newer)](http://www.sqlite.org/download.html)
- [libcurl (version 7.18.0 or newer)](http://curl.haxx.se/download.html)
- GNU gettext (on systems that don't provide gettext in the libc):
  ftp://ftp.gnu.org/gnu/gettext/
- [pkg-config](http://pkg-config.freedesktop.org/wiki/)
- [libxml2](http://xmlsoft.org/downloads.html)
- [json-c (version 0.11 or newer)](https://github.com/json-c/json-c/wiki)

Debian unstable comes with ready-to-use packages for these dependencies.

Installation
------------
Compiling and installing newsbeuter is as simple as:

	make
	make install

(And if you ever need to uninstall it, use `make uninstall`.)

Tests
-----

If you're a developer, here's how you can run the test suite:

	make -j5 PROFILE=1 all test
	(cd test && TMPDIR=/dev/shm ./test --order rand)

Note the use of ramdisk as `TMPDIR`: some of our tests require temporary files,
which degrades the performance quite a bit if `TMPDIR` isn't in-memory.

Contact
-------
Andreas Krennmair <ak@newsbeuter.org>

Support
-------

* Bugs and whatnot should be reported to the
  [issue tracker](https://github.com/akrennmair/newsbeuter/issues)
* Drop us a line at
  [Newsbeuter mailing list](http://groups.google.com/group/newsbeuter)
* Chat with developers and fellow users on #newsbeuter at
  [Freenode](https://freenode.net)

License
-------
Newsbeuter is licensed under the MIT/X Consortium License. See the file LICENSE
for further details.
