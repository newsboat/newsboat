Newsboat [![Cirrus CI Build Status](https://api.cirrus-ci.com/github/newsboat/newsboat.svg)](https://cirrus-ci.com/github/newsboat/newsboat) [![GitHub Actions: Coveralls status](https://github.com/newsboat/newsboat/workflows/Coveralls/badge.svg)](https://github.com/newsboat/newsboat/actions?query=workflow%3ACoveralls) [![Coverage Status](https://coveralls.io/repos/github/newsboat/newsboat/badge.svg?branch=master)](https://coveralls.io/github/newsboat/newsboat?branch=master) [![CII Best Practices](https://bestpractices.coreinfrastructure.org/projects/3328/badge)](https://bestpractices.coreinfrastructure.org/projects/3328)
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

A feed reader pulls updates directly from sites like blogs and news agencies,
and lets you review them in a single interface. Many times, the feed includes
the full text of the update, so you don't even need to start a web browser! You
can learn more about feed readers [on
Wikipedia](https://en.wikipedia.org/wiki/News_aggregator).

<img
    style="display: block; margin-left: auto; margin-right: auto;"
    src="https://newsboat.org/images/2.25-screenshot_2x-33f26153.png"
    alt="Viewing an article in Newsboat"
    />

Notable features
----------------

* Powerful built-in HTML renderer — no need to start the web browser to view
    text-only entries
* Send links and whole articles to third-party services using [bookmarking
    scripts](https://newsboat.org/releases/2.42/docs/newsboat.html#_bookmarking)
* [Filter articles out](https://newsboat.org/releases/2.42/docs/newsboat.html#_killfiles)
    based on title, author, contents etc.
* [Aggregate articles](https://newsboat.org/releases/2.42/docs/newsboat.html#_query_feeds)
    into meta-feeds by arbitrary criteria
* [Apply transformations](https://newsboat.org/releases/2.42/docs/newsboat.html#_scripts_and_filters_snownews_extensions)
    to feeds before passing them into Newsboat
* Integrates with services like The Old Reader, NewsBlur, FeedHQ
    and [many more](https://newsboat.org/releases/2.42/docs/newsboat.html#_newsboat_as_a_client_for_newsreading_services)
* [Macros](https://newsboat.org/releases/2.42/docs/newsboat.html#_macro_support)
    to execute sequences of actions with just two keystrokes
* Rudimentary [podcast support](https://newsboat.org/releases/2.42/docs/newsboat.html#_podcast_support)

Downloading
-----------

You can download the latest version of Newsboat from the official site:
https://newsboat.org/

Alternatively, you can check out the latest version from the Git repository:

	$ git clone https://github.com/newsboat/newsboat.git

Dependencies
------------

Newsboat depends on a number of libraries, which need to be installed before
Newsboat can be compiled.

<!--
    UPDATE doc/newsboat.asciidoc IF YOU CHANGE THIS LIST
-->
- GNU Make 4.0 or newer
- C++17 compiler: GCC 7.0 or newer, or Clang 5 or newer
- Stable [Rust](https://www.rust-lang.org/en-US/) and Cargo (Rust's package
    manager) (1.88.0 or newer; might work with older versions, but we don't
    check that)
- [STFL (version 0.21 or newer)](https://github.com/newsboat/stfl) (the link
    points to our own fork because [the upstream](http://www.clifford.at/stfl/)
    is dead)
- [SQLite3 (version 3.5 or newer)](https://www.sqlite.org/download.html)
- [libcurl (version 7.32.0 or newer)](https://curl.haxx.se/download.html)
- Header files for the SSL library that libcurl uses. You can find out which
    library that is from the output of `curl --version`; most often that's
    OpenSSL, sometimes GnuTLS, or maybe something else.
- GNU gettext (on systems that don't provide gettext in the libc):
  ftp://ftp.gnu.org/gnu/gettext/
- [pkg-config](https://pkg-config.freedesktop.org/wiki/)
- [libxml2](http://xmlsoft.org/downloads.html)
- [json-c (version 0.11 or newer)](https://github.com/json-c/json-c/wiki)
- [Asciidoctor](https://asciidoctor.org/) (1.5.3 or newer)
- Some implementation of AWK like [GNU AWK](https://www.gnu.org/software/gawk) or [NAWK](https://github.com/onetrueawk/awk).

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

  Our snap [only supports `xdg-open` as the browser][snap-browser], and you
  can't run arbitrary scripts for rendering and bookmarking. The reason is
  strict confinement; if we disabled it, the snap would be no better than
  a distribution's package;

  [snap-browser]: https://newsboat.org/releases/2.42/docs/faq.html#_with_snap_i_cant_start_browserbookmarking_scriptexecfilterrun_program_from_macro

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

    Cross-compilers need to set `CARGO_BUILD_TARGET`; see [cargo
    documentation](https://doc.rust-lang.org/cargo/reference/config.html#environment-variables).

    Then compile and install with:

      $ make                   #  pass -jN to use N CPU cores, e.g. -j8
      $ sudo make install      #  install everything under /usr/local

    To install to a different directory, pass `prefix` like so: `sudo make
    prefix=/opt/newsboat install`.

    To uninstall, run `sudo make uninstall`.

<!--
    UPDATE doc/newsboat.asciidoc IF YOU CHANGE THIS LIST
-->

Support
-------

* Check out our
  [documentation](https://newsboat.org/releases/2.42/docs/newsboat.html) and
  [FAQ](https://newsboat.org/releases/2.42/docs/faq.html)
* Report security vulnerabilities to security@newsboat.org. Please encrypt your emails to
  [OpenPGP key 4ED6CD61932B9EBE](https://newsboat.org/newsboat.pgp) if you can.
* Report bugs and ask questions on
  [the issue tracker](https://github.com/newsboat/newsboat/issues) and
  [the mailing list](https://groups.google.com/group/newsboat)
  (newsboat@googlegroups.com)
* Chat with developers and fellow users on #newsboat at
  [irc.libera.chat](https://libera.chat) (also accessible [via
  webchat](https://web.libera.chat/) and [via
  Matrix](https://matrix.to/#/#newsboat:libera.chat)). We *do not* have
  a channel on Freenode anymore.

Contributing
------------

See [CONTRIBUTING.md](CONTRIBUTING.md)

License
-------

Newsboat is licensed under [the MIT
license](https://opensource.org/licenses/MIT); see the LICENSE file. Logo [by
noobilanderi](https://groups.google.com/forum/#!topic/newsboat/Xm5pTsbeMEk),
licensed under [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/).
