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

typedef struct __nxml_download_t__ __nxml_download_t;

struct __nxml_download_t__
{
  char *mm;
  size_t size;
};

static size_t
__nxml_memorize_file (void *ptr, size_t size, size_t nmemb, void *data)
{
  register int realsize = size * nmemb;
  __nxml_download_t *mem = (__nxml_download_t *) data;

  if (!mem->mm)
    {
      if (!(mem->mm = (char *) malloc (realsize + 1)))
	return -1;
    }
  else
    {
      if (!(mem->mm = (char *) realloc (mem->mm, mem->size + realsize + 1)))
	return -1;
    }

  memcpy (&(mem->mm[mem->size]), ptr, realsize);
  mem->size += realsize;
  mem->mm[mem->size] = 0;

  return realsize;
}

nxml_error_t
nxml_download_file (nxml_t * nxml, const char *fl, char **buffer, size_t * size)
{
  __nxml_download_t *chunk;
  CURL *curl;
  CURLcode ret;

  if (!fl || !buffer || !nxml)
    return NXML_ERR_DATA;

  if (!(chunk = (__nxml_download_t *) malloc (sizeof (__nxml_download_t))))
    return NXML_ERR_POSIX;

  chunk->mm = NULL;
  chunk->size = 0;

  curl_global_init (CURL_GLOBAL_DEFAULT);
  if (!(curl = curl_easy_init ()))
    {
      free (chunk);
      return NXML_ERR_POSIX;
    }

  curl_easy_setopt (curl, CURLOPT_URL, fl);
  curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, __nxml_memorize_file);
  curl_easy_setopt (curl, CURLOPT_FOLLOWLOCATION, 1);
  curl_easy_setopt (curl, CURLOPT_FILE, (void *) chunk);
  curl_easy_setopt (curl, CURLOPT_ENCODING, "gzip, deflate");

  if (nxml->priv.timeout)
    curl_easy_setopt (curl, CURLOPT_TIMEOUT, nxml->priv.timeout);

  curl_easy_setopt (curl, CURLOPT_SSL_VERIFYPEER, !nxml->priv.verifypeer);

  if (nxml->priv.certfile)
    {
      curl_easy_setopt (curl, CURLOPT_SSLCERT, nxml->priv.certfile);

      if (nxml->priv.password)
	curl_easy_setopt (curl, CURLOPT_SSLCERTPASSWD, nxml->priv.password);

      if (nxml->priv.cacert)
	curl_easy_setopt (curl, CURLOPT_CAINFO, nxml->priv.cacert);
    }

  if (nxml->priv.authentication)
    curl_easy_setopt (curl, CURLOPT_USERPWD, nxml->priv.authentication);

  if (nxml->priv.proxy)
    {
      curl_easy_setopt (curl, CURLOPT_PROXY, nxml->priv.proxy);

      if (nxml->priv.proxy_authentication)
	curl_easy_setopt (curl, CURLOPT_PROXYUSERPWD,
			  nxml->priv.proxy_authentication);
    }

  if (nxml->priv.user_agent)
    curl_easy_setopt (curl, CURLOPT_USERAGENT, nxml->priv.user_agent);

  if ((ret = curl_easy_perform (curl)))
    {
      if (chunk->mm)
	free (chunk->mm);

      free (chunk);

      nxml->priv.curl_error = ret;

      curl_easy_cleanup (curl);
      return NXML_ERR_DOWNLOAD;
    }

  curl_easy_cleanup (curl);

  *buffer = chunk->mm;

  if (size)
    *size = chunk->size;

  free (chunk);

  return NXML_OK;
}

/* EOF */
