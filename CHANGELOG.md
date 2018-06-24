# Changes for Newsboat

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
- "Author" field for items fetched from Newsblur (Chris Nehren)
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
- When used with Newsblur, check on startup if cookie-cache exists or can be
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
- CJK text is wrapped at correct code-point boundaries (#71) (nmtake)
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
- Notify users if Newsblur feed they're subscribed to no longer exists (#494)
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
