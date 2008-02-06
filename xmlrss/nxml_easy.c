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

nxml_t *
nxmle_new_data (nxml_error_t * err)
{
  nxml_t *data = NULL;
  nxml_error_t ret;

  ret = nxml_new (&data);
  if (err)
    *err = ret;

  if (ret != NXML_OK)
    return NULL;

  return data;
}

nxml_t *
nxmle_new_data_from_url (const char *url, nxml_error_t * err)
{
  nxml_t *data = NULL;
  nxml_error_t ret;

  ret = nxml_new (&data);
  if (err)
    *err = ret;

  if (ret != NXML_OK)
    return NULL;

  nxml_set_func (data, nxml_print_generic);

  ret = nxml_parse_url (data, url);
  if (err)
    *err = ret;

  if (ret != NXML_OK)
    {
      nxml_free (data);
      return NULL;
    }

  return data;
}

nxml_t *
nxmle_new_data_from_file (const char *file, nxml_error_t * err)
{
  nxml_t *data = NULL;
  nxml_error_t ret;

  ret = nxml_new (&data);
  if (err)
    *err = ret;

  if (ret != NXML_OK)
    return NULL;

  nxml_set_func (data, nxml_print_generic);

  ret = nxml_parse_file (data, file);
  if (err)
    *err = ret;

  if (ret != NXML_OK)
    {
      nxml_free (data);
      return NULL;
    }

  return data;
}

nxml_t *
nxmle_new_data_from_buffer (const char *buffer, size_t size, nxml_error_t * err)
{
  nxml_t *data = NULL;
  nxml_error_t ret;

  ret = nxml_new (&data);
  if (err)
    *err = ret;

  if (ret != NXML_OK)
    return NULL;

  nxml_set_func (data, nxml_print_generic);

  ret = nxml_parse_buffer (data, buffer, size);
  if (err)
    *err = ret;

  if (ret != NXML_OK)
    {
      nxml_free (data);
      return NULL;
    }

  return data;
}

nxml_data_t *
nxmle_add_new (nxml_t * nxml, nxml_data_t * parent, nxml_error_t * err)
{
  nxml_error_t ret;
  nxml_data_t *child = NULL;

  ret = nxml_add (nxml, parent, &child);
  if (err)
    *err = ret;

  if (ret != NXML_OK)
    return NULL;

  return child;
}

nxml_data_t *
nxmle_add_data (nxml_t * nxml, nxml_data_t * parent, nxml_data_t * child,
		nxml_error_t * err)
{
  nxml_error_t ret;

  if (!child)
    {
      if (err)
	*err = NXML_ERR_DATA;
      return NULL;
    }

  ret = nxml_add (nxml, parent, &child);
  if (err)
    *err = ret;

  if (ret != NXML_OK)
    return NULL;

  return child;
}

nxml_attr_t *
nxmle_add_attribute_new (nxml_t * nxml, nxml_data_t * element,
			 nxml_error_t * err)
{
  nxml_error_t ret;
  nxml_attr_t *attribute = NULL;

  ret = nxml_add_attribute (nxml, element, &attribute);
  if (err)
    *err = ret;

  if (ret != NXML_OK)
    return NULL;

  return attribute;
}

nxml_attr_t *
nxmle_add_attribute_data (nxml_t * nxml, nxml_data_t * element,
			  nxml_attr_t * attribute, nxml_error_t * err)
{
  nxml_error_t ret;

  if (!attribute)
    {
      if (err)
	*err = NXML_ERR_DATA;
      return NULL;
    }

  ret = nxml_add_attribute (nxml, element, &attribute);
  if (err)
    *err = ret;

  if (ret != NXML_OK)
    return NULL;

  return attribute;
}

nxml_namespace_t *
nxmle_add_namespace_new (nxml_t * nxml, nxml_data_t * element,
			 nxml_error_t * err)
{
  nxml_error_t ret;
  nxml_namespace_t *namespace = NULL;

  ret = nxml_add_namespace (nxml, element, &namespace);
  if (err)
    *err = ret;

  if (ret != NXML_OK)
    return NULL;

  return namespace;
}

nxml_namespace_t *
nxmle_add_namespace_data (nxml_t * nxml, nxml_data_t * element,
			  nxml_namespace_t * namespace, nxml_error_t * err)
{
  nxml_error_t ret;

  if (!namespace)
    {
      if (err)
	*err = NXML_ERR_DATA;
      return NULL;
    }

  ret = nxml_add_namespace (nxml, element, &namespace);
  if (err)
    *err = ret;

  if (ret != NXML_OK)
    return NULL;

  return namespace;
}

nxml_data_t *
nxmle_root_element (nxml_t * nxml, nxml_error_t * err)
{
  nxml_data_t *root;
  nxml_error_t ret;

  ret = nxml_root_element (nxml, &root);
  if (err)
    *err = ret;

  if (ret != NXML_OK)
    return NULL;

  return root;
}

nxml_doctype_t *
nxmle_doctype_element (nxml_t * nxml, nxml_error_t * err)
{
  nxml_doctype_t *doctype;
  nxml_error_t ret;

  ret = nxml_doctype_element (nxml, &doctype);
  if (err)
    *err = ret;

  if (ret != NXML_OK)
    return NULL;

  return doctype;
}

nxml_data_t *
nxmle_find_element (nxml_t * nxml, nxml_data_t * data, const char *name,
		    nxml_error_t * err)
{
  nxml_data_t *element;
  nxml_error_t ret;

  ret = nxml_find_element (nxml, data, name, &element);
  if (err)
    *err = ret;

  if (ret != NXML_OK)
    return NULL;

  return element;
}

const char *
nxmle_find_attribute (nxml_data_t * data, const char *name, nxml_error_t * err)
{
  nxml_attr_t *attribute;
  nxml_error_t ret;
  const char *str;

  ret = nxml_find_attribute (data, name, &attribute);
  if (err)
    *err = ret;

  if (ret != NXML_OK)
    return NULL;

  if (!attribute)
    return NULL;

  str = strdup (attribute->value);
  if (!str)
    {
      if (err)
	*err = NXML_ERR_POSIX;
      return NULL;
    }

  return str;
}

const char *
nxmle_find_namespace (nxml_data_t * data, const char *name, nxml_error_t * err)
{
  nxml_namespace_t *namespace;
  nxml_error_t ret;
  const char *str;

  ret = nxml_find_namespace (data, name, &namespace);
  if (err)
    *err = ret;

  if (ret != NXML_OK)
    return NULL;

  if (!namespace)
    return NULL;

  str = strdup (namespace->ns);
  if (!str)
    {
      if (err)
	*err = NXML_ERR_POSIX;
      return NULL;
    }

  return str;
}

const char *
nxmle_get_string (nxml_data_t * data, nxml_error_t * err)
{
  nxml_error_t ret;
  char *str = NULL;

  ret = nxml_get_string (data, &str);
  if (err)
    *err = ret;

  if (ret != NXML_OK)
    return NULL;

  return str;
}

char *
nxmle_write_buffer (nxml_t * nxml, nxml_error_t * err)
{
  char *buffer;
  nxml_error_t ret;

  buffer = NULL;
  ret = nxml_write_buffer (nxml, &buffer);

  if (err)
    *err = ret;

  if (ret != NXML_OK)
    return NULL;

  return buffer;
}

int
nxmle_line_error (nxml_t * nxml, nxml_error_t * err)
{
  int line;
  nxml_error_t ret;

  ret = nxml_line_error (nxml, &line);

  if (err)
    *err = ret;

  return line;
}

/* EOF */
