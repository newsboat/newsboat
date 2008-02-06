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

#include "_mrss.h"
#include "mrss_internal.h"

char *
__mrss_download_file (nxml_t * nxml, const char *fl, size_t * size,
		      mrss_error_t * error, CURLcode * code)
{
  char *buffer;

  if (code)
    *code = CURLE_OK;

  switch (nxml_download_file (nxml, fl, &buffer, size))
    {
    case NXML_OK:
      return buffer;

    case NXML_ERR_DOWNLOAD:

      if (code)
	*code = nxml_curl_error (nxml, NXML_ERR_DOWNLOAD);

      *error = MRSS_ERR_DOWNLOAD;
      return NULL;

    default:
      *error = MRSS_ERR_POSIX;
      return NULL;
    }
}

/* EOF */
