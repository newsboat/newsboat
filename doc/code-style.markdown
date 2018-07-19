Newsboat Code Style Guide
=========================

These are guidelines, not laws. You are free to deviate, but make sure you have
a good argument for doing so.

Guidelines that could be easily automated, have been. Please install:
- [EditorConfig plugin][editorconfig], so your editor can apply some of the
  formatting right as you type;
- `clang-format` (part of `clang` package in most Linux distributions);
- our pre-commit hook, which runs `clang-format` on each commit you make. Just
  run:

    ```shell
    $ ln -s ../../git-hooks/pre-commit .git/hooks/pre-commit
    ```

    Note: `../..` is *not* a mistake.

[editorconfig]: http://editorconfig.org/ "EditorConfig"

## The Boy Scout Rule

Always leave the campground cleaner than you found it. In other words, if you
work on some part of the system and find some code that violates these
guidelines without a reason, make a separate commit that cleans up that code.

*Rationale*: it's impossible to automatically upgrade the whole codebase to
follow these guidelines, but doing it bit by bit is very feasible.

## Order of Includes

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
