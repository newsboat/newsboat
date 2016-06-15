# Changes for Newsbeuter

## 2.9 - 2015-02-19

### Added
- Support for FeedHQ

### Changed
- Update to Brazilian Portuguese translation (#126)
- Code base now uses C++11
- Don't override feed titles for hidden feeds
- Don't render inline images (#154)

### Fixed
- Custom keybindings in tag and filter selection dialogs (#78)
- Incorrect reloading of tags after editing the urls file
- Dumpconfig
- Missing variable in log output (#124)
- Type of configuration variables to path where appropriate (#125)
- Crash when GUID is lost (#127)
- Dependency check (#132)
- Segfault in `jump_to_next_unread_item` (#133)
- Feeds appearing empty due to variable shadowing issue
- Catch an exception that might be thrown by the "killfile" function



## 2.8 - 2014-01-19

### Added
- Bookmark script for getpocket.com support (Andreas Happe)
- Support for The Old Reader
- Support for NewsBlur (Thomas Weißschuh)

### Changed
- In the feed list, quitting now is the same as clearing the tag in case a tag
  is selected
- Make guid generation smarter (Jochen Sprickerhof)
- Update French translation (Sabrina Dubroca)

### Fixed
- Issues with the build process on OpenBSD (Kyle Isom)
- Crash bug in filter expression parsing
- Crash bug with unbalanced `<ol>` tags (Richard Quirk)



## 2.7 - 2013-08-27

### Added
- Option to colorize unread messages (Patrick Steinhardt)
- Option to swap title and hints bar (Patrick Steinhardt)
- `%u` and `%t` support to itemview (Giuliano Schneider)

### Changed
- Only force redraw if a form action is active (Patrick Steinhardt)

### Fixed
- Crash bug



## 2.6 - 2013-03-19

### Added
- Support for `<q>` and `<aside>` tags (Daniel Aleksandersen)
- Norwegian bokmål translation (Daniel Aleksandersen)

### Changed
- Style table headers in bold (Daniel Aleksandersen)
- Updated Russian translation (Justin Forest)
- Updated Polish translation (Michal Siemek)
- More compact default user-agent on Mac OS X (Daniel Aleksandersen)
- Remove all soft-hyphens (Daniel Aleksandersen)

### Fixed
- Crash in RSS parser (thanks to Isaac Good)
- Authentication issues with Google Reader (thanks to Fabrice Noilhan)
- Bug in Google authentication (Daniel Aleksandersen).



## 2.5 (2012-01-06):

### Added
- `download-full-page` configuration option
- Configuration option to use external URL viewer (#242)
- Ability to store Google Reader password in an external file (#239)
- Tiny Tiny RSS support (#243)
- `delete-read-articles-on-quit` option

### Changed
- HTTP authentication method is now configurable (#247)

### Fixed
- Rendering of nested `<ol>` lists



## 2.4 - 2011-02-01

### Added
- Support for query feeds in combination with Google Reader support
- Ability to configure proxy authentication method
- `-q` flag to enable quiet startup (Isaac Good)
- Keys to jump to the next/previous feed or article regardless of its "unread"
  status (Jim Pryor)
- XDG Base Directory support (Elrond)
- `cookie-cache` configuration option (#234)
- On-demand loading of feeds to reduce memory usage
- Feedlist re-sorting after all feeds have been reloaded
- Support for Google Reader offline mode

### Removed
- Bloglines support, as the service shuts down on October 1, 2010

### Fixed
- Google Reader authentication issue with certain passwords (#238)



## 2.3 - 2010-06-24

### Changed
- Made newsbeuter silent on lockfile errors when '-x' option is used

### Fixed
- HTML rendering of bold and underline text when light background is configured
- issues #192, #194, #197, #198, #199, #200, #201, #202, #210, #216
- Google Reader authentication (by Seth Mason)



## 2.2 - 2010-03-14

### Added
- Google Reader support
- Article highlighting in article list based on the article content (#174)
- "Hard quit" key to immediately quit from Newsbeuter (Jim Pryor)
- "Download status" format specifier for feedlist-format (#181).
- HTML table renderer (Stefan Erben)
- `open-in-browser-and-mark-read` key (Isaac Good)

### Changed
- "Ignore article" functionality extended with different ignore modes
  (download/display; #52)

### Fixed
- Issues #90, #160, #161, #168, #169, #171, #179, #180, #184
- Issues #183, #188 (Stefan Erben)



## 2.1 - 2009-12-08

### Added
- Support for `dc:creator` tag for RSS 2.0 parser
- When entering a feed, the first unread article is automatically selected (can
  be turned off with `goto-first-unread no`)
- When marking a feed read, move the selection to the next feed (unless a filter
  is currently applied)
- Length field to article list format (Stefan Erben)
- Support for 256-color terminals
- `dumpform` commandline command as a debugging aid
- Allow deletion of articles from article view
- Support for SOCKS proxies
- `notify-beep` notification beep (Vern Sun)
- Key to quickly jump to article URLs above #10 (Stefan Erben)

### Changed
- Improved HTML rendering (Stefan Erben)

### Fixed
- Podbeuter ignored `use-proxy` configuration command (#144)



## 2.0 - 2009-04-21

### Added
- More flexible dialog handling
- Ability to specify a list of OPML URLs when using OPML as URL source
- `keep-articles-days` config option to optionally keep articles only for
  a limited number of days
- `bookmark-interactive` config option to indicate that the configured
  bookmarking command is interactive
- Ability to search for text from the article view
- Basic support for Yahoo Media RSS
- `:source` commandline command to (re)load configuration files
- `age` attribute for articles to filter them for relative age (in days)
- Ability to configure local files as feeds
- `random-unread` key to go to a random unread article
- Ability to sort feed list and article list by interactively choosing the sort
  method
- `pipe-to` key to pipe articles to external commands
- Backtick evaluation for configuration files
- Commandline completion
- `between` operator for filter language
- `set` commandline command can now toggle boolean variables and reset
  configuration variables of all types to their default
- Commandline and search history persistence

### Changed
- Improved position handling in article list (#112) (Isaac Good)
- Replaced mrss with new RSS/Atom parser
- Made article view pager configurable
- Improved HTML rendering of links and underlined and bold text
- When opening articles from a search result dialog, make search phrase stand
  out in article view.
- Improved help dialog so that it now shows unbound functions
- Improved and extended conditional HTTP download handling

### Fixed
- A lot of bugs (#102, #111, #117, #130, #131)
- Don't display authentication information in URLs (#121)



## 1.3 - 2008-12-06

### Added
- Placeholders for `download-path` (#46)
- Ability to edit the list of subscribed URLs from newsbeuter through a text
  editor
- A file format to exchange information about read articles between different
  newsbeuter instances
- `feed-sort-order` configuration option to sort the feed list by the first tag
- Ability to toggle read flag from article view (Isaac Good)
- Ability to configure the number of parallel reload threads (#101)

### Changed
- Some internal data structures now use smart pointers (stability improvement)
- Extended podbeuter to keep finished downloads in the queue until they've been
  played
- Extended keymap to allow dialog-specific configuration
- Extended macros to enable modification of configuration variables



## 1.2 - 2008-09-02

### Added
- `download-timeout` and `download-retries` config options to make newsbeuter
  more reliable over unreliable connection (#88)

### Changed
- Improved whitespace handling in XML parser (fixes Debian issue #496765)

### Fixed
- Crash in case of invalid color/attribute names in the configuration
- Broken `open-in-browser` operation for URLs that contained a single quote
  (fixes Debian issue #497495; fixes incomplete security fix)



## 1.1 - 2008-09-01

### Added
- Line wrap for the article view's headers and the link list on the bottom
  (fixes Debian issue #491122)
- Test suite for functional tests of the user interface

### Security
- Fixed potential security issue when opening article URLs with the configured
  browser (thanks to J.H.M. Dassen (Ray) for pointing out)



## 1.0 - 2008-08-20

### Added
- Support for highlighting of regular expressions
- Search function in help dialog
- `show-read-articles` configuration option to toggle displaying of read
  articles
- `always-download` configuration option to configure a list of feed URLs for
  which newsbeuter ignores the Last-Modified timestamp
- Reading progress display in article view
- Optional format string support for `browser` configuration option
- `reset-unread-on-update` configuration command



## 0.9.1 - 2008-05-12

### Added
- Ability to open feed's link by pressing the `open-in-browser` key in feed list.

### Fixed
- Issue with filter feeds
- Issue with RFC-822 date parsing where the year only had 2 digits



## 0.9 - 2008-05-01

### Added
- Commandline option to podbeuter to automatically start download
- `article-sort-order` configuration option to freely configure the sort order
  of article lists
- Ability to delete articles

### Changed
- Improved locking to allow multiple newsbeuter instances (one instance per
  cache file)
- Flagged articles don't get deleted anymore

### Fixed
- Lots of bug fixes



## 0.8.2 - 2008-03-16

### Fixed
- Broken string conversion



## 0.8.1 - 2008-03-12

### Fixed
- Crash (related to string conversion of format string support)



## 0.8 - 2008-03-07

### Added
- Custom configurability of feed list and article list format
- Special tags to rename feeds
- Macro support
- Ruby scripting support

### Changed
- Improved reload speed by checking the Last-Modified header
- Directly integrated nxml/mrss code since API and ABI are a moving target



## 0.7 - 2007-09-18

### Added
- Possibility to predefine filters
- Bloglines synchronization support
- OPML online subscription support
- Plugin-based bookmarking support
- Custom flagging of articles
- Implemented more key commands to ease navigation even more
- Possibility to optionally use an external HTML renderer

### Changed
- Redesigned search function



## 0.6 - 2007-08-14

### Added
- Support for reloading the urls file
- Query feeds
- History for the most important input fields
- Additional commandline commands

### Fixed
- Major bug with filtering in the item list
- OPML import functionality



## 0.5 - 2007-08-02

### Added
- Unicode compatibility
- Support for notifications
- Filter language
- It is now possible to freely configure e.g. the up/down-keys

### Changed
- Improved HTML rendering
- Improved lock file handling



## 0.4 - 2007-05-08

### Added
- Configuration option to disable cache cleanup (user request)
- Configuration option to set the HTTP user-agent header to a custom value
- Italian translation (Andrea Marchesini)
- Unit tests
- `include` configuration command to make it possible to separate the
  configuration into several files
- Global configuration file /etc/newsbeuter/config
- Support for Snownews/Liferea extension scripts

### Changed
- Refactored view
- Significant speed improvement for reload and cache cleanup (Jürgen Jung)



## 0.3 - 2007-03-26

### Added
- gettext support
- Podcast support

### Changed
- Now, everything is stored as UTF-8 internally, and gets converted on-the-fly

### Removed
- Dependency to `libidn`

### Fixed
- Numerous bugs



## 0.2 - 2007-02-21

### Added
- Possibility to use the space key for key bindings
- Possibility to view all URLs within an article and open them in a browser
- HTTP proxy support
- Color configuration support
- Tagging/categorization support
- Auto-reload support
- Search dialog

### Changed
- `next unread` function now works across feeds when in item view
- Improved HTML rendering (occasional missing spaces, `<pre>` tags)

### Removed
- Unnecessary mutex lock/unlock that made newsbeuter lock up when the
  `max-items` config option was set



## 0.1.1 - 2007-01-17

### Fixed
- Crash when ISO-8859-1 encoded feeds with umlauts in the title were displayed
  on systems with UTF-8 locales enabled



## 0.1 - 2007-01-16

### Added
- This is an initial release of Newsbeuter
