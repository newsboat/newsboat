Newsboat Code Style Guide
=========================

These are guidelines, not laws. You are free to deviate, but make sure you have
a good argument for doing so.



## Automation

Guidelines that could be easily automated, have been. Please install:

- [EditorConfig plugin][editorconfig], so your editor can apply some of the
  formatting right as you type;

- [Artistic Style][astyle], so you can format C++ code;

- [rustfmt][rustfmt] from the current stable Rust, so you can format Rust code:

        $ rustup default stable
        $ rustup update
        $ rustup component add rustfmt

You can then run `make fmt` to format both C++ and Rust code.

Note: If you are running into issues running `make fmt` locally, there is a lightweight Docker container available you can use to format. Instructions are in the [Docker file](../../docker/code-formatting-tools.dockerfile).

[editorconfig]: https://editorconfig.org/ "EditorConfig"

[astyle]: http://astyle.sourceforge.net/ "Artistic Style"

[rustfmt]: https://github.com/rust-lang/rustfmt "rustfmt - GitHub"



## The Boy Scout Rule

Always leave the campground cleaner than you found it. In other words, if you
work on some part of the system and find some code that violates these
guidelines without a reason, make a separate commit that cleans up that code.

*Rationale*: it's impossible to automatically upgrade the whole codebase to
follow these guidelines, but doing it bit by bit is very feasible.



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

Use snake case, e.g. "Newsboat_utils".


### Struct and class names

Use camel case, e.g. "FeedFetcher".

Treat abbreviations as single words, e.g. "HttpStatusCode", "OpmlReader",
"UrlReader".

For names of services and such, try to preserve their capitalization, e.g.
"Inoreader" not "InoReader".

Drop hyphens, e.g. a class dealing with TT-RSS service would be named
"TtRssReader".


### Method/function names

Use snake case, e.g. "do_the_work".


### File hierarchy

Header files are stored in "include" directory. Source files are in "src". Each
pair of files only describe one class. "Helper" `struct`s should be declared in
the same header as the class they're helping.

If a big group of classes are sufficiently isolated from the rest, they can be
stored in a separate directory and compiled into a static library. Examples:
"filter" and "rss" directories.



## Rust


### Documentation tests (doctests)

Each doctest is linked as a separate binary, which is slow. Use them to
document the general API of the module, and mark them `no_run`. Do actual
testing with unit tests inside the `tests` sub-module.
