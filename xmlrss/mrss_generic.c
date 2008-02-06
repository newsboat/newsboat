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

const char *
mrss_strerror (mrss_error_t err)
{
  switch (err)
    {
    case MRSS_OK:
      return "Success";

    case MRSS_ERR_PARSER:
      return "Parser error";

    case MRSS_ERR_DOWNLOAD:
      return "Download error";

    case MRSS_ERR_VERSION:
      return "Version error";

    case MRSS_ERR_DATA:
      return "No correct paramenter in the function";

    default:
      return strerror (errno);
    }
}

const char *
mrss_curl_strerror (CURLcode err)
{
  return curl_easy_strerror (err);
}

mrss_error_t
mrss_element (mrss_generic_t element, mrss_element_t * ret)
{
  mrss_t *tmp;

  if (!element || !ret)
    return MRSS_ERR_DATA;

  tmp = (mrss_t *) element;
  *ret = tmp->element;
  return MRSS_OK;
}

static size_t
__mrss_get_last_modified_header (void *ptr, size_t size, size_t nmemb,
				 time_t * timing)
{
  char *header = (char *) ptr;

  if (!strncmp ("Last-Modified:", header, 14))
    *timing = curl_getdate (header + 14, NULL);

  return size * nmemb;
}

mrss_error_t
mrss_get_last_modified (const char *urlstring, time_t * lastmodified)
{
  return mrss_get_last_modified_with_options (urlstring, lastmodified, NULL);
}

mrss_error_t
mrss_get_last_modified_with_options (const char *urlstring, time_t * lastmodified,
				     mrss_options_t * options)
{
  CURL *curl;

  if (!urlstring || !lastmodified)
    return MRSS_ERR_DATA;

  *lastmodified = 0;

  curl_global_init (CURL_GLOBAL_DEFAULT);
  if (!(curl = curl_easy_init ()))
    return MRSS_ERR_POSIX;

  curl_easy_setopt (curl, CURLOPT_URL, urlstring);
  curl_easy_setopt (curl, CURLOPT_HEADERFUNCTION,
		    __mrss_get_last_modified_header);
  curl_easy_setopt (curl, CURLOPT_HEADERDATA, lastmodified);
  curl_easy_setopt (curl, CURLOPT_NOBODY, 1);
  curl_easy_setopt (curl, CURLOPT_FOLLOWLOCATION, 1);

  if (options)
    {
      if (options->timeout > 0)
	curl_easy_setopt (curl, CURLOPT_TIMEOUT, options->timeout);
      else if (options->timeout < 0)
	curl_easy_setopt (curl, CURLOPT_TIMEOUT, 10);

      if (options->certfile)
	curl_easy_setopt (curl, CURLOPT_SSLCERT, options->certfile);

      if (options->password)
	curl_easy_setopt (curl, CURLOPT_SSLCERTPASSWD, options->password);

      if (options->cacert)
	curl_easy_setopt (curl, CURLOPT_CAINFO, options->cacert);

      if (options->proxy)
	{
	  curl_easy_setopt (curl, CURLOPT_PROXY, options->proxy);

	  if (options->proxy_authentication)
	    curl_easy_setopt (curl, CURLOPT_PROXYUSERPWD,
			      options->proxy_authentication);
	}

      curl_easy_setopt (curl, CURLOPT_SSL_VERIFYPEER, options->verifypeer);
    }

  if (curl_easy_perform (curl))
    {
      curl_easy_cleanup (curl);
      return MRSS_ERR_POSIX;
    }

  curl_easy_cleanup (curl);

  return MRSS_OK;
}

/* EOF */
