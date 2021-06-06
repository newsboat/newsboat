# Changes for Newsboat

## Unreleased - expected 2021-06-21

### Added
### Changed
- Bumped minimum supported Rust version to 1.48.0

### Deprecated
### Removed
### Fixed
### Security



## 2.23 - 2021-03-21

Lists below only mention user-visible changes, but the full list of contributors
for this release also includes: John Jarvis, Nicholas Defranco, and Raphael
Nestler.

### Added

- `open-in-browser-noninteractively` operation which is similar to
    `open-in-browser`, but doesn't relinquish the terminal to the browser. It
    still waits for the browser to finish executing, though (Dennis van der
    Schagt)
- Confirmation for `delete-all-articles`. It's enabled by default, but can be
    disabled with `confirm-delete-all-articles no`. (#1490) (Amrit Brar)
- `%U` specifier for `feedlist-title-format` which shows the total number of
    unread articles in all feeds (#1495) (Dennis van der Schagt)
- Display images' alternate text in the article view (#1512)
    (Mark A. Matney, Jr)
- List `iframe` URLs in the article view (#1153) (Mark A. Matney, Jr)

### Changed

- Newsboat now refuses to enqueue a podcast if its filename is already present
    in the queue. If that happens, you'll have to adjust
    `download-filename-format` to make the filenames more distinguishable
    (#1411) (Dennis van der Schagt)
- Reduced message flickering when reloading feeds (Dennis van der Schagt)
- Updated translations: Dutch (Dennis van der Schagt), German (Lysander
    Trischler), Italian (Mauro Scomparin), Polish (Carno), Russian (Alexander
    Batischev), Turkish (Emir Sari), and Ukrainian (Alexander Batischev)
- Bumped minimum supported Rust version to 1.46.0
- Updated vendored library Catch2 to 2.13.4

### Removed

- `dumpform` command-line command which was only intended for debugging
    (Dennis van der Schagt)

### Fixed

- Missing empty lines inside `pre` tags (#1429) (Alexander Batischev)
- `open-all-unread-in-browser-and-mark-read` not synchronizing the "read" status
    to the remote API (#1449) (Dennis van der Schagt)
- Newsboat redrawing the screen once a minute even if idle (#563) (Dennis van
    der Schagt)
- `delete-all-articles` no longer deletes items that aren't visible (e.g.
    because of `ignore-mode display`) (#1360) (Alexander Batischev)
- Slashes are now replaced by underscores when generating a podcast filename
    (#836) (Dennis van der Schagt)
- File- and dirbrowsers no longer produce invalid paths when user navigates with
    arrow keys (#1418) (Dennis van der Schagt)
- Successful OPML import is no longer misreported as an error (Alexander
    Batischev)
- Descriptions in the help dialog are localized again (#1471) (Emir Sari)
- Added a newline after each `div`, since it's a block element (#1405)
    (Alexander Batischev)
- Re-introduce `set x!` (toggle) and `set x&` (reset) (#1491) (Dennis van der
    Schagt)



## 2.22.1 - 2021-01-10

### Fixed

- Slow scrolling in the article list (regression) (#1372) (Alexander Batischev)
- Segfaults if `swap-title-and-hints` is enabled (regression) (#1399) (Dennis
    van der Schagt)
- Build failure on GCC 9 due to `maybe-uninitialized` warning which `-Werror`
    turns into an error (Alexander Batischev)
- Articlelist's title not being updated when moving to the next unread feed
    (#1385) (Alexander Batischev)



## 2.22 - 2020-12-21

Lists below only mention user-visible changes, but the full list of contributors
for this release also includes David Brito and panvicka.

### Added

- `confirm-mark-all-feeds-read` setting, which makes Newsboat ask for
    confirmation before marking all the feeds as read (#1215) (Tarishi Jain)
- Command line autocompletion in the save dialog (#893) (Dennis van der Schagt)
- Support for ^U, ^K, ^G, and ^W editing keys (as in readline and Emacs) in
    command line in the save dialog (Dennis van der Schagt)
- Support for RSS Media extension in Atom feeds (#595) (Dennis van der Schagt)
- New, more detailed, documentation chapters on macros and running external
    commands (A1RO)
- User-contributed script that exports feeds with their tags in the OPML format:
    contrib/exportOPMLWithTags.py (jartigag)
- Help dialog in the URLs view (#1218) (Dennis van der Schagt)
- Handling of terminal resizes for all dialogs (#389, #390) (Dennis van der
    Schagt)
- `goto-title` operation, which selects a feed with a given title (#888, #1135)
    (Dennis van der Schagt)
- `--cleanup` command-line flag, which does the same as `cleanup-on-quit`
    option (#1182) (Dennis van der Schagt)
- `check` and `ci-check` Makefile targets. Both run C++ and Rust test suites
    consecutively, but the former fails early. Use `check` locally where
    re-running tests is quick, and use `ci-check` in CI where re-running tests
    usually means re-building everything first (#896) (Alexander Batischev)
- Command line support in the help dialog (Dennis van der Schagt)
- "(localized)" marks in documentation for all settings with internationalized
    default values (#1270) (Amrit Brar)
- `%F` placeholder in `browser` setting, which is *always* replaced by the
    feed's URL (unlike `%u`, which depends on the context in which the browser
    is invoked) (#423) (Dennis van der Schagt)
- Dumping of `ignore-article` rules with `dumpconfig` command (in Newsboat's
    internal command line) (#635) (Dennis van der Schagt)
- `%L` placeholder in `datetime-format` setting, which turns into "X days ago"
    string explaining when the article was published (#1323) (Amrit Brar)
- Support for escaped double quotes in arguments to `set` operation when used in
    macros (#1345) (Dennis van der Schagt)
- Podboat: error message if the podcast file can't be written onto disk (#1209)
    (Nicholas Defranco)

### Changed

- Abort startup if the urls file or config file is not in UTF-8 encoding. This
    limitation was effectively in place for a couple releases already, but
    Newsboat crashed instead of displaying an error message. We intend to relax
    the requirement again, but for now, we choose to be upfront about it rather
    than crashing (#723, #844) (Dennis van der Schagt, Alexander Batischev)
- `save-all` operation no longer provides "yes for all" and "no for all" options
    when there is only one conflict to resolve (#657) (saleh)
- Config parser now allows to have tab characters between `macro` arguments;
    they will be treated as space (Dennis van der Schagt)
- Bumped minimum supported Rust version to 1.44.0
- Updated vendored libraries: Catch2 to 2.13.3, martinmoene/optional-lite
    to 3.4.0, martinmoene/expected-lite to 0.5.0
- Updated translations: Dutch (Dennis van der Schagt), French (tkerdonc), German
    (Lysander Trischler), Italian (Mauro Scomparin), Polish (Carno), Russian
    (Alexander Batischev), Turkish (Emir Sarı), Ukrainian (Alexander Batischev)

### Fixed

- Whitespace not being consolidated in item titles (#1227) (Dennis van der
    Schagt)
- Misleading error message when the urls file exists, but can't be opened (#439)
    (Dennis van der Schagt)
- Newsboat processing the leftovers of `stdin` after running `open-in-browser`
    operation (#26, #63, #1094) (Dennis van der Schagt)
- `mark-feed-read` operation not marking articles as read on a remote service
    when the operation is invoked in the query feed (#220) (Dennis van der
    Schagt)
- `run-on-startup` setting preventing Podboat from starting (#1288) (Dennis van
    der Schagt)
- `scrolloff` setting being ignored when opening a feed with lots of read
    articles before the unread one (#1293) (Dennis van der Schagt)
- Memory corruption while rendering an article with JavaScript that contains
    HTML (#1300) (Alexander Batischev)
- Podboat help dialog crashing if `BACKSPACE` is bound (#1139) (Dennis van der
    Schagt)
- Being unable to run a second Newsboat instance with `--cache-file` switch if
    `cache-file` setting is used (#1318) (Dennis van der Schagt)
- Misleading "an instance is already running" message when a lock file can't be
    created or written to (#314) (Dennis van der Schagt)
- Failing to parse macros which contain semicolons in operations' arguments
    (#1200) (Alexander Batischev)
- Not installing some of the contrib scripts (Alexander Batischev)



## 2.21 - 2020-09-20

Lists below only mention user-visible changes, but the full list of contributors
for this release also includes Ivan Tham.

### Added

- Build dependency on AWK
- A note that security vulnerability should be reported to
    security@newsboat.org, preferably encrypted to PGP key 4ED6CD61932B9EBE
- Confirmation before marking all feeds as read (#1006) (Dennis van der Schagt)
- `scrolloff` setting which keeps the specified number of lines above and below
    the selected list item (#1103) (Dennis van der Schagt)
- `%=[width][identifier]` formatting sequence for `*-format` settings. It
    centers a given value inside a given width, padded with spaces and slanting
    to the left if it can't be aligned evenly (Daniel Bauer)
- Support for Miniflux (#448) (Galen Abell)
- `run-on-startup` setting which executes a given list of operations when
    Newsboat starts. This can be used to e.g. open tag dialog on startup, or go
    to a certain feed (#888) (Dennis van der Schagt)
- Documentation for `one`, `two`, ..., `nine`, `zero` operations that open
    a corresponding URL in the browser (A1RO)

### Changed

- It is now a startup error for a macro to have no operations
- Bumped minimum supported Rust version to 1.42.0
- Updated vendored libraries: Catch2 to 2.13.1, json.hpp to 3.9.1
- Empty strings in filter expressions are treated as zero when compared with
    a numeric attribute like `age` (Alexander Batischev)
- Converted various tables in docs to decorated lists, making them easier to
    read (#441) (Spacewalker2)
- In macros, no longer require a space between operation and the following
    semicolon (#702) (Dennis van der Schagt)
- Sorting by first tag now ignores "title tags", i.e. the ones that start with
    a tilde (#1128) (José Rui Barros)
- contrib/feedgrabber.rb updated to use Newsboat directories instead of
    Newsbeuter's (Fabian Holler)
- Updated translations: Dutch (Dennis van der Schagt), German (Lysander
    Trischler), Polish (Carno), Russian (Alexander Batischev), Turkish (Emir
    Sarı), Ukrainian (Alexander Batischev)

### Fixed

- TT-RSS not taking the MIME type into account when deciding what enclosure to
    pick (#941) (chux0519)
- Typos in documentation (Edgar Hipp)
- History files storing the *oldest* entries instead of the most recent ones
    (#1081) (Dennis van der Schagt)
- Search dialogs all displaying results of the last search, not their individual
    searches (#1087) (Dennis van der Schagt)
- Feeds apparently not being sorted after a reload (#1089) (Alexander Batischev)
- Search dialog displaying the new query even if the search failed (Dennis van
    der Schagt)
- `delete-all-articles` operation not working in the search dialog (Dennis van
    der Schagt)
- First feed marked as read when deleting all items in search dialog (Dennis van
    der Schagt)
- Arrow keys not working in the tag list (Dennis van der Schagt)
- Inoreader not marking items unread (#1109) (José Rui Barros)
- `content` attribute being unavailable to query feeds (#111, #218)
    (Dennis van der Schagt)
- Newsboat sometimes opening wrong items (#72, #1126) (Dennis van der Schagt)
- Unread items being double-counted by `-x print-unread` and notifications
    (#444, #1120) (Alexander Batischev)
- Nested lists being strung out into a single, non-nested list (#1158) (Dennis
    van der Schagt)
- Colons sometimes making filter expressions invalid (Alexander Batischev)
- Child processes that display notifications not being waited on. We now
    double-fork them (glacambre)
- Newsboat deleting all items of a feed when `cleanup-on-quit` is enabled (the
    default) and user moves from a search feed to a different feed with
    `next-unread` or `prev-unread` operations (#685) (Dennis van der Schagt)



## 2.20.1 - 2020-06-24

### Fixed
- Installation on BSDs (Tobias Kortkamp)
- Regression that caused Newsboat to require a space before semicolon in macros,
    which made `set browser "lynx"; open-in-browser` invalid (#1013, #1015,
    \#1017, #1018) (Alexander Batischev)
- Possible segfault upon startup (#1025) (Dennis van der Schagt, Alexander
    Batischev)
- Feed sorting in Spanish locale (#1028) (Dennis van der Schagt, Alexander
    Batischev)



## 2.20 - 2020-06-20

Lists below only mention user-visible changes, but the full list of contributors
for this release also includes: Björn Esser, Jan Staněk, Mario Rugiero, Rui
Chen, and Tobias Kortkamp.

### Added
- An FAQ item on why TT-RSS authentication might fail (#44) (Alexander
    Batischev)
- An FAQ item on browser failures in Newsboat 2.18 (Alexander Batischev)
- Ability to bind multiple keys to the same operation (#110) (Dennis van der
    Schagt)
- Ability to bind operations to Tab key (Dennis van der Schagt)
- New format specifiers for `articlelist-format`: `%n` (article unread), `%d`
    (article deleted), `%F` (article's flags) (Dennis van der Schagt)
- New format specifier for `feedlist-title-format`,
    `articlelist-title-format`, and `searchresult-title-format`: `%F`, which
    contains current filter expression. That specifier is now included into
    those settings by default (#946) (Dennis van der Schagt)
- New setting, `switch-focus`, which specifies a key that moves the cursor
    between widgets in File- and DirBrowser (Dennis van der Schagt)
- New setting, `wrap-scroll`, which makes the cursor jump to the last item when
    scrolling up on the top one, and vice versa (David Pedersen)
- `exec` command-line command, which allows to run an arbitrary operation (#892)
    (Marco Sirabella)
- Dependency on martinmoene/optional-lite and martinmoene/expected-lite
    libraries, both of which we vendor
- Include enclosure URL in the article's urlview (#809) (Spacewalker2, Alexander
    Batischev)
- Allow `open-in-browser` and `open-in-browser-and-mark-read` operations in the
    URL view, where they open the selected URL (David Pedersen)
- Open command line when a number key is pressed in a tag-list (#939) (Dennis
    van der Schagt)
- Install Newsboat's SVG icon as part of `install` target (Nikos Tsipinakis)

### Changed
- Merged es and es_ES translations into one (Marcos Cruz)
- Updated translations: Dutch (Dennis van der Schagt), German (Lysander
    Trischler), Russian (Alexander Batischev), Spanish (Marcos Cruz), Turkish
    (Emir Sari), Ukrainian (Alexander Batischev)
- It's now an error to have `always-download` or `reset-unread-on-update`
    without parameters (Alexander Batischev)
- It's now an error to have `macro` without at least two arguments (Alexander
    Batischev)
- The conditional format sequence (`%?[char]?[format]&[format]?`) now treats
    whitespace-only value as empty. This allows changing the formatting of, for
    example, "unread" and "deleted" fields in articlelist-format (Dennis van der
    Schagt)
- `open-in-browser-and-mark-read` in feedlist no longer marks articles read if
  the browser fails (Nikos Tsipinakis)
- Macro execution halts if one of the operations fails (Nikos Tsipinakis)
- Inoreader now marks articles read on a thread, to hide latency (#710)
    (pi.scateu.me)
- Bumped minimum supported Rust version to 1.40.0
- Updated vendored libraries: Catch2 to 2.12.2, json.hpp to 3.8.0

### Removed
- Newsboat's Inoreader API keys. Users need to register their own Inoreader
    application now, and set them via `inoreader-app-id` and `inoreader-app-key`
    settings. Please see "Inoreader" section in the HTML documentation for
    details. (Alexander Batischev)

### Fixed
- Help dialog showing operations as unbound even though they *are* bound to some
    keys (#843) (Dennis van der Schagt)
- `feedlink` attribute containing feed title instead of feed URL (Alexander
    Batischev)
- `feeddate` attribute containing fixed string instead of item's publication
    date and time (Alexander Batischev)
- `browser` setting not working if it contains `<` (#917) (Dennis van der
    Schagt)
- `up`, `down`, `pageup`, `pagedown`, `home`, and `end` now working in macros
    (#890) (Dennis van der Schagt)
- Backslash inside double quotes requiring three escapes instead of one, every
    other time (#536, #642, #926) (Alexander Batischev)
- Users can bind operations to `UP`, `DOWN`, `HOME`, `END`, `NPAGE`, and `PPAGE`
    keys (#903) (Dennis van der Schagt)
- Generate example config as part of `doc` target, so `install-examples` can
    simply copy it instead of generating (Alexander Batischev)
- Install manpages via `install-docs` target, not `install-newsboat` and
    `install-podboat` (#829) (Alexander Batischev)
- The wrong feed being opened (#72) turned out to be caused by a bug in libstfl.
    A patch for that library is available at
    https://github.com/dennisschagt/stfl/pull/4#issuecomment-613640246 (Dennis
    van der Schagt)



## 2.19 - 2020-03-22

Lists below only mention user-visible changes, but I would also like to
acknowledge contributions from the following people: Ivan Tham, Merlin Büge,
Raphael Nestler, and Tobias Kortkamp.

This is the last release to support Rust 1.26.0. Starting with Newsboat 2.20,
we will be supporting only the last five stable Rust compilers (at the time of
the release), e.g. Newsboat 2.20 will only support Rust 1.40, 1.41, 1.42, 1.43,
1.44 (which should be the current stable at the time of Newsboat 2.20 release).
Please see https://github.com/newsboat/newsboat/issues/709 for more details on
this decision.

### Added
- contrib/urls-maintenance.sh: a script that converts HTTP to HTTPS, updates
    URLs according to HTTP redirects etc. (velaja)
- `delete-played-files` setting (#669) (Dennis van der Schagt)
- `%K` format for  `podlist-format`. This format specifier is replaced by the
  human readable download speed (automatically switches between KB/s, MB/s, and
  GB/s) (Dennis van der Schagt)
- Docs on how to synchronize with Bazqux (Jonathan Siddle, Alexander Batischev)
- Document that regexes use POSIX extended regular expressions
- Document that regexes in filter language are case-insensitive

### Changed
- Dependency: we now use Asciidoctor instead of Asciidoc
- Dependency on Rust: we now have a schedule for bumping the minimum supported
    Rust version (#709)
- Update vendored version of Catch2 to 2.11.3
- Display `<audio>` and `<video>` tags in article view (Ignacio Losiggio)
- Update translations: Dutch (Dennis van der Schagt), German (Lysander
    Trischler), Russian, Ukrainian (Alexander Batischev)
- `podlist-format` now uses `%K` instead of `%k` by default (shows human
    readable speed instead of always using KB/s) (#727) (Dennis van der Schagt)
- contrib/pinboard.pl: save description of the article (Donald Merand)
- The EOT markers ("~" characters below blocks of text) no longer inherit their
  style (colors + attributes) from the "article" style. Instead, they can be
  configured separately, allowing to hide them without hiding the article text
  (example config line: `color end-of-text-marker default default invis`) (#507)
  (Dennis van der Schagt)

### Fixed
- **Breaking change**: `bind-key` context `podbeuter` renamed to `podboat`
    (Alexander Batischev) (Kudos to Marcos Cruz)
- Garbage displayed in empty lines turned out to be a bug in libstfl. Dennis van
    der Schagt created a patch and submitted it upstream on 7 March 2020, but
    the upstream maintainer haven't responded. Please apply the patch yourself:
    https://github.com/newsboat/newsboat/issues/506#issuecomment-596091556
    (#273, #506) (Dennis van der Schagt)
- Podboat now saves and restores "finished" state of the podcast (#714) (Dennis
    van der Schagt)
- Command-line options that take paths as arguments (--cache-file, --url-file
    etc.) now resolve tilde as path to the home directory (#524) (Alexander
    Batischev)
- `--execute print-unread` now takes `ignore-article` into account (#484)
    (@Brn9hrd7)
- Podboat no longer spuriously creates .part directories (#725) (Dennis van der
    Schagt)
- Incorrect paths in filebrowser and dirbrowser when navigating with arrow keys
    and Enter (#547) (Dennis van der Schagt)
- Incorrect dates parsing on macOS 10.15 Catalina (Alexander Batischev)
- `--help` now displays paths to config, urls file, and cache file (#294)
    (Alexander Batischev)
- Documentation now correctly explains that positive padding values add padding
    on the left (Dennis van der Schagt)
- Newsboat not displaying titles of empty feeds (#732) (Dennis van der Schagt)
- Newsboat forgetting feed titles if reload brought no new items (#748)
    (Alexander Batischev)
- filebrowser and dirbrowser displaying ".." instead of an actual directory path
    (#731) (Dennis van der Schagt)
- `make -jN` now *really* limits the number of jobs to N (#768) (Anatoly Sablin,
    Alexander Batischev)
- `pb-purge` (`P` in Podboat) no longer removes played files, just as
    documentation claims (Dennis van der Schagt)
- `highlight` in feedlist being overridden after reload (#37) (Dennis van der
    Schagt)
- `highlight` regexes unable to match beginning-of-line (#242, #535) (Dennis van
    der Schagt)
- Search not extending into and across hyperlinks (#331) (Dennis van der Schagt)
- `highlight` in articles extending beyond the text that the regex matched
    (#488) (Dennis van der Schagt)
- `highlight` that matches beginning-of-line matching again after the first
    match (#796) (Dennis van der Schagt)
- Feed/article titles are now sanitized, to prevent HTML markup from breaking
    formatting (#796) (Dennis van der Schagt)
- Plain-text rendition of an article no longer contains STFL markup (Dennis van
    der Schagt)
- "Filler sequence" (`%>`) not working in format strings for articlelist,
    dialogs, help, select-tag, select-filter and urls dialogs (#88) (Dennis van
    der Schagt)
- Cursor in Podboat is hidden (Dennis van der Schagt)
- Crash when displaying an article that has double-closed `<ol>` tags (#659)
    (Dennis van der Schagt)
- Alignment of feed- and articlelist broken by wide characters like CJK and
    emojis (#139, #683) (Dennis van der Schagt)
- Whitespace ignored if followed by an HTML tag (#512) (Dennis van der Schagt)



## 2.18 - 2019-12-22

Lists below only mention user-visible changes, but I would also like to
acknowledge contributions from the following people: Simon Schuster, and seanBE.

### Added
- Logo by noobilanderi

### Changed
- `open-in-browser-and-mark-read` no longer marks item read if browser returned
    a non-zero exit code. Similarly, `open-all-unread-in-browser` and
    `open-all-unread-in-browser-and-mark-read` abort on non-zero exit code
    (Marco Sirabella)
- Update vendored version of Catch2 to 2.11.0
- Update vendored version of nlohmann/json to 3.7.3
- Update translations: German (Lysander Trischler), Russian, Ukrainian
    (Alexander Batischev)

### Fixed
- `unbind-key -a` breaking cmdline, search and goto-url (#454) (kmws)
- Flaky `run_command()` test (Alexander Batischev)

### Security
- smallvec crate bumped to 0.6.10, to get fixes for RUSTSEC-2019-0009 and
    RUSTSEC-2019-0012



## 2.17.1 - 2019-10-02

### Added
- Mention that `cookie-cache` setting uses Netscape file format (Alexander
    Batischev on a prod from f-a)

### Changed
- Update German translation (Lysander Trischler)

### Fixed
- Feeds not updating when `max-items` is set (#650). This negates some of the
    performance improvement we got in 2.17, but we haven't measured how much;
    it's guaranteed to not be any slower than 2.16.1 (Alexander Batischev)
- Failing to start if config contains `#` that doesn't start a comment, e.g.
    inside regular or filter expression (#652) (Alexander Batischev)



## 2.17 - 2019-09-22

Lists below only mention user-visible changes, but I would also like to
acknowledge contributions from the following people: Raphael Nestler, kpcyrd,
and seanBE.

### Added
- FreeBSD and Linux i686 jobs on continuous integration servers. We won't break
    the build on these platforms ever again (Alexander Batischev)
- Documentation for `macro-prefix` settings (Lysander Trischler)
- `save-all` operation, to save all articles in the feed (Romeu Vieira)
- `dirbrowser-title-format` setting, used in the DirBrowser dialog invoked by
    `save-all` operation (Romeu Vieira)
- `dirbrowser` context in `bind-key` command, to add bindings to DirBrowser
    dialog invoked by `save-all` (Alexander Batischev)
- `selecttag-format` setting, to control how the lines in "Select tag" dialog
    look (Penguin-Guru, Alexander Batischev) (#588)

### Changed
- Bumped minimum required Rust version to 1.26.0
- Update vendored version of nlohmann/json to 3.7.0
- Update vendored version of Catch2 to 2.9.2
- Update Italian translations (Leandro Noferini)

### Removed
- Some unnecessary work done at startup time, shaving off 6% in my tests
    (Alexander Batischev)

### Fixed
- `newsboat --version` not displaying the version (Alexander Batischev) (#579)
- Processing backticks inside comments (Jan Staněk)
- Use-after-free crash when opening an article (Juho Pohjala) (#189)
- Crash on `toggle-item-read` in an empty feed (Nikos Tsipinakis)
- Un-applying a filter when command is ran or operation is executed (Nikos
    Tsipinakis) (#607, #227)
- Numerous memory leaks detected by Clang's AddressSanitizer (Alexander
    Batischev) (#620, #621, #623, #624)
- Segfauls when toggling article's "read" status after marking all articles read
    (#473) (Nikos Tsipinakis)



## 2.16.1 - 2019-06-26

### Changed
- Update German translations (Lysander Trischler)

### Fixed
- Build on FreeBSD and i386 (Tobias Kortkamp)



## 2.16 - 2019-06-25

Lists below only mention user-visible changes, but I would also like to
acknowledge contributions from Ivan Tham.

### Added
- Install changelog and contrib/ alongside docs (Alexander Batischev) (#474)
- `show-title-bar` config option to hide the title bar. Defaults to yes, i.e.
    the behaviour is the same as with Newsboat 2.15 (Sermak) (#375)
- Contrib scripts for image preview (Sermak) (#480)
- Nord colour scheme (Daryl Manning)
- Ability to search withing the search results, narrowing them down (Tumlinh)
    (#327)
- Color scheme based on Adapta-Maia GTK theme (Lucas Parsy)

### Changed
- Marking feed as read only resets the cursor if article list is sorted by date
    (Stefan Assmann)
- `include` also accepts relative paths (Marco Sirabella) (#489)
- Update vendored version of nlohmann/json to 3.6.1
- Update vendored version of Catch2 to 2.9.1

### Fixed
- Parser breaking on spaces inside backticks (Marco Sirabella) (#492)
- Hidden tags changing the title of their feeds (Alexander Batischev) (#498)
- Segfaults some time after using an invalid regex in a filter expression
    (Alexander Batischev) (#501)
- Single quotes in podcast names replaced by %27 (屑鉄さらい;Scrap Trawler)
    (#290, #457)
- Out-of-bounds access on empty "author" tag in RSS 0.9x (Alexander Batischev)
    (#542)



## 2.15 - 2019-03-23

### Added
- `random` `article-sort-order` (Jagannathan Tiruvallur Eachambadi)
- Cursor position in article list is reset after marking the feed as read
    (Stefan Assmann)

### Changed
- Update vendored version of Catch2 to 2.7.0
- Give our Snap access to `xdg-open`
- Make "delete all items" work in query feeds (Alexander Batischev) (#456)

### Fixed
- Use a native compiler for internal tools when cross-compiling (maxice8)
- Always write to `error-log`, no matter the log level specified on the command
    line (Alexander Batischev)
- (Regression) Let users interact with programs run by "exec:", backticks in
    config, and `*-passwordeval` settings (Alexander Batischev) (#455)
- Do not add deleted items to query feeds (Alexander Batischev) (#456)
- Setup directories before importing feeds, to avoid the import silently failing
    (Neill Miller)



## 2.14.1 - 2019-02-10

Lists below only mention user-visible changes, but I would also like to
acknowledge contributions from the following people: kkdd, Raphael Nestler.

### Added
- Support for cross-compilation with CARGO_BUILD_TARGET environment variable
    (maxice8)
- `%N` format for `download-path` and `download-filename-format` settings. This
    format is replaced by item's original feed-title, even when selected through
    the query feed (Felix Viernickel) (#428)

### Changed
- Translations: Polish (Carno)
- When opening a never-fetched feed in the browser, just use the feed's URL
    (Alexander Batischev)
- Update vendored version of Catch2 to 2.6.0

### Fixed
- Build on FreeBSD (Tobias Kortkamp)
- Messed-up highlighting when regex matches start-of-line (zaowen) (#401)
- Failing to update The Old Reader feeds (Alexander Batischev) (#406)
- "NewsBlur" spelling throughout the docs and messages (zaowen) (#409)
- Lack of space between podcast URL and its MIME type (Alexander Batischev) (#425)
- "rev-sort" command name in docs (Jakob Kogler)
- Keybindings not applied in dialogs view (Felix Viernickel) (#431)
- Spacer formatter not working in podlist-format (Alexander Batischev) (#434)



## 2.14 - 2018-12-29

Lists below only mention user-visible changes, but I would also like to
acknowledge contributions from the following people: Paul Woolcock, Raphael
Nestler, Thanga Ayyanar A.

### Added
- Dependency on Rust 1.25+. The compiler (`rustc`) and the build tool (`cargo`)
    are required
- `download-filename-format` setting that controls how Podboat names the files.
    Default is the same as older versions of Podboat (Jagannathan Tiruvallur
    Eachambadi)
- `podlist-format` setting that controls formatting of the items on Podboat's
    main screen. Also, a `%b` format specifier that contains just the basename
    of the download (e.g. "podcast.mp3" instead of "/home/name/podcast.mp3")
    (zaowen)
- Human-readable message when Rust code panics (Alexander Batischev)
- `unbind-key -a`, which unbinds all keys (Kamil Wsół)

### Changed
- Look up `BROWSER` environment variable before defaulting to lynx(1) (Kamil
    Wsół) (#283)
- Strip query parameters from downloaded podcasts' names (i.e. name them as
    "podcast.mp3", not "podcast.mp3?key=19ad740") (Jagannathan Tiruvallur
    Eachambadi)
- Update translations: Russian, Ukrainian (Alexander Batischev), German
    (Lysander Trischler)
- Document that minimum supported CURL version is 7.21.6. This has been the case
    since 2.10, but wasn't documented at the time (Alexander Batischev)
- Update vendored version of nlohmann/json to 3.4.0
- Update vendored version of Catch2 to 2.5.0

### Fixed
- HTTP response 400 errors with Inoreader (Erik Li) (#175)
- Podboat's crash (segmentation fault) when parsing a comment in the queue file.
    Comments aren't really supported by Podboat since it overwrites the file
    from time to time, but the crash is still unacceptable (Nikos Tsipinakis)
- Newsboat displaying articles differently in "internal" and external pagers
    (Alexander Batischev)
- One-paragraph items not rendered at all (Alexander Batischev)
- Crash (segmentation fault) on feeds that don't provide a `url` attribute
    (ksunden)
- Hangs when `highlight` rule matches an empty string (zaowen)
- Article disappearing from the pager upon feed reload (zaowen)
    (https://github.com/akrennmair/newsbeuter/issues/534)
- Leading and trailing spaces not stripped from the URLs in <A> tags (Raphael
    Nestler)



## 2.13 - 2018-09-22

Lists below only mention user-visible changes, but I would also like to
acknowledge contributions from the following people: Anatoly Kalin, Eduardo
Sanchez, Friedrich von Never, Kamil Wsół, glacambre.

### Added
- Respect TMPDIR environment variable when writing temporary files (ಚಿರಾಗ್ ನಟರಾಜ್)
    (#250)
- `delete-all-articles` operation that marks all articles in the feed as deleted
    (Kamil Wsół)

### Changed
- Require `cookie-cache` setting if NewsBlur API is used (Alexander Batischev)
- Translations: Russian, Ukrainian (Alexander Batischev), Swedish (Dennis
    Öberg), German (Lysander Trischler)
- json.hpp updated to version 3.2.0
- Natural sort order for article titles, so numbers are put in the expected
    order (e.g. 1, 2, 5, 10, 11 rather than 1, 10, 11, 2, 5) (Nikos Tsipinakis)

### Fixed
- Do not create empty files if history is disabled (Nikos Tsipinakis)



## 2.12 - 2018-06-24

Lists below only mention user-visible changes, but I would also like to
acknowledge contributions from the following people: Kamil Wsół and Simon
Schuster.

### Added
- Ability to override path to `pkg-config` (Ali Lown)
- Socket support in filebrowser (Sebastian Rakel)
- `ls --classify`-like formatting for filenames in filebrowser (Sebastian Rakel)
- Ability to sort feedlist by last update (TwilightSpectre) (#191)
- `:q` as alternative to `:quit` (Franz König)
- Support for `open-in-browser` in URL dialog, thus fixing many user macros in
    that dialog (Felix Viernickel) (#194)
- "Author" field for items fetched from NewsBlur (Chris Nehren)
- Coding style, mostly enforced through `clang-format`. Non-enforceable things
    are documented in docs/code-style.markdown (Alexander Batischev)
- A check in `bind-key` that will now throw an error on binding to
    a non-existent operation (Nikos Tsipinakis)

### Changed
- The markup in docs, to be consistent throughout (Lysander Trischler)
- HTTP to HTTPS in communication with The Old Reader (Richard Quirk)
- Translations: Russian, Ukrainian (Alexander Batischev), Italian (Francesco
    Ariis)

### Fixed
- `setlocale()` no longer fails on MacOS (Jacob Wahlgren, Alexander Batischev)
    (#156)
- Colors for unread items in all contributed colorschemes (@sandersantema)
    (#163)
- Segfaults in dialogs view when `swap-title-and-hints` is enabled (Alexander
    Batischev) (#168)
- Typo in JSON field name in TT-RSS API (Sebastian Rakel) (#177)
- Filebrowser displaying "d" filetype for everything but regular files
    (Sebastian Rakel) (#184)
- TT-RSS relogin (Sebastian Rakel)
- Internal HTML renderer not stripping whitespace in front of text (Alexander
    Batischev) (#204)
- Podboat breaking if XDG data dir already exists (Alexander Batischev)
- Makefile failing if user overrode `ls` somehow (Alexander Batischev)
- Various problems found by clang-analyzer and Coverity Scan (Alexander
    Batischev)



## 2.11.1 - 2018-03-30

### Fixed

- If built from the tarball, Newsboat 2.11 reported its version as 2.10.2. My
    bad. Kudos to Haudegen, Ryan Mulligan and Robert Schütz for catching that
    one. (Alexander Batischev)



## 2.11 - 2018-03-25

Lists below only mention user-visible changes, but I would also like to
acknowledge contributions from the following people: Avindra Goolcharan, David
Pedersen, and Kaligule.

### Added

- Podboat adds ".part" suffix of the files it currently downloads (José Manuel
    García-Patos)
- Support for `CURL_CA_BUNDLE` environment variable (Marius Bakke, Alexander
    Batischev)
- Snapcraft package (Alan Pope)
- Dependency on nlohmann/json (which is now used in TT-RSS interface instead of
    json-c)
- CURL error codes are converted to strings in logs (Alexander Batischev)
- Open command line when a number key is pressed in a list (e.g. feedlist or
    itemlist) (Nikos Tsipinakis)
- Basic Evernote bookmark plugin (see contrib/) (Royce)
- New command: `mark-all-above-as-read` (Roman Vasin)
- Supprot for RSS media enclosures in ownCloud News (dirb)

### Changed

- Valid podcast MIME types are "audio/*", "video/*", and "application/ogg"
    (#105) (Alexander Batischev)
- Use just two queries to fetch items from TT-RSS (only works with API level ≥2)
    (Simon Schuster)
- Make the cursor visible in filebrowser, and put it at the end of the line
    (Nikos Tsipinakis)
- Sort the file list in filebrowser (Nikos Tsipinakis)
- Translations: Russian, Ukrainian (Alexander Batischev), German (Lysander
    Trischler), Italian (Francesco Ariis), French (tkerdonc), Brazilian
    Portuguese (Adiel Mittmann)
- Compile with optimizations (`-O2`) by default (Alexander Batischev, Nikos
    Tsipinakis)

### Fixed

- Unwanted logging to stdout on `--export-to-opml` (#104) (Alexander Batischev)



## 2.10.2 - 2017-12-25

Lists below only mention user-visible changes, but I would also like to
acknowledge contributions from the following people: Alok Singh, Carno, Jonas
Karlsson, Kamil Wsół, Mike Crute, Niels Kobschätzki, and maiki.

### Added

- HTML anchors for all config commands in docs. You can now link to each command
    separately (#10) (Lysander Trischler)
- Support for Inoreader (Bart Libert)
- Slovak translation (František Hájik)

### Changed

- Enqueue *last* audio enclosure
    (https://github.com/akrennmair/newsbeuter/issues/604)
- `text-width` doesn't apply if it's bigger than terminal width
    (https://github.com/akrennmair/newsbeuter/issues/602)
- Translations: German (Lysander Trischler), Russian, Ukrainian
    (Alexander Batischev)

### Removed

- Build dependency on Perl (#6)
- Test dependency on bc (Nikos Tsipinakis)

### Fixed

- Do not create XDG data dir if not using XDG (#8)
- When used with NewsBlur, check on startup if cookie-cache exists or can be
    created, because integration doesn't work without cookies (#13)
- Builds on AARCH64 and ARMHF (#43)
- Only show an error message once when unknown option is supplied
    (Lysander Trischler)
- License header used to say it's MIT/X Consortium License, whereas in reality
    it's a MIT License (discovered by Nikos Tsipinakis)
- Cross-compilation made possible by conditionally assigning to RANLIB and AR in
    Makefile (Fredrik Fornwall)
- Cookies actually get persisted (Simon Schuster, reported and tested by Håkan
    Jerning)
- CJK text is wrapped at correct code-point boundaries (#38, #71) (nmtake)
- Don't segfault if `error-log` points to non-existent file (Simon Schuster)
- Spanish translation (José Manuel García-Patos)



## 2.10.1 - 2017-09-22

### Added

- Documentation for automatic migration from Newsbeuter

### Fixed

- XDG data dir is created if XDG config dir exists (regression happened in 2.10)



## 2.10 - 2017-09-20

This is what Newsbeuter 2.10 should have been. Newsboat continues Newsbeuter's
version numbering to show that we are a spiritual continuation, not a separate
project.

### Added
- Solarized-light colorscheme (OmeGa)
- Japanese and Catalan translations (The Flying Rapist; Alejandro Gallo)
- FAQ list
- Long options support (#38)
- `%F` format in `itemview-title-format`, mapping to feed title (Luke Duncan)
- Support for OwnCloud News (#134) (dirb)
- Documentation for `*-title-format`, `*-jumps-to-next-unread`, and
    `newsblur-url` settings (#234, #358) (Alexander Batischev, Nikos Tsipinakis)
- `ssl-verify` option that controls SSL certificate verification. Default: on
    (#354) (Nikos Tsipinakis)
- Support for `xml:base` attribute in RSS 0.9.x and 2.0 (David Kalnischkies)
- Ability to escape backticks in config with a backslash (#334)
- Support for delta feeds (RFC3229+feed) (Daniel Aleksandersen)
- Command to open multiple articles in browser (and optionally mark them read)
    (Tanguy Kerdoncuff)
- Newsbeuter includes commit hash in version string when built from Git (Nikos
    Tsipinakis)
- New proxy type: socks5h. It proxies DNS requests as well as connections (David
    Kalnischkies)
- Support for h5 and h6 HTML elements (Nikos Tsipinakis)
- Notify users if NewsBlur feed they're subscribed to no longer exists (#494)
    (Andrew Martin)
- `passwordeval` settings for all remote APIs, which obtains a password by
    running a user-specified command (Andrew Martin)
- `ssl-verifyhost` and `ssl-verifypeer` options to control how Newsbeuter checks
    SSL certificates (Xu Fasheng)
- Documentation for `feedhq-url` setting
- Reproducible builds (Bernhard M. Wiedemann)
- Migration of Newsbeuter configs and data

### Changed
- Items fetched via TT-RSS now contain item's author (John W. O'Neill)
- Tags are now extracted from The Old Reader (#189)
- Solarized-dark colorscheme got updated (OmeGa)
- `pkg-config` is used to search for `ncursesw` (Jan Pobrislo)
- ESCDELAY is set to 25ms (#221)
- One can now build and install Newsbeuter without localization files with `make
    newsbeuter && sudo make install-newsbeuter` (#241)
- Marked lists (`<ol>`) are now rendered with a space after the marker
- URLs are now hard-wrapped on the window's edge, even if `text-width` is
    non-zero (#282)
- contrib/pinboard.pl got a cosmetic update (Srijith Nair)
- `dc:creator` is now the same as `author` in RSS 1.0 (#143)
- Bookmark scripts now receive feed's title as the fourth parameter (#341)
- SSL certificate verification is now on by default (#354) (Nikos Tsipinakis)
- Cursor in Newsbeuter is hidden most of the time (#344) (Tobias Umbach)
- "Catchup all" renamed to "Mark all read" (#216)
- Articles are marked read when passed to external pager (#495)
- `passwordfile` setting now exists for all remote APIs (Andrew Martin)
- Do not set HTTP WWW-Authenticate header for multiuser TT-RSS, allowing for it
  to be hosted behind http-basic auth (Simon Schuster)
- Cache file is now created in XDG dir if config is in XDG dir (#245)
- Self-closing tags like `<br/>` are not ignored anymore (#281)

### Deprecated
- When using `colorN` notation, N can't start with zero anymore (#186)

### Removed
- Wesnoth fix XSL as the original feed isn't broken anymore
- Offline mode

### Fixed
- (CVE-2017-12904) Sanitize parameters that are passed to bookmark-cmd (#591)
    (Jeriko One, Alexander Batischev)
- (CVE-2017-14500) Sanitize podcast filenames when playing the file via
    Podbeuter (#598) (Simon Schuster)
- Translations: German (Simon Nagl, Lysander Trischler), Russian (kstn), French
    (esteban123456789, rugie), Spanish (Alejandro Gallo)
- Format errors in Brazilian Portuguese, Ukrainian and Chinese localization
  files (#274)
- Undefined behaviour in configcontainer (#135) (Andrey Hitrin)
- Segfault in Podbeuter, along with two potential bugs (Tilman Keskinoz)
- Cache deletion when only one feed is configured (Harshaverdhan Rangan, Simon
    Nagl)
- Example for macro that executes external command via `set-browser` (mrbiber)
- Tags in NewsBlur API (aniran)
- Typo in config commands descriptions (Travis Reddell)
- Dummy articles in NewsBlur API (aniran)
- Numerous bugs caused by FeedHQ API not passing a setting to `libcurl` (Keith
    Smiley)
- *The* memory leak (gave us quite a bit of a headache, that one) (cpubug)
- Multiple `highlight-article` (#166) (Luke Duncan)
- Colors for unread feeds in feedlist (Luke Duncan)
- Whole feeds occasionally marked unread and already enqueued enclosures
    re-enqueued (#164) (trUSTssc)
- Errors when retrieving feeds from The Old Reader (#150)
- Parser fails when three arguments were passed to `highlight` (#225)
- Query feeds tokenization (#194)
- `highlight-article` priority (it's now higher than item formats) (#227) (Luke
    Duncan)
- Feedlist/articlelist slowness when there's a lot of items (#110)
- "Catchup all" in tag view is now limited to that view (#251)
- In podbeuter, requesting to download a file that is already downloaded will
    re-download the file, not delete it (#169)
- One-line `urls` files without line feed at the end are no longer considered
  empty by Newsbeuter (smaudet)
- Items being skipped while applying ignore rules (#269)
- Incorrect behaviour if no ignore rules are present (#283)
- Potential bug in RSS parser (#287) (Timotej Lazar)
- Segfault with NewsBlur (#261) (Thomas Weißschuh)
- Text wrapping (#256)
- ESC in filebrowser ("Save article" dialog etc.) actually works now (#252)
- FeedHQ, OldReader and TT-RSS APIs won't segfault on FreeBSD anymore if
    `ttrss-password` is not set (#336)
- Articles that were marked read in the search dialog now stay read when you go
    back to articles view (#137, #339) (Andrey Hitrin)
- Search in query feeds (#313) (Nikos Tsipinakis)
- Newlines inside text blocks are treated as whitespace (#351) (Nikos
    Tsipinakis)
- Not Found errors (HTTP code 404) in Pocket script (#357) (Vlad Glagolev)
- Off-by-one error in dumpconfig (#359) (Nikos Tsipinakis)
- If `prepopulate-query-feeds` is set, query feeds are populated *before*
    feedlist is sorted, fixing `unreadarticlecount` sorting (#362)
- Toggling unread flag now removes deleted flag (#330) (Lance Orner)
- Don't fail if iconv doesn't support //TRANSLIT (#364, #174) (Ján Kobezda)
- Date conversion accounts for DST now (#369) (Lance Orner)
- Newsbeuter always using XDG directories when in silent mode (#374)
- Don't allow query feeds to be opened in browser (Nikos Tsipinakis)
- NewsBlur authentication failure
- Soft hyphens messing up the output (#259)
- Newsbeuter no longer gets confused by duplicate flags (#440) (Tanguy
    Kerdoncuff)
- Don't ignore config's last line if there's no \n at the end (#426) (Spiridonov
  Alexander)
