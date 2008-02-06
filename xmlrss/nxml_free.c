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

static void __nxml_private_free (__nxml_private_t * priv);
static void __nxml_entity_free (__nxml_private_t * priv);

nxml_error_t
nxml_free_data (nxml_data_t * data)
{
  nxml_namespace_t *namespace;
  nxml_attr_t *attr;
  nxml_data_t *tmp;
  void *old;

  if (!data)
    return NXML_ERR_DATA;

  if (data->value)
    free (data->value);

  namespace = data->ns_list;
  while (namespace)
    {
      old = namespace;
      namespace = namespace->next;

      nxml_free_namespace (old);
    }

  attr = data->attributes;
  while (attr)
    {
      old = attr;
      attr = attr->next;

      nxml_free_attribute (old);
    }

  tmp = data->children;
  while (tmp)
    {
      old = tmp;
      tmp = tmp->next;

      nxml_free_data (old);
    }

  free (data);

  return NXML_OK;
}

nxml_error_t
nxml_free_attribute (nxml_attr_t * t)
{
  if (!t)
    return NXML_ERR_DATA;

  if (t->name)
    free (t->name);

  if (t->value)
    free (t->value);

  free (t);

  return NXML_OK;
}

nxml_error_t
nxml_free_namespace (nxml_namespace_t * t)
{
  if (!t)
    return NXML_ERR_DATA;

  if (t->prefix)
    free (t->prefix);

  if (t->ns)
    free (t->ns);

  free (t);

  return NXML_OK;
}

nxml_error_t
nxml_free_doctype (nxml_doctype_t * doctype)
{
  nxml_doctype_t *tmp;

  if (!doctype)
    return NXML_ERR_DATA;

  while (doctype)
    {
      if (doctype->value)
	free (doctype->value);

      if (doctype->name)
	free (doctype->name);

      tmp = doctype;
      doctype = doctype->next;

      free (tmp);
    }

  return NXML_OK;
}

nxml_error_t
nxml_free (nxml_t * data)
{
  if (!data)
    return NXML_ERR_DATA;

  nxml_empty (data);

  __nxml_private_free (&data->priv);

  free (data);

  return NXML_OK;
}

nxml_error_t
nxml_empty (nxml_t * data)
{
  nxml_data_t *t, *old;
  __nxml_private_t priv;

  if (!data)
    return NXML_ERR_DATA;

  if (data->file)
    free (data->file);

  if (data->encoding)
    free (data->encoding);

  /* I must free the doctype, I must not empty only! */
  if (data->doctype)
    nxml_free_doctype (data->doctype);

  t = data->data;
  while (t)
    {
      old = t;
      t = t->next;
      nxml_free_data (old);
    }

  __nxml_entity_free (&data->priv);

  memcpy (&priv, &data->priv, sizeof (__nxml_private_t));
  memset (data, 0, sizeof (nxml_t));
  memcpy (&data->priv, &priv, sizeof (__nxml_private_t));

  return NXML_OK;
}

static void
__nxml_entity_free (__nxml_private_t * priv)
{
  __nxml_entity_t *entity;

  if (!priv)
    return;

  while (priv->entities)
    {
      entity = priv->entities;
      priv->entities = priv->entities->next;

      if (entity->entity)
	free (entity->entity);

      if (entity->name)
	free (entity->name);

      free (entity);
    }
}

static void
__nxml_private_free (__nxml_private_t * priv)
{
  if (!priv)
    return;

  if (priv->proxy)
    free (priv->proxy);

  if (priv->proxy_authentication)
    free (priv->proxy_authentication);

  if (priv->certfile)
    free (priv->certfile);

  if (priv->password)
    free (priv->password);

  if (priv->cacert)
    free (priv->cacert);

  if (priv->authentication)
    free (priv->authentication);

  if (priv->user_agent)
    free (priv->user_agent);

  __nxml_entity_free (priv);
}

/* EOF */
