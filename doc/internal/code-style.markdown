Newsboat Code Style Guide
=========================

These are guidelines, not laws. You are free to deviate, but make sure you have
a good argument for doing so.



## Automation

Guidelines that could be easily automated, have been. Please install:

- [EditorConfig plugin][editorconfig], so your editor can apply some of the
  formatting right as you type;

- `clang-format` version 8, so you can format C++ code;

- `rustfmt` from the current stable Rust, so you can format Rust code:

        $ rustup default stable
        $ rustup update
        $ rustup component add rustfmt

You can then run `make fmt` to format C++ and Rust source.

If you can't or won't install `clang-format` and `rustfmt` on your system, you
can build a Docker image with those tools, and use that instead:

    $ docker build \
        --tag=newsboat-tools \
        --file=docker/newsboat-tools.dockerfile \
        docker
    $ docker run \
        --rm \
        --volume $(pwd):/workspace \
        newsboat-tools \
        make fmt

Note that you'll have to rebuild the image every time we update the Dockerfile,
which is every time new stable Rust comes out. See also [how to build Newsboat
in Docker](docker.md).

If Docker isn't your jam either, you can rely on our continuous integration
pipeline. Submit your pull request, and wait. One of the CI jobs checks
formatting, and if it fails, it'll print out a diff. You can copy that and
apply it as a patch.

[editorconfig]: http://editorconfig.org/ "EditorConfig"



## The Boy Scout Rule

Always leave the campground cleaner than you found it. In other words, if you
work on some part of the system and find some code that violates these
guidelines without a reason, make a separate commit that cleans up that code.

*Rationale*: it's impossible to automatically upgrade the whole codebase to
follow these guidelines, but doing it bit by bit is very feasible.



## Rust

Install `rustfmt` and run `cargo fmt` to format the code according to [Rust
style guide](https://github.com/rust-dev-tools/fmt-rfcs).



## C++


### Order of Includes

Put your `#include` statements in the following order:

1. *related header*, in double quotes (e.g. "foo.h" for "foo.cpp");
2. a blank line;
3. *headers from outside the repository*, in angle brackets (e.g. `<cassert>` or
   `<string>`);
4. a blank line;
5. *headers from inside the repository*, in double quotes.

*Rationale*: 

- putting related header in a prominent place makes it easier to navigate the
  code;
- visually distinguishing between outside and inside headers makes it easier to
  understand the dependencies and navigate the code.


### Namespaces

Use snake case, e.g. "newsboat_utils".


### Struct and class names

Use camel case, e.g. "FeedFetcher".

Treat abbreviations as single words, e.g. "HttpStatusCode", "OpmlReader",
"UrlReader".

For names of services and such, try to preserve their capitalization, e.g.
"Inoreader" not "InoReader".

Drop hyphens, e.g. a class dealing with TT-RSS service would be named
"TtRssReader".


### Method names

Use snake case, e.g. "do_the_work".


### File hierarchy

Header files are stored in "include" directory. Source files are in "src". Each
pair of files only describe one class. "Helper" `struct`s should be declared in
the same header as the class they're helping.

If a big group of classes are sufficiently isolated from the rest, they can be
stored in a separate directory and compiled into a static library. Examples:
"filter" and "rss" directories.
