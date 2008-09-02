#ifndef NEWSBEUTER_CONFIG__H
#define NEWSBEUTER_CONFIG__H

#define PROGRAM_NAME		PACKAGE
#define PROGRAM_VERSION		"1.3"
#define PROGRAM_URL			"http://www.newsbeuter.org/"
#define USER_AGENT			PROGRAM_NAME " rss feedreader " PROGRAM_VERSION " (" PROGRAM_URL ")"

#define NEWSBEUTER_PATH_SEP			"/"
#define NEWSBEUTER_CONFIG_SUBDIR	".newsbeuter"

#ifdef _ENABLE_NLS
#	include <libintl.h>
#	include <locale.h>

#	ifdef _
#		undef _
#	endif

#	define _(string) gettext(string)
#else
#	define _(string) (string)
#endif


#endif
