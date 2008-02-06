/* nXml - Copyright (C) 2005-2007 bakunin - Andrea Marchesini 
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

#include "_nxml.h"

const char *
nxml_strerror (nxml_t * nxml, nxml_error_t err)
{
  switch (err)
    {
    case NXML_OK:
      return "Success";

    case NXML_ERR_PARSER:
      return "Parser error";

    case NXML_ERR_DOWNLOAD:
      return nxml
	&& nxml->priv.curl_error ? curl_easy_strerror (nxml->priv.
								curl_error) :
	"Download error";

    case NXML_ERR_DATA:
      return "No correct paramenter in the function";

    default:
      return strerror (errno);
    }
}

CURLcode
nxml_curl_error (nxml_t * nxml, nxml_error_t err)
{
  if (!nxml || err != NXML_ERR_DOWNLOAD)
    return CURLE_OK;

  return nxml->priv.curl_error;
}

nxml_error_t
nxml_line_error (nxml_t * nxml, int *line)
{
  if (!nxml || !line)
    return NXML_ERR_DATA;

  *line = nxml->priv.line;

  return NXML_OK;
}

/* EOF */
