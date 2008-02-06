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

nxml_error_t
nxml_new (nxml_t ** nxml)
{
  if (!nxml)
    return NXML_ERR_DATA;

  if (!(*nxml = (nxml_t *) calloc (1, sizeof (nxml_t))))
    return NXML_ERR_POSIX;

  return NXML_OK;
}

static void
nxml_add_rec (nxml_t * nxml, nxml_data_t * data)
{
  while (data)
    {
      data->doc = nxml;
      if (data->children)
	nxml_add_rec (nxml, data->children);

      data = data->next;
    }
}

nxml_error_t
nxml_add (nxml_t * nxml, nxml_data_t * parent, nxml_data_t ** child)
{
  nxml_data_t *tmp;

  if (!nxml || !child)
    return NXML_ERR_DATA;

  if (!*child && !(*child = (nxml_data_t *) calloc (1, sizeof (nxml_data_t))))
    return NXML_ERR_POSIX;


  (*child)->doc = nxml;
  (*child)->parent = parent;
  (*child)->next = NULL;


  if (parent)
    {
      if (!parent->children)
	parent->children = *child;

      else
	{
	  tmp = parent->children;

	  while (tmp->next)
	    tmp = tmp->next;

	  tmp->next = *child;
	}
    }
  else
    {
      if (!nxml->data)
	nxml->data = *child;

      else
	{
	  tmp = nxml->data;

	  while (tmp->next)
	    tmp = tmp->next;

	  tmp->next = *child;
	}
    }

  nxml_add_rec (nxml, (*child)->children);

  return NXML_OK;
}

nxml_error_t
nxml_remove (nxml_t * nxml, nxml_data_t * parent, nxml_data_t * child)
{
  nxml_data_t *tmp, *old;

  if (!nxml || !child)
    return NXML_ERR_DATA;

  if (parent)
    tmp = parent->children;
  else
    tmp = nxml->data;

  old = NULL;

  while (tmp)
    {
      if (tmp == child)
	{
	  if (old)
	    old->next = child->next;
	  else if (parent)
	    parent->children = child->next;
	  else
	    nxml->data = child->next;

	  break;
	}

      old = tmp;
      tmp = tmp->next;
    }

  child->next = NULL;

  return NXML_OK;
}

nxml_error_t
nxml_add_attribute (nxml_t * nxml, nxml_data_t * element, nxml_attr_t ** attr)
{
  nxml_attr_t *tmp;

  if (!nxml || !element || !attr)
    return NXML_ERR_DATA;

  if (!*attr && !(*attr = (nxml_attr_t *) calloc (1, sizeof (nxml_attr_t))))
    return NXML_ERR_POSIX;

  (*attr)->next = NULL;

  if (!element->attributes)
    element->attributes = *attr;

  else
    {
      tmp = element->attributes;

      while (tmp->next)
	tmp = tmp->next;

      tmp->next = *attr;
    }

  return NXML_OK;
}

nxml_error_t
nxml_remove_attribute (nxml_t * nxml, nxml_data_t * element,
		       nxml_attr_t * attr)
{
  nxml_attr_t *tmp, *old;

  if (!nxml || !element || !attr)
    return NXML_ERR_DATA;

  tmp = element->attributes;

  old = NULL;

  while (tmp)
    {
      if (tmp == attr)
	{
	  if (old)
	    old->next = attr->next;
	  else
	    element->attributes = attr->next;

	  break;
	}

      old = tmp;
      tmp = tmp->next;
    }

  attr->next = NULL;

  return NXML_OK;
}

nxml_error_t
nxml_add_namespace (nxml_t * nxml, nxml_data_t * element,
		    nxml_namespace_t ** namespace)
{
  nxml_namespace_t *tmp;

  if (!nxml || !element || !namespace)
    return NXML_ERR_DATA;

  if (!*namespace
      && !(*namespace =
	   (nxml_namespace_t *) calloc (1, sizeof (nxml_namespace_t))))
    return NXML_ERR_POSIX;

  (*namespace)->next = NULL;

  if (!element->ns_list)
    element->ns_list = *namespace;

  else
    {
      tmp = element->ns_list;

      while (tmp->next)
	tmp = tmp->next;

      tmp->next = *namespace;
    }

  return NXML_OK;
}

nxml_error_t
nxml_remove_namespace (nxml_t * nxml, nxml_data_t * element,
		       nxml_namespace_t * namespace)
{
  nxml_namespace_t *tmp, *old;

  if (!nxml || !element || !namespace)
    return NXML_ERR_DATA;

  tmp = element->ns_list;

  old = NULL;

  while (tmp)
    {
      if (tmp == namespace)
	{
	  if (old)
	    old->next = namespace->next;
	  else
	    element->ns_list = namespace->next;

	  break;
	}

      old = tmp;
      tmp = tmp->next;
    }

  namespace->next = NULL;

  return NXML_OK;
}

nxml_error_t
nxml_set_func (nxml_t * nxml, void (*func) (char *, ...))
{
  if (!nxml)
    return NXML_ERR_DATA;

  nxml->priv.func = func;

  return NXML_OK;
}

nxml_error_t
nxml_set_timeout (nxml_t * nxml, int timeout)
{
  if (!nxml)
    return NXML_ERR_DATA;

  nxml->priv.timeout = timeout;

  return NXML_OK;
}

nxml_error_t
nxml_set_proxy (nxml_t * nxml, const char *proxy, const char *userpwd)
{
  if (!nxml)
    return NXML_ERR_DATA;

  if (nxml->priv.proxy)
    free (nxml->priv.proxy);

  if (proxy)
    nxml->priv.proxy = strdup (proxy);
  else
    nxml->priv.proxy = NULL;

  if (nxml->priv.proxy_authentication)
    free (nxml->priv.proxy_authentication);

  if (userpwd)
    nxml->priv.proxy_authentication = strdup (userpwd);
  else
    nxml->priv.proxy_authentication = NULL;

  return NXML_OK;
}

nxml_error_t
nxml_set_authentication (nxml_t * nxml, const char *userpwd)
{
  if (!nxml)
    return NXML_ERR_DATA;

  if (nxml->priv.authentication)
    free (nxml->priv.authentication);

  if (userpwd)
    nxml->priv.authentication = strdup (userpwd);
  else
    nxml->priv.authentication = NULL;

  return NXML_OK;
}

nxml_error_t
nxml_set_textindent (nxml_t * nxml, char textindent)
{
  if (!nxml)
    return NXML_ERR_DATA;

  if (textindent)
    nxml->priv.textindent = 1;
  else
    nxml->priv.textindent = 0;

  return NXML_OK;
}

nxml_error_t
nxml_set_user_agent (nxml_t * nxml, const char *user_agent)
{
  if (!nxml)
    return NXML_ERR_DATA;

  if (nxml->priv.user_agent)
    free (nxml->priv.user_agent);

  if (user_agent)
    nxml->priv.user_agent = strdup (user_agent);
  else
    nxml->priv.user_agent = NULL;

  return NXML_OK;
}

nxml_error_t
nxml_set_certificate (nxml_t * nxml, const char *certificate, const char *password,
		      const char *cacert, int verifypeer)
{
  if (!nxml)
    return NXML_ERR_DATA;

  if (nxml->priv.certfile)
    free (nxml->priv.certfile);

  if (certificate)
    nxml->priv.certfile = strdup (certificate);
  else
    nxml->priv.certfile = NULL;

  if (nxml->priv.password)
    free (nxml->priv.password);

  if (password)
    nxml->priv.password = strdup (password);
  else
    nxml->priv.password = NULL;

  if (cacert)
    nxml->priv.cacert = strdup (cacert);
  else
    nxml->priv.cacert = NULL;

  nxml->priv.verifypeer = !verifypeer;

  return NXML_OK;
}

void
nxml_print_generic (char *str, ...)
{
  va_list va;

  va_start (va, str);
  vfprintf (stderr, str, va);
  va_end (va);
}


/* EOF */
