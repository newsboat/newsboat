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
nxml_root_element (nxml_t * nxml, nxml_data_t ** data)
{
  nxml_data_t *tmp;

  if (!data || !nxml)
    return NXML_ERR_DATA;

  tmp = nxml->data;
  while (tmp)
    {

      if (tmp->type == NXML_TYPE_ELEMENT)
	break;

      tmp = tmp->next;
    }

  *data = tmp;

  return NXML_OK;
}

nxml_error_t
nxml_doctype_element (nxml_t * nxml, nxml_doctype_t ** data)
{
  if (!data || !nxml)
    return NXML_ERR_DATA;

  *data = nxml->doctype;
  return NXML_OK;
}

nxml_error_t
nxml_get_string (nxml_data_t * data, char **string)
{
  if (!data || !string)
    return NXML_ERR_DATA;

  if (data->type == NXML_TYPE_TEXT)
    *string = strdup (data->value);

  else if (data->type == NXML_TYPE_ELEMENT)
    {
      nxml_data_t *tmp;

      tmp = data->children;
      *string = NULL;

      while (tmp)
	{
	  if (tmp->type == NXML_TYPE_TEXT)
	    {
	      *string = strdup (tmp->value);
	      break;
	    }
	  tmp = tmp->next;
	}
    }
  else
    *string = NULL;

  return NXML_OK;
}


nxml_error_t
nxml_find_element (nxml_t * nxml, nxml_data_t * data, const char *name,
		   nxml_data_t ** element)
{
  nxml_data_t *tmp;

  if (!nxml || !name || !element)
    return NXML_ERR_DATA;

  if (data && data->type != NXML_TYPE_ELEMENT)
    {
      *element = NULL;
      return NXML_OK;
    }

  if (data)
    tmp = data->children;
  else
    tmp = nxml->data;

  while (tmp)
    {
      if (tmp->type == NXML_TYPE_ELEMENT && !strcmp (tmp->value, name))
	{
	  *element = tmp;
	  return NXML_OK;
	}

      tmp = tmp->next;
    }

  *element = NULL;
  return NXML_OK;
}

nxml_error_t
nxml_find_attribute (nxml_data_t * data, const char *name, nxml_attr_t ** attribute)
{
  nxml_attr_t *tmp;

  if (!data || !name || !attribute)
    return NXML_ERR_DATA;

  if (data->type != NXML_TYPE_ELEMENT)
    {
      *attribute = NULL;
      return NXML_OK;
    }

  tmp = data->attributes;

  while (tmp)
    {
      if (!strcmp (tmp->name, name))
	{
	  *attribute = tmp;
	  return NXML_OK;
	}

      tmp = tmp->next;
    }

  *attribute = NULL;
  return NXML_OK;
}

nxml_error_t
nxml_find_namespace (nxml_data_t * data, const char *name,
		     nxml_namespace_t ** namespace)
{
  nxml_namespace_t *tmp;

  if (!data || !name || !namespace)
    return NXML_ERR_DATA;

  if (data->type != NXML_TYPE_ELEMENT)
    {
      *namespace = NULL;
      return NXML_OK;
    }

  tmp = data->ns_list;

  while (tmp)
    {
      if (!strcmp (tmp->ns, name))
	{
	  *namespace = tmp;
	  return NXML_OK;
	}

      tmp = tmp->next;
    }

  *namespace = NULL;
  return NXML_OK;
}

/* EOF */
