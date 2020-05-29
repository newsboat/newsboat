Newsboat [![Travis CI Build Status](https://travis-ci.com/newsboat/newsboat.svg?branch=master)](https://travis-ci.com/newsboat/newsboat) [![Cirrus CI Build Status](https://api.cirrus-ci.com/github/newsboat/newsboat.svg)](https://cirrus-ci.com/github/newsboat/newsboat) [![Coverage Status](https://coveralls.io/repos/github/newsboat/newsboat/badge.svg?branch=master)](https://coveralls.io/github/newsboat/newsboat?branch=master) [![CII Best Practices](https://bestpractices.coreinfrastructure.org/projects/3328/badge)](https://bestpractices.coreinfrastructure.org/projects/3328)
========

<a href="https://newsboat.org">
<img
    src="https://newsboat.org/logo.svg"
    alt="Newsboat logo"
    align="left"
    height="60"
    width="60"
    vspace="6" /></a>

Newsboat is an RSS/Atom feed reader for the text console. It's an actively
maintained fork of Newsbeuter.

Logo [by noobilanderi](https://groups.google.com/forum/#!topic/newsboat/Xm5pTsbeMEk),
licensed under [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/).

Downloading
-----------

You can download the latest version of Newsboat from the official site:
https://newsboat.org/

Alternatively, you can check out the latest version from the Git repository:

	$ git clone git://github.com/newsboat/newsboat.git

Dependencies
------------

Newsboat depends on a number of libraries, which need to be installed before
Newsboat can be compiled.

<!--
    UPDATE doc/newsboat.asciidoc IF YOU CHANGE THIS LIST
-->
- GCC 4.9 or newer, or Clang 3.6 or newer
- Stable [Rust](https://www.rust-lang.org/en-US/) and Cargo (Rust's package
    manager) (1.40.0 or newer; might work with older versions, but we don't
    check that)
- [STFL (version 0.21 or newer)](http://www.clifford.at/stfl/)
- [SQLite3 (version 3.5 or newer)](https://www.sqlite.org/download.html)
- [libcurl (version 7.21.6 or newer)](https://curl.haxx.se/download.html)
- Header files for the SSL library that libcurl uses. You can find out which
    library that is from the output of `curl --version`; most often that's
    OpenSSL, sometimes GnuTLS, or maybe something else.
- GNU gettext (on systems that don't provide gettext in the libc):
  ftp://ftp.gnu.org/gnu/gettext/
- [pkg-config](https://pkg-config.freedesktop.org/wiki/)
- [libxml2](http://xmlsoft.org/downloads.html)
- [json-c (version 0.11 or newer)](https://github.com/json-c/json-c/wiki)
- [Asciidoctor](https://asciidoctor.org/) (1.5.3 or newer)

Developers will also need:

- [xtr (version 0.1.4 or newer)](https://github.com/woboq/tr) (can be installed
    with `cargo install xtr`)
- [Coco/R for C++](http://www.ssw.uni-linz.ac.at/coco/), needed to re-generate
    filter language parser using `regenerate-parser` target.
<!--
    UPDATE doc/newsboat.asciidoc IF YOU CHANGE THIS LIST
-->

Installation
------------

<!--
    UPDATE doc/newsboat.asciidoc IF YOU CHANGE THIS LIST
-->

There are numerous ways:

- install from your distribution's repository ([a lot of distros have
    a package](https://repology.org/project/newsboat));

- install via [Snap](https://snapcraft.io/docs/installing-snapd):

        $ sudo snap install newsboat

- [build from source with Docker](doc/docker.md). Note that the resulting binary
    might not run outside of that same Docker container if your system doesn't
    have all the necessary libraries, or if their versions are too old;

- build from source in a chroot: to avoid polluting your system with developer
    packages, or to avoid upgrading, you might use a tool like
    [`debootstrap`](https://wiki.debian.org/Debootstrap) to create an isolated
    environment. Once that's done, just build from source as outlined in the
    next item;

- build from source.

    Install everything that's listed in the "Dependencies" section above. Make
    sure to install the header files as well (on Debian and derivatives, headers
    are in `-dev` packages, e.g. `libsqlite3-dev`.)

    Then compile and install with:

        $ make                   #  pass -jN to use N CPU cores, e.g. -j8
        $ sudo make install      #  install everything under /usr/local

    To install to a different directory, pass `prefix` like so: `sudo make
    prefix=/opt/newsboat install`.

    To uninstall, run `sudo make uninstall`.

Cross-compilers need to set `CARGO_BUILD_TARGET`; see [cargo
documentation](https://doc.rust-lang.org/cargo/reference/config.html#environment-variables).

<!--
    UPDATE doc/newsboat.asciidoc IF YOU CHANGE THIS LIST
-->

Support
-------

* Check out our
  [documentation](https://newsboat.org/releases/2.19/docs/newsboat.html) and
  [FAQ](https://newsboat.org/releases/2.19/docs/faq.html)
* Bugs and whatnot should be reported to the
  [issue tracker](https://github.com/newsboat/newsboat/issues)
* Drop us a line at
  [newsboat mailing list](https://groups.google.com/group/newsboat)
  (newsboat@googlegroups.com)
* Chat with developers and fellow users on #newsboat at
  [Freenode](https://freenode.net) ([webchat
  available!](https://webchat.freenode.net/?channels=newsboat))

Development
-----------

Decided to work on an issue, fix a bug or add a feature? Great! Be sure to
check out [our style guide](doc/internal/code-style.markdown).

You'll probably want to run the tests; here's how:

	$ make -j5 PROFILE=1 all test  # 5 is CPU cores + 1, to parallelize the build
	$ (cd test && TMPDIR=/dev/shm ./test --order rand) && cargo test

Note the use of ramdisk as `TMPDIR`: some tests create temporary files, which
slows them down if `TMPDIR` is on HDD or even SSD.

Newsboat can also be [built in Docker](doc/docker.md).

License
-------

Newsboat is licensed under [the MIT
license](https://opensource.org/licenses/MIT); see the LICENSE file.
