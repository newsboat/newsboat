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

mrss_options_t *
mrss_options_new (int timeout, const char *proxy, const char *proxy_authentication,
		  const char *certfile, const char *password, const char *cacert,
		  int verifypeer, const char *authentication, const char *user_agent)
{
  mrss_options_t *options;

  if (!(options = (mrss_options_t *) malloc (sizeof (mrss_options_t))))
    return NULL;

  options->timeout = timeout;
  options->proxy = proxy ? strdup (proxy) : NULL;
  options->proxy_authentication =
    proxy_authentication ? strdup (proxy_authentication) : NULL;
  options->certfile = certfile ? strdup (certfile) : NULL;
  options->password = password ? strdup (password) : NULL;
  options->cacert = cacert ? strdup (cacert) : NULL;
  options->authentication = authentication ? strdup (authentication) : NULL;
  options->user_agent = user_agent ? strdup (user_agent) : NULL;
  options->verifypeer = verifypeer;

  return options;
}

void
mrss_options_free (mrss_options_t * options)
{
  if (!options)
    return;

  if (options->proxy)
    free (options->proxy);

  if (options->proxy_authentication)
    free (options->proxy_authentication);

  if (options->certfile)
    free (options->certfile);

  if (options->password)
    free (options->password);

  if (options->cacert)
    free (options->cacert);

  if (options->authentication)
    free (options->authentication);

  if (options->user_agent)
    free (options->user_agent);

  free (options);
}

/* EOF */
