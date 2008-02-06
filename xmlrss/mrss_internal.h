/* mRss - Copyright (C) 2005-2007 bakunin - Andrea Marchesini 
 *                                    <bakunin@autistici.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef __M_RSS_INTERNAL_H__
#define __M_RSS_INTERNAL_H__

#include <curl/curl.h>
#include "_nxml.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>

#ifdef USE_LOCALE
#  ifdef USE_X_LOCALE
#    include <xlocale.h>
#  endif

#  ifdef USE_GENERIC_LOCALE
#    include <locale.h>
#  endif
#endif

char *	__mrss_download_file	(nxml_t *, const char *, size_t *, mrss_error_t *, CURLcode *code);

#endif

/* EOF */

