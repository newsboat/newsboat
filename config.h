#ifndef NEWSBOAT_CONFIG_H_
#define NEWSBOAT_CONFIG_H_

#define PACKAGE				"newsboat"
#define PROGRAM_NAME			PACKAGE

#define STRINGIFY_HELPER(str) #str
#define STRINGIFY(str) STRINGIFY_HELPER(str)

#define NEWSBOAT_VERSION_MAJOR 2
#define NEWSBOAT_VERSION_MINOR 15
#define NEWSBOAT_VERSION_PATCH 0
#define REAL_VERSION \
	STRINGIFY(NEWSBOAT_VERSION_MAJOR) \
	"." \
	STRINGIFY(NEWSBOAT_VERSION_MINOR) \
	"." \
	STRINGIFY(NEWSBOAT_VERSION_PATCH)

#ifdef GIT_HASH
#define PROGRAM_VERSION			GIT_HASH
#else
#define PROGRAM_VERSION			REAL_VERSION
#endif

#define PROGRAM_URL			"https://newsboat.org/"

#define NEWSBEUTER_PATH_SEP			"/"
#define NEWSBEUTER_CONFIG_SUBDIR	".newsbeuter"
#define NEWSBEUTER_SUBDIR_XDG		"newsbeuter"
#define NEWSBOAT_CONFIG_SUBDIR	".newsboat"
#define NEWSBOAT_SUBDIR_XDG		"newsboat"

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

/* #define NDEBUG */ // only enable this #define if you want to disable all debug logging.

#endif /* NEWSBOAT_CONFIG_H_ */
