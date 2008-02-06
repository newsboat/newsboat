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

struct __nxml_data_ns_t
{
  nxml_namespace_t *ns;
  struct __nxml_data_ns_t *next;
};

static void
__nxml_namespace_free_item (nxml_data_t * e)
{
  nxml_namespace_t *ns;
  nxml_data_t *child;

  while (e->ns_list)
    {
      ns = e->ns_list->next;

      if (e->ns_list->prefix)
	free (e->ns_list->prefix);

      if (e->ns_list->ns)
	free (e->ns_list->ns);

      free (e->ns_list);

      e->ns_list = ns;
    }

  e->ns = NULL;

  child = e->children;
  while (child)
    {
      __nxml_namespace_free_item (child);
      child = child->next;
    }
}

static void
__nxml_namespace_free (nxml_t * nxml)
{
  nxml_data_t *e;

  e = nxml->data;
  while (e)
    {
      __nxml_namespace_free_item (e);

      e = e->next;
    }
}

int
__nxml_namespace_parse_add (nxml_data_t * data, char *prefix, char *namespace)
{
  nxml_namespace_t *ns;

  if (!(ns = (nxml_namespace_t *) calloc (1, sizeof (nxml_namespace_t))))
    return 1;

  if (prefix && !(ns->prefix = strdup (prefix)))
    {
      free (ns);
      return 1;
    }

  if (!(ns->ns = strdup (namespace)))
    {
      if (ns->prefix)
	free (ns->prefix);
      free (ns);
      return 1;
    }

  ns->next = data->ns_list;
  data->ns_list = ns;

  return 0;
}

static int
__nxml_namespace_find_item (nxml_t * nxml, nxml_data_t * e)
{
  nxml_data_t *child;
  nxml_attr_t *att;

  att = e->attributes;
  while (att)
    {
      if (!strcmp (att->name, "xmlns"))
	{
	  if (__nxml_namespace_parse_add (e, NULL, att->value))
	    {
	      __nxml_namespace_free (nxml);
	      return 1;
	    }
	}
      else if (!strncmp (att->name, "xmlns:", 6))
	{
	  if (__nxml_namespace_parse_add (e, att->name + 6, att->value))
	    {
	      __nxml_namespace_free (nxml);
	      return 1;
	    }
	}

      att = att->next;
    }

  child = e->children;
  while (child)
    {
      if (child->type == NXML_TYPE_ELEMENT)
	__nxml_namespace_find_item (nxml, child);
      child = child->next;
    }

  return 0;
}

static int
__nxml_namespace_find (nxml_t * nxml)
{
  nxml_data_t *e;

  e = nxml->data;
  while (e)
    {
      if (e->type == NXML_TYPE_ELEMENT)
	__nxml_namespace_find_item (nxml, e);
      e = e->next;
    }

  return 0;

}

static void
__nxml_namespace_associate_attribute (struct __nxml_data_ns_t *list,
				      nxml_attr_t * e)
{
  int i;
  int len = strlen (e->name);
  int k;

  for (i = k = 0; i < len; i++)
    if (e->name[i] == ':')
      {
	k = i;
	break;
      }

  if (!k)
    {
      while (list)
	{
	  if (!list->ns->prefix)
	    {
	      e->ns = list->ns;
	      return;
	    }
	  list = list->next;
	}

      return;
    }

  else
    {
      while (list)
	{
	  if (list->ns->prefix && !strncmp (list->ns->prefix, e->name, k))
	    {
	      char *a = strdup (e->name + strlen (list->ns->prefix) + 1);

	      if (!a)
		return;

	      free (e->name);
	      e->name = a;

	      e->ns = list->ns;
	      return;
	    }
	  list = list->next;
	}
    }
}

static void
__nxml_namespace_associate_item (struct __nxml_data_ns_t *list,
				 nxml_data_t * e)
{
  int i;
  int len;
  int k;
  nxml_attr_t *attr;

  attr = e->attributes;
  while (attr)
    {
      __nxml_namespace_associate_attribute (list, attr);
      attr = attr->next;
    }

  len = strlen (e->value);

  for (i = k = 0; i < len; i++)
    if (e->value[i] == ':')
      {
	k = i;
	break;
      }

  if (!k)
    {
      while (list)
	{
	  if (!list->ns->prefix)
	    {
	      e->ns = list->ns;
	      return;
	    }
	  list = list->next;
	}

      return;
    }

  else
    {
      while (list)
	{
	  if (list->ns->prefix && !strncmp (list->ns->prefix, e->value, k))
	    {
	      char *a = strdup (e->value + strlen (list->ns->prefix) + 1);

	      if (!a)
		return;

	      free (e->value);
	      e->value = a;

	      e->ns = list->ns;
	      return;
	    }
	  list = list->next;
	}
    }
}

static void
__nxml_namespace_associate (struct __nxml_data_ns_t **list,
			    nxml_data_t * root)
{
  nxml_data_t *e;
  nxml_namespace_t *ns;
  struct __nxml_data_ns_t *tmp, *old;

  ns = root->ns_list;
  while (ns)
    {
      if (!(tmp = calloc (1, sizeof (struct __nxml_data_ns_t))))
	return;

      tmp->ns = ns;
      tmp->next = (*list);
      (*list) = tmp;

      ns = ns->next;
    }

  __nxml_namespace_associate_item (*list, root);

  e = root->children;
  while (e)
    {
      if (e->type == NXML_TYPE_ELEMENT)
	__nxml_namespace_associate (list, e);

      e = e->next;
    }

  ns = root->ns_list;
  while (ns)
    {
      tmp = *list;
      old = NULL;

      while (tmp)
	{
	  if (tmp->ns == ns)
	    {
	      if (old)
		old->next = tmp->next;
	      else
		*list = tmp->next;

	      free (tmp);
	      break;
	    }

	  old = tmp;
	  tmp = tmp->next;
	}

      ns = ns->next;
    }
}

static void
__nxml_namespace_connect (nxml_t * nxml)
{
  nxml_data_t *e;
  struct __nxml_data_ns_t *list = NULL;

  e = nxml->data;
  while (e)
    {
      if (e->type == NXML_TYPE_ELEMENT)
	__nxml_namespace_associate (&list, e);

      e = e->next;
    }
}

void
__nxml_namespace_parse (nxml_t * nxml)
{
  __nxml_namespace_free (nxml);

  if (__nxml_namespace_find (nxml))
    return;

  __nxml_namespace_connect (nxml);
}

/* EOF */
