Newsboat [![Build Status](https://travis-ci.org/newsboat/newsboat.svg?branch=master)](https://travis-ci.org/newsboat/newsboat) [![Coverage Status](https://coveralls.io/repos/github/newsboat/newsboat/badge.svg?branch=master)](https://coveralls.io/github/newsboat/newsboat?branch=master) [![Coverity Scan Build Status](https://scan.coverity.com/projects/15567/badge.svg)](https://scan.coverity.com/projects/newsboat-newsboat)
========

Newsboat is a fork of Newsbeuter, an RSS/Atom feed reader for the text console.
The only difference is that Newsboat is actively maintained while Newsbeuter
isn't.

Downloading
-----------

You can download the latest version of Newsboat from the official site:
https://newsboat.org/

Alternatively, you can check out the latest version from the Git repository:

	$ git clone git://github.com/newsboat/newsboat.git

Dependencies
------------

Newsboat depends on a number of libraries, which need to be installed before
newsboat can be compiled.

- GCC 4.9 or newer, or Clang 3.6 or newer
- Stable [Rust](https://www.rust-lang.org/en-US/) (1.25.0 or newer)
- [STFL (version 0.21 or newer)](http://www.clifford.at/stfl/)
- [SQLite3 (version 3.5 or newer)](http://www.sqlite.org/download.html)
- [libcurl (version 7.21.6 or newer)](http://curl.haxx.se/download.html)
- GNU gettext (on systems that don't provide gettext in the libc):
  ftp://ftp.gnu.org/gnu/gettext/
- [pkg-config](http://pkg-config.freedesktop.org/wiki/)
- [libxml2, xmllint, and xsltproc](http://xmlsoft.org/downloads.html)
- [json-c (version 0.11 or newer)](https://github.com/json-c/json-c/wiki)
- [asciidoc](http://www.methods.co.nz/asciidoc/INSTALL.html)
- DocBook XML
- DocBook XSL

Installation
------------

First, you'll have to get the dependencies. Make sure to install the header
files for the libraries (on Debian and derivatives, headers are in `-dev`
packages, e.g. `libsqlite3-dev`.) After that, compiling and installing newsboat
is as simple as:

	$ make
	$ sudo make install

And if you ever need to uninstall it, use `make uninstall`.

Cross-compilers need to set `CARGO_BUILD_TARGET`; see [cargo
documentation](https://doc.rust-lang.org/cargo/reference/config.html#environment-variables).

Support
-------

* Check out our
  [documentation](https://newsboat.org/releases/2.15/docs/newsboat.html) and
  [FAQ](https://newsboat.org/releases/2.15/docs/faq.html)
* Bugs and whatnot should be reported to the
  [issue tracker](https://github.com/newsboat/newsboat/issues)
* Drop us a line at
  [newsboat mailing list](http://groups.google.com/group/newsboat)
* Chat with developers and fellow users on #newsboat at
  [Freenode](https://freenode.net) ([webchat
  available!](https://webchat.freenode.net/?channels=newsboat))

Development
-----------

Decided to work on an issue, fix a bug or add a feature? Great! Be sure to
check out [our style guide](doc/code-style.markdown).

You'll probably want to run the tests; here's how:

	$ make -j5 PROFILE=1 all test  # 5 is CPU cores + 1, to parallelize the build
	$ (cd test && TMPDIR=/dev/shm ./test --order rand) && cargo test

Note the use of ramdisk as `TMPDIR`: some tests create temporary files, which
slows them down if `TMPDIR` is on HDD or even SSD.

We check the formatting of the Rust code during CI using
[rust-fmt](https://github.com/rust-lang/rustfmt).  To make sure your code is
properly formated install and run rust-fmt:

	$ rustup component add rustfmt
	$ cargo fmt

License
-------

Newsboat is licensed under [the MIT
license](https://opensource.org/licenses/MIT); see the LICENSE file.
