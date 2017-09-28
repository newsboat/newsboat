Newsboat [![Build Status](https://travis-ci.org/newsboat/newsboat.svg?branch=master)](https://travis-ci.org/newsboat/newsboat) [![Coverage Status](https://coveralls.io/repos/github/newsboat/newsboat/badge.svg?branch=master)](https://coveralls.io/github/newsboat/newsboat?branch=master)
========

Newsboat is a fork of Newsbeuter, an RSS/Atom feed reader for the text console.
The only difference is that Newsboat is actively maintained while Newsbeuter
isn't.

Downloading
-----------

You can download the latest version of Newsboat from the official site:
https://newsboat.org/

Alternatively, you can check out the latest version from the Git repository:

    git clone git://github.com/newsboat/newsboat.git

Dependencies
------------

Newsboat depends on a number of libraries, which need to be installed before
newsboat can be compiled.

- GCC 4.9 or newer, or Clang 3.6 or newer
- [STFL (version 0.21 or newer)](http://www.clifford.at/stfl/)
- [SQLite3 (version 3.5 or newer)](http://www.sqlite.org/download.html)
- [libcurl (version 7.18.0 or newer)](http://curl.haxx.se/download.html)
- GNU gettext (on systems that don't provide gettext in the libc):
  ftp://ftp.gnu.org/gnu/gettext/
- [pkg-config](http://pkg-config.freedesktop.org/wiki/)
- [libxml2](http://xmlsoft.org/downloads.html)
- [json-c (version 0.11 or newer)](https://github.com/json-c/json-c/wiki)

Installation
------------

First, you'll have to get the dependencies. Make sure to install the header
files for the libraries (on Debian and derivatives, headers are in `-dev`
packages, e.g. `libsqlite3-dev`.) After that, compiling and installing newsboat
is as simple as:

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

Support
-------

* Bugs and whatnot should be reported to the
  [issue tracker](https://github.com/newsboat/newsboat/issues)
* Drop us a line at
  [newsboat mailing list](http://groups.google.com/group/newsboat)
* Chat with developers and fellow users on #newsboat at
  [Freenode](https://freenode.net)

License
-------
Newsboat is licensed under the MIT/X Consortium License. See the file LICENSE
for further details.
