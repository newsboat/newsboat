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

static mrss_error_t __mrss_search_tag_real (mrss_tag_t * tag, const char *name,
					    const char *ns, mrss_tag_t ** ret);

mrss_error_t
mrss_search_tag (mrss_generic_t data, const char *name, const char *ns, mrss_tag_t ** tag)
{
  mrss_t *tmp;
  mrss_error_t err;

  if (!data || !name)
    return MRSS_ERR_DATA;


  tmp = (mrss_t *) data;
  switch (tmp->element)
    {
    case MRSS_ELEMENT_CHANNEL:
      err =
	__mrss_search_tag_real (((mrss_t *) data)->other_tags, name, ns, tag);
      break;

    case MRSS_ELEMENT_ITEM:
      err =
	__mrss_search_tag_real (((mrss_item_t *) data)->other_tags, name, ns,
				tag);
      break;

    case MRSS_ELEMENT_TAG:
      err =
	__mrss_search_tag_real (((mrss_tag_t *) data)->children, name, ns,
				tag);
      break;

    default:
      err = MRSS_ERR_DATA;
      break;
    }

  return err;
}

static mrss_error_t
__mrss_search_tag_real (mrss_tag_t * tag, const char *name, const char *ns,
			mrss_tag_t ** ret)
{
  int i;

  for (*ret = NULL; tag; tag = tag->next)
    {
      i = 0;
      if (tag->ns)
	i++;
      if (ns)
	i++;

      if ((!i || (i == 2 && !strcmp (tag->ns, ns)))
	  && !strcmp (name, tag->name))
	{
	  *ret = tag;
	  return MRSS_OK;
	}
    }

  return MRSS_OK;
}

mrss_error_t
mrss_search_attribute (mrss_generic_t data, const char *name, const char *ns,
		       mrss_attribute_t ** attribute)
{
  mrss_tag_t *tag;
  mrss_attribute_t *attr;
  int i;

  if (!data || !name)
    return MRSS_ERR_DATA;

  tag = (mrss_tag_t *) data;
  if (tag->element != MRSS_ELEMENT_TAG)
    return MRSS_ERR_DATA;

  for (*attribute = NULL, attr = tag->attributes; attr; attr = attr->next)
    {
      i = 0;
      if (attr->ns)
	i++;
      if (ns)
	i++;

      if ((!i || (i == 2 && !strcmp (attr->ns, ns)))
	  && !strcmp (name, attr->name))
	{
	  *attribute = attr;
	  return MRSS_OK;
	}
    }

  return MRSS_OK;
}

/* EOF */
