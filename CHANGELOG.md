# Changes for Newsboat

# 2.42 - 2025-12-28

### Added

- Option `toggleitemread-jumps-to-next` (#2271) (Jorenar)

### Changed

- In documentation, references to other sections turned into hyperlinks for
    easier navigation (Dennis van der Schagt)
- Updated translations: Chinese (CookiePieWw), Dutch (Dennis van der Schagt),
    German (Lysander Trischler), Italian (Mauro Scomparin), Polish (Carno),
    Russian and Ukrainian (Alexander Batischev), Swedish (Dennis Öberg),
    Turkish (Emir SARI)
- Bumped minimum supported Rust version to 1.88.0

### Fixed

- No error messages when OPML import fails (Lysander Trischler)
- Build failure on macOS (Forketyfork)
- Position in article not updated unless the window is resized (#3204)
    (Juho Eerola)
- Typo in documentation (M0RP43U588)
- Cookies not written with curl 8.17 (#3221) (Alexander Batischev)



# 2.41 - 2025-09-21

Lists below only mention user-visible changes, but the full list of contributors
for this release also includes bogdasar1985.

### Added

- Error message for when operation is not supported in the current view (Dennis
    van der Schagt)
- Fallback to libxml2's encoding autodetection when Newsboat's fails (#3070)
    (Dennis van der Schagt)
- Import feed titles from OPML (#3063) (Jorenar)

### Changed

- Ported some helper programs that generate parts of the documentation from C++ to
    AWK, which was already used for some other helper scripts (Dennis van der
    Schagt, Juho Eerola)
- Switched from `curl_proxytype` to `long int` for compatibility with curl
    8.16.0+ (Carno)
- Updated translations: Chinese (CookiePieWw), Dutch (Dennis van der Schagt),
    German (Lysander Trischler), Italian (Mauro Scomparin), Polish (Carno),
    Russian and Ukrainian (Alexander Batischev), Swedish (Dennis Öberg),
    Turkish (Emir SARI)
- Bumped minimum supported Rust version to 1.85.0
- Updated vendored library expected-lite to 0.9.0 (Alexander Batischev)

### Fixed

- Crash in `:dumpconfig` after `:set` was used to toggle or reset a non-existent
    option (#3104) (Dennis van der Schagt)
- Crash in RSS parsers if there is no "channel" element (Burkov Egor)
- Error messages being written to stdout rather than stderr (Lysander Trischler)



# 2.40 - 2025-06-21

### Added

- `latestunread` feed sort order, which sorts feeds by their most recent unread
    article (#2492) (Daniel Lublin)
- contrib: "monochrome" colorscheme (Halano)

### Changed

- Bumped minimum supported GCC version to GCC 7
- Default `cleanup-on-quit` changed from `yes` to the new option `nudge`, which
    shows a message and waits for a keypress when unreachable items are found in
    the cache. This avoids accidental data loss (#1183) (Dennis van der Schagt)
- Updated translations: Chinese (CookiePieWw), Dutch (Dennis van der Schagt),
    German (Lysander Trischler), Italian (Mauro Scomparin), Polish (Carno),
    Russian and Ukrainian (Alexander Batischev), Spanish (Roboron3042),
    Swedish (Dennis Öberg), Turkish (Emir SARI)
- Bumped minimum supported Rust version to 1.83.0
- Updated vendored library json.hpp to 3.12.0

### Removed

- Support for GCC 5 and 6, in line with previous deprecation of compilers that
    don't support C++17

### Fixed

- Crash when invoking `edit-flags` from a new-style binding (i.e. `bind`)
    (Dennis van der Schagt)
- Query feed configs not being read from the urls file when `urls-source` is set
    to `opml` (#3057) (Jorenar)
- Some remote APIs not reading tags from the urls file (Dennis van der Schagt)
- `exec:` and `filter:` feeds could be opened in the browser even though they
    don't have a URL (Juho Eerola)



## 2.39 - 2025-03-23

### Added

- New `bind` command, which is an improvement on both `bind-key` and `macro`.
    It allows multi-key bindings (that's new!) which execute one or more actions
    (similar to macros), and can have a description that'll be displayed in the
    help dialog (that's new too!). It also offers an additional syntax for
    specifying keys: instead of `^R`, one can write `<C-r>`. Multi-key bindings
    are slightly limited in that bindings with same prefix must all be the same
    length to avoid ambiguity when executing them (if I had bindings `for` and
    `fork`, and typed `for`, Newsboat wouldn't know if that's it or I'm going to
    type `k` next) (#1165) (Dennis van der Schagt, Alexander Batischev, Lysander
    Trischler)
- contrib/newsboat-idlefeeds.sh: a script to show feeds that weren't updated in
    a given amount of time (T3SQ8)

### Changed

- Highlighting in the help form is now case-insensitive (#2998) (Juho Eerola)
- Updated translations: Dutch (Dennis van der Schagt),
    German (Lysander Trischler), Italian (Mauro Scomparin), Polish (Carno),
    Russian and Ukrainian (Alexander Batischev), Swedish (Dennis Öberg),
    Turkish (Emir SARI)
- Bumped minimum supported Rust version to 1.81.0

### Deprecated

- Compilers that don't support C++17. This release compiles with `-std=c++17`,
    but only for compatibility with header files of libicu 75+; Newsboat itself
    only uses C++14 for now. The next release might start using C++17 features,
    so please open an issue if that'll cause problems for you (#3031) (Alexander
    Batischev)

### Removed

- Support for Clang 4, because it doesn't accept `-std=c++17` (#3031) (Alexander
    Batischev)

### Fixed

- Crash when setting a non-existent variable (#2989) (Dennis van der Schagt)
- Highlights not displayed for searches that contain less-than sign, due to
    errors in quoting (#3008) (Juho Eerola)
- Building with libicu 75+, which requires C++17. libicu is an indirect
    dependency of Newsboat via libxml2 (#3031) (Alexander Batischev)



## 2.38 - 2024-12-22

Lists below only mention user-visible changes, but the full list of contributors
for this release also includes Juho Eerola.

### Added

- Shell completions for ZSH (Ludovico Gerardi)
- Keymap hint for "Clear filter" when filter is active (Dennis van der Schagt)
- Better logs when Miniflux authentication fails with something other than
    401 Unauthorized (Dennis van der Schagt)
- Support for enclosures with Miniflux (Anshul Gupta)

### Changed

- Shell completions are now installed into directories where shells are looking
    for them. Previously, completions were installed into doc/contrib (Ludovico
    Gerardi)
- Updated translations: Chinese (CookiePieWw), Dutch (Dennis van der Schagt),
    German (Lysander Trischler), Italian (Mauro Scomparin), Polish (Carno),
    Russian and Ukrainian (Alexander Batischev), Spanish (Roboron3042),
    Swedish (Dennis Öberg), Turkish (Emir SARI)
- Bumped minimum supported Rust version to 1.79.0

### Fixed

- `MKDIR` not being used in all the relevant places of the Makefile (Ludovico
    Gerardi)
- Dates far in the past or the future not being stored correctly (#2871) (Dennis
    van der Schagt)
- `cookie-cache` having not effect on `reload-all` (bound to `R` by default,
    also activated by `auto-reload`) (#2935) (Dennis van der Schagt)



## 2.37 - 2024-09-22

### Added

- contrib: a bookmark plugin for Readeck (Ada Wildflower)
- Podboat: podcasts can now be "missing", meaning a file was downloaded by
    Podboat but removed by some other program (Dennis van der Schagt)
- contrib: completions file for fish shell (Dennis van der Schagt)

### Changed

- Sped up `-x reload` a little by grouping feeds on the same domain. Other ways
    to trigger a reload already had this optimization (Juho Eerola)
- Sped up FreshRSS integration with regard to marking articles as read
    (fjebaker)
- Updated translations: Chinese (CookiePieWw), Dutch (Dennis van der Schagt),
    German (Lysander Trischler), Italian (Mauro Scomparin), Polish (Carno),
    Russian and Ukrainian (Alexander Batischev), Spanish (Roboron3042), Turkish
    (Emir SARI)
- Bumped minimum supported Rust version to 1.77.0
- Updated vendored library Catch2 to 3.7.0 (Alexander Batischev)

### Fixed

- Newsboat spamming sites with requests when `download-retries` is changed from
    its default of 1 and the site returns HTTP code 304 Not Modified (#2732)
    (Dennis van der Schagt)
- Crash on articles with combining Unicode characters (#2805) (Dennis van der
    Schagt)
- Crash on articles where author name ends in a closing parenthesis (Mikhail
    Yumanov)
- Cursor highlighting the wrong list entry after the terminal was resized
    (#2845) (Dennis van der Schagt)
- `highlight-article` with expressions that contain flags had no effect on
    unread articles (#2814) (Dennis van der Schagt)
- "Searching..." message not disappearing when the search is over (#2837)
    (Dennis van der Schagt)

### Security

- Fixed clickjacking vulnerability on newsboat.org (reported by Kunal Mhaske)



## 2.36.1 - 2024-09-22

### Fixed

- Newsboat spamming sites with requests when `download-retries` is changed from
    its default of 1 and the site returns HTTP code 304 Not Modified (#2732)
    (Dennis van der Schagt)



## 2.35.1 - 2024-09-22

### Fixed

- Newsboat spamming sites with requests when `download-retries` is changed from
    its default of 1 and the site returns HTTP code 304 Not Modified (#2732)
    (Dennis van der Schagt)



## 2.34.1 - 2024-09-22

### Fixed

- Newsboat spamming sites with requests when `download-retries` is changed from
    its default of 1 and the site returns HTTP code 304 Not Modified (#2732)
    (Dennis van der Schagt)



## 2.33.1 - 2024-09-22

### Fixed

- Newsboat spamming sites with requests when `download-retries` is changed from
    its default of 1 and the site returns HTTP code 304 Not Modified (#2732)
    (Dennis van der Schagt)



## 2.32.1 - 2024-09-22

### Fixed

- Newsboat spamming sites with requests when `download-retries` is changed from
    its default of 1 and the site returns HTTP code 304 Not Modified (#2732)
    (Dennis van der Schagt)



## 2.31.1 - 2024-09-22

### Fixed

- Newsboat spamming sites with requests when `download-retries` is changed from
    its default of 1 and the site returns HTTP code 304 Not Modified (#2732)
    (Dennis van der Schagt)



## 2.30.2 - 2024-09-22

### Fixed

- Newsboat spamming sites with requests when `download-retries` is changed from
    its default of 1 and the site returns HTTP code 304 Not Modified (#2732)
    (Dennis van der Schagt)



## 2.29.1 - 2024-09-22

### Fixed

- Newsboat spamming sites with requests when `download-retries` is changed from
    its default of 1 and the site returns HTTP code 304 Not Modified (#2732)
    (Dennis van der Schagt)



## 2.28.1 - 2024-09-22

### Fixed

- Newsboat spamming sites with requests when `download-retries` is changed from
    its default of 1 and the site returns HTTP code 304 Not Modified (#2732)
    (Dennis van der Schagt)



## 2.27.1 - 2024-09-22

### Fixed

- Newsboat spamming sites with requests when `download-retries` is changed from
    its default of 1 and the site returns HTTP code 304 Not Modified (#2732)
    (Dennis van der Schagt)



## 2.26.1 - 2024-09-22

### Fixed

- Newsboat spamming sites with requests when `download-retries` is changed from
    its default of 1 and the site returns HTTP code 304 Not Modified (#2732)
    (Dennis van der Schagt)



## 2.25.1 - 2024-09-22

### Fixed

- Newsboat spamming sites with requests when `download-retries` is changed from
    its default of 1 and the site returns HTTP code 304 Not Modified (#2732)
    (Dennis van der Schagt)



## 2.24.1 - 2024-09-22

### Fixed

- Newsboat spamming sites with requests when `download-retries` is changed from
    its default of 1 and the site returns HTTP code 304 Not Modified (#2732)
    (Dennis van der Schagt)



## 2.23.1 - 2024-09-22

### Fixed

- Newsboat spamming sites with requests when `download-retries` is changed from
    its default of 1 and the site returns HTTP code 304 Not Modified (#2732)
    (Dennis van der Schagt)



## 2.22.2 - 2024-09-22

### Fixed

- Newsboat spamming sites with requests when `download-retries` is changed from
    its default of 1 and the site returns HTTP code 304 Not Modified (#2732)
    (Dennis van der Schagt)



## 2.21.1 - 2024-09-22

### Fixed

- Newsboat spamming sites with requests when `download-retries` is changed from
    its default of 1 and the site returns HTTP code 304 Not Modified (#2732)
    (Dennis van der Schagt)



## 2.20.2 - 2024-09-22

### Fixed

- Newsboat spamming sites with requests when `download-retries` is changed from
    its default of 1 and the site returns HTTP code 304 Not Modified (#2732)
    (Dennis van der Schagt)



## 2.19.1 - 2024-09-22

### Fixed

- Newsboat spamming sites with requests when `download-retries` is changed from
    its default of 1 and the site returns HTTP code 304 Not Modified (#2732)
    (Dennis van der Schagt)



## 2.36 - 2024-06-22

### Changed

- Improved readability of `--help`: better alignment, replace unreadable ad hoc
    filenames with `<file>` (Dennis van der Schagt)
- Updated translations: Brazilian Portuguese (André L. C. Moreira),
    Chinese (CookiePieWw), Dutch (Dennis van der Schagt), German (Lysander
    Trischler), Italian (Mauro Scomparin), Polish (Carno), Russian and Ukrainian
    (Alexander Batischev), Swedish (Dennis Öberg), Turkish (Emir SARI)
- Updated vendored library Catch2 to 3.5.4 (Nikos Tsipinakis)
    expected-lite to 0.8.0

### Fixed

- Updated default color configuration in the docs (Xiao Pan)
- Parts of contrib/ not being installed (Dennis van der Schagt)

### Security

- Added DMARC policy for newsboat.org to better prevent spoofing (we already had
    an SPF record) (reported by Kunal Mhaske)



## 2.35 - 2024-03-24

Lists below only mention user-visible changes, but the full list of contributors
for this release also includes Daniel Oh and Lucio Sauer.

### Added

- Support for Feedbin (#716) (James Vaughan)
- `--queue-file`, `--search-history-file`, and `--cmdline-history-file` options
    to specify locations of the respective files. This, along with the existing
    options `--url-file`, `--config-file`, and `--cache-file`, lets the user
    fully customize the locations instead of relying on XDG or dotfiles (#57)
    (CookiePieWw)
- `miniflux-flag-star` setting, so articles flagged in Newsboat appear as
    starred in Miniflux (#2235) (gilcu3)
- `miniflux-show-special-feeds` setting (enabled by default) which adds a "Starred
    Items" feed to the feedlist for those two use Miniflux (gilcu3)
- Podboat: if `--log-level` is specified but `--log-file` isn't, write the log
    to a file named after the template `podboat_%Y-%m-%d_%H.%M.%S.log`, i.e. use
    the current date and time. The same functionality was added to Newsboat
    proper in 2.31 (Dennis van der Schagt)
- contrib: a bookmark plugin for Linkding (Mike Hall)
- contrib: a bookmark plugin for Wallabag (Mike Hall)
- contrib: a filter to turn twtxt protocol into RSS (Cyril Augier)
- contrib: a filter to add newlines to Slashdot feed (Dennis van der Schagt)

### Changed

- Build in C++14 mode. This does *not* increase our requirements for compilers,
    because the ones we require already provide C++14 support
- Updated translations: Chinese (CookiePieWw), Dutch (Dennis van der Schagt),
    German (Lysander Trischler), Italian (Mauro Scomparin), Polish (Carno),
    Russian and Ukrainian (Alexander Batischev), Swedish (deob83),
    Turkish (Emir SARI)
- Updated vendored library optional-lite to 3.6.0
- Bumped minimum supported Rust version to 1.72.1

### Fixed

- `%>[char]` format not working inside a conditional format
    (e.g. `%D %?T?%-63t%> %T&%t?`) (#2645) (Juho Eerola)



## 2.34 - 2023-12-25

Lists below only mention user-visible changes, but the full list of contributors
for this release also includes bogdasar1985, who made quite a few changes to
Newsboat internals (and continues work on even more!).

### Added

- `latest_article_age` feed attribute; it's similar to the `age` attribute of an
    article, and can be used to e.g. hide feeds which were recently updated
    (#2619) (Dennis van der Schagt)

### Changed

- Updated translations: Chinese (CookiePieWw), Dutch (Dennis van der Schagt),
    German (Lysander Trischler), Italian (Mauro Scomparin), Polish (Carno),
    Russian and Ukrainian (Alexander Batischev)
- Updated vendored library json.hpp to 3.11.3
- Bumped minimum supported Rust version to 1.70.0

### Fixed

- Articles in Mastodon feeds having no titles when using Newsboat with NextCloud
    News (#2573) (Dennis van der Schagt)



## 2.33 - 2023-09-24

### Added

- Support for `0`..`9` keys in dialog overview. If you never used dialog
    overview, it's invoked by `V` key by default and is very useful if you also
    employ `^V`/`^G`/`^X` to keep multiple dialogs open within Newsboat (Dennis
    van der Schagt)
- Support importing OPML 2.0 with `--import-from-opml` (#2448) (bogdasar1985)

### Changed

- Image enclosures are now displayed at the start of the article. This improves
    support for Mastodon feeds (#2305, #2495) (Dennis van der Schagt)
- In feeds that lack title and whose URLs end entirely in digits, use the
    description as a title instead. This improves support for Mastodon feeds and
    hopefully doesn't break others (#2530) (Martin Vilcans)
- Taught contrib/image-preview/nbrun to take Newsboat's CLI arguments (venomega)
- Updated translations: Dutch (Dennis van der Schagt), German (Lysander
    Trischler), Italian (Mauro Scomparin), Polish (Carno), Russian and Ukrainian
    (Alexander Batischev), Spanish (Roboron3042), Swedish (Dennis Öberg),
    Turkish (Emir SARI)
- Bumped minimum supported Rust version to 1.68.2

### Fixed

- Hanging if scripts used by `filter:`, `bookmark-cmd`, or `html-renderer`
    process their input in a streaming fashion rather than reading it entirely
    before outputting anything (Alexander Batischev)
- Slowness in opening and navigating feeds that contain many items (#229)
    (Dennis van der Schagt)
- `--export-to-opml` producing invalid OPML documents (missing `text` attribute)
    (#2518) (bogdasar1985)



## 2.32 - 2023-06-25

### Added

- contrib: a filter to remove Substack's "Subscribe now" prompts from the posts
    (NunoSempere)
- contrib: an image pager which can spot all the images in a post and display
    them with either `feh` or `kitty`'s `icat` (whyrgola)
- Tags in OPML export. This is implemented as a new option, `--export-to-opml2`,
    because the output format is OPML version 2.0 rather than the 1.0 that
    `--export-to-opml` produces. We intend to make OPML 2.0 the default in some
    future major release of Newsboat (#871) (Gwyneth Morgan)

### Changed

- Enclosures are only enqueued if their MIME type looks like a podcast or is
    empty. This prevents Newsboat from e.g. enqueueing images from Mastodon
    feeds (#2367) (Dennis van der Schagt)
- Asciidoctor is a truly optional dependency now. `make all` still builds docs,
    but at least one can `make newsboat` and such without installing Asciidoctor
    (#2353) (Alexander Batischev)
- Newlines are now removed from the author's name (#2434) (blankie)
- If the same URL is used in `<a>` and/or `<img>`/`<iframe>`, it's marked as
    "image" or "iframe" rather than simply a "link" (#2432) (blankie)
- `goto-title` operation now searches for the titles *as you see them*, i.e. if
    a feed doesn't have a title and Newsboat displays its URL instead,
    `goto-feed` will use that URL rather than the (empty) title (#2451)
    (blankie)
- Updated translations: Dutch (Dennis van der Schagt), German (Lysander
    Trischler), Italian (Mauro Scomparin), Polish (Carno), Russian and Ukrainian
    (Alexander Batischev), Swedish (Dennis Öberg), Turkish (Emir SARI)
- Updated vendored library expected-lite to 0.6.3
- Bumped minimum supported Rust version to 1.66.1

### Fixed

- Detection of Cargo and Asciidoctor: it succeeded even if these programs were
    missing (Alexander Batischev)



## 2.31 - 2023-03-26

Lists below only mention user-visible changes, but the full list of contributors
for this release also includes: Sergei Trofimovich and giuliano.

### Added

- Operations for scrolling by half a page (`halfpageup`, `halfpagedown`) (#36)
    (Dennis van der Schagt)
- If `--log-level` is specified but `--log-file` isn't, write the log to a file
    named after the template `newsboat_%Y-%m-%d_%H.%M.%S.log`, i.e. use the
    current date and time (Dennis van der Schagt)
- _contrib/move_url.py_ for moving feeds in Newsboat's database while keeping
    articles (blankie)

### Changed

- Updated translations: Dutch (Dennis van der Schagt), German (Lysander
    Trischler), Italian (Mauro Scomparin), Polish (Carno), Russian and Ukrainian
    (Alexander Batischev), Spanish (Roboron3042), Turkish (Emir SARI)
- Bumped minimum supported Rust version to 1.64.0

### Fixed

- Inability to exit search mode when `quit` is bound to `BACKSPACE` (#2336)
    (Dennis van der Schagt, Alexander Batischev)



## 2.30.1 - 2022-12-30

### Fixed

- Build failure with curl 7.87.

  It was caused by us using a deprecated curl constant, which in 7.87 started
  emitting a warning. Since we turn warnings into errors (-Werror), this failed
  the build. This wasn't spotted by our CI because curl 7.87 only came out a few
  days before Newsboat 2.30 did, and our CI uses Ubuntu LTS which doesn't pull
  updates *that* fast.

  The fix replaces the deprecated constant with a newer one. This also required
  a bump of minimum supported curl version from 7.21.6 (released 16 June 2010)
  to 7.32.0 (released 11 August 2013), which shouldn't affect anyone because of
  how low the new requirement is.

  (#2297) (Dennis van der Schagt)



## 2.30 - 2022-12-25

Lists below only mention user-visible changes, but the full list of contributors
for this release also includes: Arttano, Danny Kirkham, Juho Eerola, and Pepe
Doval.

### Added

- In config, long lines can now be broken into multiple ones with `\`. When
    parsing the config, Newsboat would remove the backslash and append the
    following line to the current one. Be careful when indenting the lines that
    follow the backslash, as the indentation will be included in the
    concatenated string; this can change the meaning of some commands, e.g. if
    the indentation ends up inside a regex (#2212) (Simon Farre)
- `article-feed` operation, to go to the feed of the currently selected article.
    This can come in handy in query feeds (phire)
- New placeholder for `browser`, `%T`, which is replaced by the title of the
    selected feed or item (#2224) (Aneesh)
- Miniflux: fail on startup if credentials are wrong (#2220) (Dennis van der
    Schagt)
- Miniflux: support for API token authentication, which is available since
    Miniflux 2.0.21 and is the preferred authentication method:
    https://miniflux.app/docs/api.html#authentication See `miniflux-token`,
    `miniflux-tokeneval`, and `miniflux-tokenfile` settings (#2122) (Dennis van
    der Schagt)

### Changed

- Bumped minimum supported Rust version to 1.62.0
- When `cleanup-on-quit` is disabled and the cache contains unreachable feeds,
    print their number, and write their URLs to `error-log` (#1548) (Maximilian
    Winkler)
- If an item contains enclosure(s) but doesn't specify their type(s), pick the
    last one (as Newsboat only displays a single enclosure per item). This won't
    always do the right thing, e.g. it could pick cover art instead of the
    podcast, but *sometimes* it will, so it's still better than nothing (#2050)
    (Dennis van der Schagt)
- Updated translations: Dutch (Dennis van der Schagt), German (Lysander
    Trischler), Italian (Mauro Scomparin), Polish (Carno), Russian (Alexander
    Batischev), Turkish (Emir SARI), Ukrainian (Alexander Batischev)
- Updated vendored library Catch2 to 2.13.10

### Fixed

- Atom: if an item doesn't specify the `atom:author`, use the field from the
    contained `atom:source`; if that's not specified either, use `atom:author`
    of the feed itself. This is dictated by RFC 4287 §4.2.1 (#2256) (Lysander
    Trischler)

### Security

- Remove transitive dependency on the vulnerable `time` 0.1 crate:
    CVE-2020-26235 https://osv.dev/vulnerability/RUSTSEC-2020-0071 (#2288)
    (Alexander Batischev, thanks to the prod from critkitten)



## 2.29 - 2022-09-25

### Added

- New placeholder for `browser`, `%t`, which is replaced by the type of the URL
    (#1954) (blank X)
- contrib: a script to reorder lines in the `urls` file (#1918) (T3SQ8)
- Support for Brotli compression. In fact, Newsboat will now use all the methods
    supported by the libcurl it's linked to (#2152) (Dennis van der Schagt)
- In tags dialog, put the cursor on the currently selected tag, or the first tag
    if none is selected at the moment (#2093) (Dennis van der Schagt)

### Changed

- When rendering an article, put a newline between consecutive `<audio>` and
    `<video>` tags (#2103) (blank X)
- When `select-filter` is used with an argument (e.g. from a macro), that
  argument is now used to look up a predefined filter by name. The old
  behaviour (applying the argument as filter) is still available by using
  `set-filter` instead (#2137) (Dennis van der Schagt)
- When `select-filter` is used without an argument, it will now open the filter
  selection dialog. Previously, `set-filter` from a macro without arguments
  was ignored in the article list (#2137) (Dennis van der Schagt)
- Docs: use a new style for keys. It should be more readable than the old one,
    please file issues if you disagree! (#2028) (Dennis van der Schagt)
- When reloading feeds in parallel, status line now shows the progress rather
    than the number of the currently reloaded feed (#2065) (Juho Eerola)
- Updated translations: Dutch (Dennis van der Schagt), French (Tonus), German
    (Lysander Trischler), Italian (Mauro Scomparin), Polish (Carno), Russian and
    Ukrainian (Alexander Batischev), Spanish (Roboron3042), Turkish (Emir SARI)
- Bumped minimum supported Rust version to 1.59.0
- Bumped minimum supported GNU Make version to 4.0 (released on
    October 9th, 2013)
- Updated vendored libraries: expected-lite to 0.6.2, json.hpp to 3.11.2

### Fixed

- Segfault on `sqlite3DbMallocRawNN` (#1980) (Juho Eerola)
- Scrolling when toggling `show-read-feeds` (#2138) (Dennis van der Schagt)
- Feeds not reloading in parallel when reloading only visible feeds (#2067)
    (Juho Eerola)



## 2.28 - 2022-06-26

Lists below only mention user-visible changes, but the full list of contributors
for this release also includes: Jan Staněk, blank X, and sebashwa.

### Added

- Ability to go back to the previous set of search results. This introduces
    a new dialog, `searchresultslist`, which supports a new operation,
    `previoussearchresults` (bound to `z` key by default). The
    `searchresult-title-format` setting now supports one more placeholder, `%s`,
    which is replaced by the term you searched for (#1785, #2043) (bogdasar1985)
- Support for `enqueue` operation in the article list (#2023) (Dennis van der
    Schagt)
- New placeholder for `articlelist-format`, `%e`, which is replaced by the URL
    of the article's enclosure (#2023) (Dennis van der Schagt)

### Changed

- If an article has multiple links, `open-in-browser` will open the HTTP or
    HTTPS one. If article has no such links, then the last one to appear in the
    feed's source is picked. (Note: this is about the link that you see in the
    item's header, prefixed by "Link:" -- NOT about the links in the article
    itself) (#2060) (bogdasar1985)
- Updated translations: Brazilian Portuguese (Alexandre Provencio), Dutch
    (Dennis van der Schagt), German (Lysander Trischler), Italian (Mauro
    Scomparin), Polish (Michał Siemek), Russian, Ukrainian (Alexander
    Batischev), Spanish (Roboron3042), Turkish (Emir SARI)
- Bumped minimum supported Rust version to 1.57.0
- Updated vendored library Catch2 to 2.13.9, expected-lite to 0.6.0

### Removed

- Snap: i386 support. The package is now based on Ubuntu 20.04, which doesn't
    support i386 (#2058)

### Fixed

- Stop scrolling to top of article when window is resized (#1298) (Dennis van
    der Schagt)
- NextCloud News 18.1.0+: crashes when reloading feeds that have no author or
    title (#2102) (Alexander Batischev)
- Snap: "Error opening terminal: xterm-kitty". This fix comes at the cost of
    i386 support in Snap (#2058) (Gianluca Della Vedova, Alexander Batischev)



## 2.27 - 2022-03-22

### Added

- Support for regexes in `ignore-article`; for example, you can now use
    `ignore-article "https://nitter.net/.*" "title =~ \"RT by\""` to ignore all
    retweets in your Nitter feeds. This is more efficient than matching on
    `feedurl` from the filter expression, because this new form is only
    evaluated for the feeds that match the regex, while the old form would run
    for all articles of all feeds (#1913) (duarm)
- A "universal" color scheme (Yurii H)
- A user-contributed script to show images in Kitty terminal emulator:
    contrib/kitty-img-pager.sh (Timm Heuss)
- Support for XDG directories in contrib/exportOPMLWithTags.py (frogtile)

### Changed

- Bumped minimum supported Rust version to 1.55.0
- We now link to our own STFL fork: https://github.com/newsboat/stfl. The
    upstream's SVN is down, and we never managed to get any of our bugfixes in
    there anyway. Since we seem to be the last remaining STFL user, we advise
    downstream maintainers to rely on our repo instead. Our fork is maintained to
    the extent necessary for Newsboat
- In manpages, command line options are now set in bold and are underlined,
    while inline code is underlined. These changes improve readability (Lysander
    Trischler)
- Clarified error messages in OPML import (#1919) (bogdasar1985)
- Updated translations: Dutch (Dennis van der Schagt), French (Tonus), German
    (Lysander Trischler), Hungarian (maxigaz), Italian (Mauro Scomparin), Polish
    (Michał Siemek), Russian, Ukrainian (Alexander Batischev), Spanish (Roboron3042),
    Turkish (Emir SARI)
- Updated vendored library json.hpp to 3.10.5, Catch2 to 2.13.8

### Fixed

- Cursor jumping too far after marking a feed read when hidden feeds are present
    (#1934) (Dennis van der Schagt)
- Newsboat exiting with code 0 when OPML import fails (bogdasar1985)
- XML entities not being decoded in "text/plain" entries (#1938) (bogdasar1985)
- Crash when "ol" or "ul" tags are closed multiple times (#1974) (Dennis van der
    Schagt)
- Confirmation and Q&A text being almost invisible with "nord" color scheme
    (Daryl Manning)
- Some invalid code in contrib/exportOPMLWithTags.py (frogtile)
- Blank lines inside tags not being preserved when inside a "pre" tag (#2003)
    (blank X)
- A potential crash in HTML entities decoding code (blank X)

### Security

- Addressed CVE-2022-24713 (a.k.a. RUSTSEC-2022-0013) by updating "regex" crate
    to 1.5.5 (#2008) (Alexander Batischev)



## 2.26 - 2021-12-27

Lists below only mention user-visible changes, but the full list of contributors
for this release also includes: Amrit Brar, Ben Armstead, Max, Simon Farre, and
twittumz.

### Added

- `confirm-mark-feed-read` setting (enabled by default) (#1781) (aflyingpumpkin)
- `miniflux-min-items` setting to control how many articles to load for each
    feed (Damian Korbuszewski)
- A script to bookmark articles to buku (Greg Fitzgerald)
- A gruvbox color scheme (Greg Fitzgerald)

### Changed

- Bumped minimum supported Rust version to 1.53.0
- Accept empty feed title when bookmarking on autopilot (#243) (Dennis van der
    Schagt)
- Do not show ignored articles in search results (#1812) (Q-I-U)
- `:save` command now uses `save-path` setting to resolve relative paths (#1689)
    (Q-I-U)
- Updated translations: Dutch (Dennis van der Schagt), German (Lysander
    Trischler), Italian (Mauro Scomparin), Polish (Michał Siemek), Russian,
    Ukrainian (Alexander Batischev), and Turkish (Emir Sari)
- Updated vendored library optional-lite to 3.5.0, json.hpp to 3.10.4

### Removed

- Ability to switch away from modal dialogs (e.g. FileBrowser) (#117) (Dennis
    van der Schagt)

### Fixed

- Relative URLs in articles are now resolved relative to their permalink when
    the feed doesn't set the `xml:base` (#1818,
    https://github.com/akrennmair/newsbeuter/issues/507) (Alexander Batischev)
- Starred items not being synchronized to NextCloud News (#743) (Dennis van der
    Schagt)



## 2.25 - 2021-09-20

Lists below only mention user-visible changes, but the full list of contributors
for this release also includes: Amrit Brar, noctux, and shenlebantongying.

### Added

- New elements to style with `color` command: `title`, `hint-key`,
    `hint-keys-delimiter`, `hint-separator`, and `hint-description`. If there is
    no style for one of those elements, the style of `info` is used instead. See
    "Configuring Colors" section in the docs (Alexander Batischev)
- Podboat: show an error when a podcast can't be renamed (#545) (Kartikeya
    Gupta)
- A note that the urls file can contain comments (Lysander Trischler)

### Changed

- Bumped minimum supported Rust version to 1.51.0
- Key hints (at the bottom of the screen) are now styled to make them easier to
    read. If you're using a custom colorscheme and want to use these new
    elements, you need to update it; see the "Configuring Colors" section in the
    docs (#1016) (Alexander Batischev)
- Podboat: move to the next item after marking a podcast finished or deleted
    (Dennis van der Schagt)
- `purge-deleted` now keeps the cursor near where it was in the list, rather
    than the *line* it was on (#1728) (Allan Wind)
- Updated translations: Dutch (Dennis van der Schagt), German (Lysander
    Trischler), Italian (Mauro Scomparin), Polish (Michał Siemek), Russian,
    Ukrainian (Alexander Batischev), and Turkish (Emir Sari)
- Updated vendored library Catch2 to 2.13.7, and json.hpp to 3.10.2

### Fixed

- A crash when entering a feed with an invalid filter expression (#1665)
    (Alexander Batischev)
- Slow scrolling in Podboat (#1375) (Dennis van der Schagt)
- Slow `mark-all-read` with remote APIs. It's still slow when a filter is
    applied, but without a filter it's as fast as possible (Alexander Batischev)
- A crash when `next-unread` is invoked while viewing a tag with no unread feeds
    (#1734) (Alexander Batischev)
- A crash on feeds that contain wide characters (e.g. emojis) (mcz)



## 2.24 - 2021-06-20

Lists below only mention user-visible changes, but the full list of contributors
for this release also includes Alexandre Alapetite.

### Added

- FreshRSS support (Petra Lamborn)
- Ability to add descriptions to macros (#228) (Dennis van der Schagt)
- Support for plain-text Atom entries (YouTube is the most prominent publisher
    of those) (#468, #1022, #1010) (Dennis van der Schagt)
- `restrict-filename` setting to control if non-alphanumeric symbols will get
    replaced by underscores when saving an article (#1110) (crimsonskylark)
- `highlight-feed` setting to highlight feedlist entries according to the filter
    expressions (same as `highlight-article` in the article list) (#1510)
    (Vonter)

### Changed

- IRC channel moved from Freenode to Libera.Chat network
- Newsboat will now fail to start if settings are passed more parameters than
    they expect. To fix this, read the doc for the setting and try using double
    quotes as necessary (Alexander Batischev)
- Newsboat will now quit if you try to open a query feed whose expression
    contains an unknown attribute. This is a temporary workaround; the next
    version will display an error instead. The proper fix couldn't be added to
    2.24 because that'd disrupt the freeze on translations (#1665) (Dennis van
    der Schagt)
- Updated translations: Dutch (Dennis van der Schagt), German (Lysander
    Trischler), Italian (Mauro Scomparin), Polish (Michał Siemek), Russian
    (Alexander Batischev), Turkish (Emir Sari), Ukrainian (Alexander Batischev)
- Bumped minimum supported Rust version to 1.48.0
- Updated vendored library Catch2 to 2.13.6

### Fixed

- Segfault in Podboat when purging the list where the last item is finished or
    deleted (#1546) (Dennis van der Schagt)
- `mark-feed-read` applying to invisible items (e.g. the ones that are filtered
    out, or read ones when `show-read-articles` is in effect) (#1364) (Dennis
    van der Schagt)
- Supplying garbage instead of a correct Newsboat version to FeedHQ and The Old
    Reader (Alexander Batischev)
- Sorting in ascending order partially reversing the results of the previous
    sort (#1561) (tau3)
- Compile errors when building with a stack protector (#1598) (Alexander
    Batischev)
- `toggle-article-read` ignoring its argument in macros when executed from an
    article view (#1637) (ysh16)
- Article view scrolling back to top after opening a link (#1463) (Dennis van
    der Schagt)
- The build system trying to find `iconv()` in libc instead of linking with
    libiconv (Theo Buehler)



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
- `newsboat -x` not reporting that another instance is already running (#483)
    (Dennis van der Schagt)
- Itemlist refreshing while a macro is executing, causing the macro to operate
    on the wrong items (#70) (Dennis van der Schagt)



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
