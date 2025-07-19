#ifndef NEWSBOAT_CONFIG_H_
#define NEWSBOAT_CONFIG_H_

#include "filepath.h"

#define PACKAGE				"newsboat"
#define PROGRAM_NAME			"Newsboat"

#define PROGRAM_URL			"https://newsboat.org/"

const newsboat::Filepath NEWSBEUTER_CONFIG_SUBDIR = newsboat::Filepath::from_locale_string(".newsbeuter");
const newsboat::Filepath NEWSBEUTER_SUBDIR_XDG = newsboat::Filepath::from_locale_string("newsbeuter");
const newsboat::Filepath NEWSBOAT_CONFIG_SUBDIR = newsboat::Filepath::from_locale_string(".newsboat");
const newsboat::Filepath NEWSBOAT_SUBDIR_XDG = newsboat::Filepath::from_locale_string("newsboat");

#include <libintl.h>
#include <locale.h>

#ifdef _
#undef _
#endif

#define _(string) gettext(string)

#ifdef _s
#undef _s
#endif

#define _s(msg) std::string(gettext(msg))

// Use this macro instead of `_` and `_s` if you want the string to get extracted
// by xgettext, but don't want to call gettext(). This is useful for
// initializers of statically-allocated global objects, where gettext() calls
// operate with default (C) locale and are useless.
#define translatable(msg) msg

/* #define NDEBUG */ // only enable this #define if you want to disable all debug logging.

#endif /* NEWSBOAT_CONFIG_H_ */
