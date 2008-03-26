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

static void
__mrss_parse_tag_insert (mrss_tag_t ** where, mrss_tag_t * what)
{
  if (!*where)
    *where = what;
  else
    {
      mrss_tag_t *tag = *where;

      while (tag->next)
	tag = tag->next;

      tag->next = what;
    }
}

static mrss_tag_t *
__mrss_parse_tag (nxml_t * doc, nxml_data_t * cur)
{
  mrss_tag_t *tag;
  mrss_attribute_t *attribute;
  nxml_attr_t *nxml_attr;

  if (!(tag = (mrss_tag_t *) calloc (1, sizeof (mrss_tag_t))))
    return NULL;

  tag->element = MRSS_ELEMENT_TAG;
  tag->allocated = 1;

  if (!(tag->name = strdup (cur->value)))
    {
      mrss_free (tag);
      return NULL;
    }

  if (cur->ns && cur->ns->ns && !(tag->ns = strdup (cur->ns->ns)))
    {
      mrss_free (tag);
      return NULL;
    }

  for (nxml_attr = cur->attributes; nxml_attr; nxml_attr = nxml_attr->next)
    {

      if (!
	  (attribute =
	   (mrss_attribute_t *) calloc (1, sizeof (mrss_attribute_t))))
	return NULL;

      attribute->element = MRSS_ELEMENT_ATTRIBUTE;
      attribute->allocated = 1;

      if (!(attribute->name = strdup (nxml_attr->name)))
	{
	  mrss_free (tag);
	  return NULL;
	}

      if (!(attribute->value = strdup (nxml_attr->value)))
	{
	  mrss_free (tag);
	  return NULL;
	}

      if (nxml_attr->ns && nxml_attr->ns->ns
	  && !(attribute->ns = strdup (nxml_attr->ns->ns)))
	{
	  mrss_free (tag);
	  return NULL;
	}

      if (!tag->attributes)
	tag->attributes = attribute;
      else
	{
	  mrss_attribute_t *tmp = tag->attributes;

	  while (tmp->next)
	    tmp = tmp->next;

	  tmp->next = attribute;
	}

    }

  for (cur = cur->children; cur; cur = cur->next)
    {
      if (cur->type == NXML_TYPE_TEXT)
	{
	  if (!tag->value && !(tag->value = strdup (cur->value)))
	    {
	      mrss_free (tag);
	      return NULL;
	    }
	}
      else if (cur->type == NXML_TYPE_ELEMENT)
	{
	  mrss_tag_t *child = __mrss_parse_tag (doc, cur);

	  if (child)
	    __mrss_parse_tag_insert (&tag->children, child);
	}
    }

  return tag;
}

static void
__mrss_parser_atom_string (nxml_t * doc, nxml_data_t * cur, char **what,
			   char **type)
{
  const char *c;

  if (!(c = nxmle_find_attribute (cur, "type", NULL)) || !strcmp (c, "text"))
    {
      if (what)
      	*what = (char *)nxmle_get_string (cur, NULL);
      if (type)
	*type = (char *)c;
      else
	free((char *)c);      	
      return;
    }

  if (!strcmp (c, "html") || !strcmp (c, "xhtml"))
    {
      nxml_data_t *ncur;
      char *total;
      const char *c1;
      nxml_t *new;
      int size;

      total = NULL;
      size = 0;

      c1 = nxmle_get_string(cur, NULL);

      if (c1 && strlen(c1) > 0)
	{
	  total = (char *)c1;
	  size = strlen(total);
	}
      else
      while ((ncur = cur->children))
	{
	  char *buffer = NULL, *p;
	  char *tmp;
	  int len;

	  if (nxml_remove (doc, cur, ncur) != NXML_OK)
	    continue;

	  if (nxml_new (&new) != NXML_OK)
	    {
	      nxml_free_data (ncur);
	      continue;
	    }

	  if (nxml_add (new, NULL, &ncur) != NXML_OK)
	    {
	      nxml_free_data (ncur);
	      nxml_free (new);
	      continue;
	    }

	  if (!(buffer = nxmle_write_buffer (new, NULL)))
	    {
	      nxml_free (new);
	      continue;
	    }

	  nxml_free (new);

	  if (strncmp (buffer, "<?xml ", 6))
	    {
	      free (buffer);
	      continue;
	    }

	  p = buffer;

	  while (*p && *p != '>')
	    p++;

	  if (!*p)
	    {
	      free (buffer);
	      continue;
	    }

	  p++;
	  while (*p && (*p == ' ' || *p == '\t' || *p == '\n'))
	    p++;

	  len = strlen (p);

	  if (!(tmp = realloc (total, size + len + 1)))
	    {
	      free (buffer);

	      if (total)
		{
		  free (total);
		  total = NULL;
		}

	      break;
	    }

	  total = tmp;
	  strcpy (total + size, p);
	  size += len;

	  free (buffer);
	}

      if (what)
      	*what = (char *)total;
      else
        free(total);

      if (type)
	*type = (char *)c;
      else
      	free((char *)c);

      return;
    }

  free ((char *)c);
  if (what)
  	*what = (char *)nxmle_get_string (cur, NULL);
}

static char *
__mrss_atom_prepare_date (mrss_t * data, const char *datestr)
{
  struct tm stm;

  data = data;

  if (!datestr)
    return NULL;

  memset (&stm, 0, sizeof (stm));

  /* format: 2007-01-17T07:45:50Z */
  if (sscanf
      (datestr, "%04d-%02d-%02dT%02d:%02d:%02dZ", &stm.tm_year,
       &stm.tm_mon, &stm.tm_mday, &stm.tm_hour, &stm.tm_min,
       &stm.tm_sec) == 6)
    {
      char datebuf[256];
      stm.tm_year -= 1900;
      stm.tm_mon -= 1;

#ifdef USE_LOCALE
      if (!data->c_locale
	  && !(data->c_locale = newlocale (LC_ALL_MASK, "C", NULL)))
	return NULL;

      strftime_l (datebuf, sizeof (datebuf), "%a, %d %b %Y %H:%M:%S %z", &stm,
		  data->c_locale);
#else
      strftime (datebuf, sizeof (datebuf), "%a, %d %b %Y %H:%M:%S %z", &stm);
#endif

      return strdup (datebuf);
    }

  return NULL;
}

static void
__mrss_parser_atom_category (nxml_data_t * cur, mrss_category_t ** category)
{
  const char *c;
  mrss_category_t *cat;

  if (!(cat = calloc (1, sizeof (mrss_category_t))))
    return;

  if (!(c = nxmle_find_attribute (cur, "term", NULL)))
    {
      free (cat);
      return;
    }

  cat->element = MRSS_ELEMENT_CATEGORY;
  cat->allocated = 1;
  cat->category = (char *)c;

  if ((c = nxmle_find_attribute (cur, "scheme", NULL)))
    cat->domain = (char *)c;

  if ((c = nxmle_find_attribute (cur, "label", NULL)))
    cat->label = (char *)c;

  if (!*category)
    *category = cat;

  else
    {
      mrss_category_t *tmp;
      tmp = *category;

      while (tmp->next)
	tmp = tmp->next;

      tmp->next = cat;
    }
}

static void
__mrss_parser_atom_author (nxml_data_t * cur, char **name, char **email,
			   char **uri)
{
  for (cur = cur->children; cur; cur = cur->next)
    {
      if (!*name && !strcmp (cur->value, "name"))
	*name = (char *)nxmle_get_string (cur, NULL);

      else if (!*email && !strcmp (cur->value, "email"))
	*email = (char *)nxmle_get_string (cur, NULL);

      else if (!*uri && !strcmp (cur->value, "uri"))
	*uri = (char *)nxmle_get_string (cur, NULL);
    }
}

static void
__mrss_parser_atom_entry (nxml_t * doc, nxml_data_t * cur, mrss_t * data)
{
  const char *c;
  mrss_item_t *item;

  if (!(item = malloc (sizeof (mrss_item_t))))
    return;

  memset (item, 0, sizeof (mrss_item_t));
  item->element = MRSS_ELEMENT_ITEM;
  item->allocated = 1;

  for (cur = cur->children; cur; cur = cur->next)
    {
      if (cur->type == NXML_TYPE_ELEMENT)
	{
	  /* title -> title */
	  if (!item->title && !strcmp (cur->value, "title"))
	    __mrss_parser_atom_string (doc, cur, &item->title,
				       &item->title_type);

	  /* link href -> link */
	  else if (!item->link && !strcmp (cur->value, "link")
		   && (c = nxmle_find_attribute (cur, "href", NULL)))
	    item->link = (char *)c;

	  /* content -> description */
	  else if (!item->description && !strcmp (cur->value, "content"))
	    __mrss_parser_atom_string (doc, cur, &item->description,
				       &item->description_type);

	  /* summary -> description */
	  else if (!item->description && !strcmp (cur->value, "summary"))
	    __mrss_parser_atom_string (doc, cur, &item->description, NULL);

	  /* right -> copyright */
	  else if (!item->copyright && !strcmp (cur->value, "rights"))
	    __mrss_parser_atom_string (doc, cur, &item->copyright, NULL);

	  /* author structure -> author elements */
	  else if (!strcmp (cur->value, "author"))
	    __mrss_parser_atom_author (cur, &item->author,
				       &item->author_email,
				       &item->author_uri);

	  /* contributor structure -> contributor elements */
	  else if (!strcmp (cur->value, "contributor"))
	    __mrss_parser_atom_author (cur, &item->contributor,
				       &item->contributor_email,
				       &item->contributor_uri);

	  /* published -> pubDate */
	  else if (!item->pubDate && !strcmp (cur->value, "published")
		   && data->version == MRSS_VERSION_ATOM_1_0
		   && (c = nxmle_get_string (cur, NULL)))
	    {
	      item->pubDate = __mrss_atom_prepare_date (data, c);
	      free ((char *)c);
	    }

	  else if (!item->pubDate && !strcmp (cur->value, "updated")
		   && data->version == MRSS_VERSION_ATOM_1_0
		   && (c = nxmle_get_string (cur, NULL)))
	    {
	      item->pubDate = __mrss_atom_prepare_date (data, c);
	      free ((char *)c);
	    }

	  /* issued -> pubDate (Atom 0.3) */
	  else if (!item->pubDate && !strcmp (cur->value, "issued")
		   && (c = nxmle_get_string (cur, NULL)))
	    {
	      item->pubDate = __mrss_atom_prepare_date (data, c);
	      free ((char *)c);
	    }

	  /* id -> guid */
	  else if (!item->guid && !strcmp (cur->value, "id")
		   && (c = nxmle_get_string (cur, NULL)))
	    item->guid = (char *)c;

	  /* categories */
	  else if (!strcmp (cur->value, "category"))
	    __mrss_parser_atom_category (cur, &item->category);

	  else
	    {
	      mrss_tag_t *tag;
	      if ((tag = __mrss_parse_tag (doc, cur)))
		__mrss_parse_tag_insert (&item->other_tags, tag);
	    }
	}
    }

  if (!data->item)
    data->item = item;

  else
    {
      mrss_item_t *tmp = data->item;

      while (tmp->next)
	tmp = tmp->next;

      tmp->next = item;
    }
}

static void
__mrss_parser_rss_image (nxml_t * doc, nxml_data_t * cur, mrss_t * data)
{
  const char *c;

  doc = doc;

  for (cur = cur->children; cur; cur = cur->next)
    {
      if (cur->type == NXML_TYPE_ELEMENT)
	{
	  /* title */
	  if (!strcmp (cur->value, "title") && !data->image_title
	      && (c = nxmle_get_string (cur, NULL)))
	    data->image_title = (char *)c;

	  /* url */
	  else if (!strcmp (cur->value, "url") && !data->image_url
		   && (c = nxmle_get_string (cur, NULL)))
	    data->image_url = (char *)c;

	  /* link */
	  else if (!strcmp (cur->value, "link") && !data->image_link
		   && (c = nxmle_get_string (cur, NULL)))
	    data->image_link = (char *)c;

	  /* width */
	  else if (!strcmp (cur->value, "width") && !data->image_width
		   && (c = nxmle_get_string (cur, NULL)))
	    {
	      data->image_width = atoi (c);
	      free ((char *)c);
	    }

	  /* height */
	  else if (!strcmp (cur->value, "height") && !data->image_height
		   && (c = nxmle_get_string (cur, NULL)))
	    {
	      data->image_height = atoi (c);
	      free ((char *)c);
	    }

	  /* description */
	  else if (!strcmp (cur->value, "description")
		   && !data->image_description
		   && (c = nxmle_get_string (cur, NULL)))
	    data->image_description = (char *)c;
	}
    }
}

static void
__mrss_parser_rss_textinput (nxml_t * doc, nxml_data_t * cur, mrss_t * data)
{
  const char *c;

  doc = doc;

  for (cur = cur->children; cur; cur = cur->next)
    {
      if (cur->type == NXML_TYPE_ELEMENT)
	{
	  /* title */
	  if (!strcmp (cur->value, "title") && !data->textinput_title
	      && (c = nxmle_get_string (cur, NULL)))
	    data->textinput_title = (char *)c;

	  /* description */
	  else if (!strcmp (cur->value, "description")
		   && !data->textinput_description
		   && (c = nxmle_get_string (cur, NULL)))
	    data->textinput_description = (char *)c;

	  /* name */
	  else if (!strcmp (cur->value, "name") && !data->textinput_name
		   && (c = nxmle_get_string (cur, NULL)))
	    data->textinput_name = (char *)c;

	  /* link */
	  else if (!strcmp (cur->value, "link") && !data->textinput_link
		   && (c = nxmle_get_string (cur, NULL)))
	    data->textinput_link = (char *)c;
	}
    }
}

static void
__mrss_parser_rss_skipHours (nxml_t * doc, nxml_data_t * cur, mrss_t * data)
{
  const char *c;

  doc = doc;

  for (cur = cur->children; cur; cur = cur->next)
    {
      if (cur->type == NXML_TYPE_ELEMENT)
	{
	  if (!strcmp (cur->value, "hour")
	      && (c = nxmle_get_string (cur, NULL)))
	    {
	      mrss_hour_t *hour;

	      if (!(hour = (mrss_hour_t *) calloc (1, sizeof (mrss_hour_t))))
		{
		  free ((char *)c);
		  return;
		}

	      hour->element = MRSS_ELEMENT_SKIPHOURS;
	      hour->allocated = 1;
	      hour->hour = (char *)c;

	      if (!data->skipHours)
		data->skipHours = hour;
	      else
		{
		  mrss_hour_t *tmp;

		  tmp = data->skipHours;

		  while (tmp->next)
		    tmp = tmp->next;
		  tmp->next = hour;
		}
	    }
	}
    }
}

static void
__mrss_parser_rss_skipDays (nxml_t * doc, nxml_data_t * cur, mrss_t * data)
{
  const char *c;
  
  doc = doc;

  for (cur = cur->children; cur; cur = cur->next)
    {
      if (cur->type == NXML_TYPE_ELEMENT)
	{
	  if (!strcmp (cur->value, "day")
	      && (c = nxmle_get_string (cur, NULL)))
	    {
	      mrss_day_t *day;

	      if (!(day = (mrss_day_t *) calloc (1, sizeof (mrss_day_t))))
		{
		  free ((char *)c);
		  return;
		}

	      day->element = MRSS_ELEMENT_SKIPDAYS;
	      day->allocated = 1;
	      day->day = (char *)c;

	      if (!data->skipDays)
		data->skipDays = day;
	      else
		{
		  mrss_day_t *tmp;

		  tmp = data->skipDays;

		  while (tmp->next)
		    tmp = tmp->next;
		  tmp->next = day;
		}
	    }
	}
    }
}

static void
__mrss_parser_rss_item (nxml_t * doc, nxml_data_t * cur, mrss_t * data)
{
  const char *c;
  const char *attr;
  mrss_item_t *item;

  if (!(item = (mrss_item_t *) calloc (1, sizeof (mrss_item_t))))
    return;

  item->element = MRSS_ELEMENT_ITEM;
  item->allocated = 1;

  for (cur = cur->children; cur; cur = cur->next)
    {
      if (cur->type == NXML_TYPE_ELEMENT)
	{
	  /* title */
	  if (!strcmp (cur->value, "title") && !item->title
	      && (c = nxmle_get_string (cur, NULL)))
	    item->title = (char *)c;

	  /* link */
	  else if (!strcmp (cur->value, "link") && !item->link
		   && (c = nxmle_get_string (cur, NULL)))
	    item->link = (char *)c;

	  /* description */
	  else if (!strcmp (cur->value, "description") && !item->description
		   && (c = nxmle_get_string (cur, NULL)))
	    item->description = (char *)c;

	  /* source */
	  else if (!strcmp (cur->value, "source") && !item->source)
	    {
	      item->source = (char *)nxmle_get_string (cur, NULL);

	      if ((attr = nxmle_find_attribute (cur, "url", NULL)))
		item->source_url = (char *)attr;
	    }

	  /* enclosure */
	  else if (!strcmp (cur->value, "enclosure") && !item->enclosure)
	    {
	      item->enclosure = (char *)nxmle_get_string (cur, NULL);

	      if (!item->enclosure_url && (attr = nxmle_find_attribute (cur, "url", NULL)))
		item->enclosure_url = (char *)attr;

	      if ((attr = nxmle_find_attribute (cur, "length", NULL)))
		{
		  item->enclosure_length = atoi (attr);
		  free ((char *)attr);
		}

	      if (!item->enclosure_type && (attr = nxmle_find_attribute (cur, "type", NULL)))
		item->enclosure_type = (char *)attr;
	    }

	  /* category */
	  else if (!strcmp (cur->value, "category")
		   && (c = nxmle_get_string (cur, NULL)))
	    {
	      mrss_category_t *category;

	      if (!
		  (category =
		   (mrss_category_t *) calloc (1, sizeof (mrss_category_t))))
		{
		  free ((char *)c);
		  return;
		}

	      category->element = MRSS_ELEMENT_CATEGORY;
	      category->allocated = 1;
	      category->category = (char *)c;

	      if ((attr = nxmle_find_attribute (cur, "domain", NULL)))
		category->domain = (char *)attr;

	      if (!item->category)
		item->category = category;
	      else
		{
		  mrss_category_t *tmp;

		  tmp = item->category;
		  while (tmp->next)
		    tmp = tmp->next;
		  tmp->next = category;
		}
	    }

	  /* author */
	  else if (!strcmp (cur->value, "author") && !item->author
		   && (c = nxmle_get_string (cur, NULL)))
	    item->author = (char *)c;

	  /* comments */
	  else if (!strcmp (cur->value, "comments") && !item->comments
		   && (c = nxmle_get_string (cur, NULL)))
	    item->comments = (char *)c;

	  /* guid */
	  else if (!strcmp (cur->value, "guid") && !item->guid
		   && (c = nxmle_get_string (cur, NULL)))
	    {
	      item->guid = (char *)c;

	      if ((attr = nxmle_find_attribute (cur, "isPermaLink", NULL)))
		{
		  if (!strcmp (attr, "false"))
		    item->guid_isPermaLink = 0;
		  else
		    item->guid_isPermaLink = 1;

		  free ((char *)attr);
		}
	    }

	  /* pubDate */
	  else if (!strcmp (cur->value, "pubDate") && !item->pubDate
		   && (c = nxmle_get_string (cur, NULL)))
	    item->pubDate = (char *)c;

	  /* Other tags: */
	  else
	    {
	      mrss_tag_t *tag;

	      if ((tag = __mrss_parse_tag (doc, cur)))
		__mrss_parse_tag_insert (&item->other_tags, tag);
	    }

	}
    }


  if (!data->item)
    data->item = item;
  else
    {
      mrss_item_t *tmp;

      tmp = data->item;

      while (tmp->next)
	tmp = tmp->next;
      tmp->next = item;
    }
}

static mrss_error_t
__mrss_parser_atom (nxml_t * doc, nxml_data_t * cur, mrss_t ** ret)
{
  mrss_t *data;
  const char *c = NULL;

  if (!(data = malloc (sizeof (mrss_t))))
    return MRSS_ERR_POSIX;

  memset (data, 0, sizeof (mrss_t));
  data->element = MRSS_ELEMENT_CHANNEL;
  data->allocated = 1;
  data->version = MRSS_VERSION_ATOM_1_0;

  if (doc->encoding && !(data->encoding = strdup (doc->encoding)))
    {
      mrss_free (data);
      return MRSS_ERR_POSIX;
    }

  if (!data->language && (c = nxmle_find_attribute (cur, "xml:lang", NULL)))
    data->language = (char *)c;

  if ((c = nxmle_find_attribute (cur, "version", NULL)))
    {
      if (!strcmp (c, "0.3"))
	data->version = MRSS_VERSION_ATOM_0_3;

      free ((char *)c);
    }

  for (cur = cur->children; cur; cur = cur->next)
    {
      if (cur->type == NXML_TYPE_ELEMENT)
	{
	  /* title -> title */
	  if (!data->title && !strcmp (cur->value, "title"))
	    __mrss_parser_atom_string (doc, cur, &data->title,
				       &data->title_type);

	  /* subtitle -> description */
	  else if (!data->description
		   && data->version == MRSS_VERSION_ATOM_1_0
		   && !strcmp (cur->value, "subtitle"))
	    __mrss_parser_atom_string (doc, cur, &data->description,
				       &data->description_type);

	  /* tagline -> description (Atom 0.3) */
	  else if (data->version == MRSS_VERSION_ATOM_0_3
		   && !data->description && !strcmp (cur->value, "tagline"))
	    __mrss_parser_atom_string (doc, cur, &data->description,
				       &data->description_type);

	  /* link href -> link */
	  else if (!strcmp (cur->value, "link") && !data->link
		   && (c = nxmle_find_attribute (cur, "href", NULL)))
	    data->link = (char *)c;

	  /* id -> id */
	  else if (!strcmp (cur->value, "id") && !data->id
		   && (c = nxmle_get_string (cur, NULL)))
	    data->id = (char *)c;

	  /* rights -> copyright */
	  else if (!data->copyright && !strcmp (cur->value, "rights"))
	    __mrss_parser_atom_string (doc, cur, &data->copyright,
				       &data->copyright_type);

	  /* updated -> lastBuildDate */
	  else if (!strcmp (cur->value, "updated")
		   && (c = nxmle_get_string (cur, NULL)))
	    {
	      data->lastBuildDate = __mrss_atom_prepare_date (data, c);
	      free ((char *)c);
	    }

	  /* author -> managingeditor */
	  else if (!strcmp (cur->value, "author"))
	    __mrss_parser_atom_author (cur, &data->managingeditor,
				       &data->managingeditor_email,
				       &data->managingeditor_uri);

	  /* contributor */
	  else if (!strcmp (cur->value, "contributor"))
	    __mrss_parser_atom_author (cur, &data->contributor,
				       &data->contributor_email,
				       &data->contributor_uri);

	  /* generator -> generator */
	  else if (!strcmp (cur->value, "generator") && !data->generator
		   && (c = nxmle_get_string (cur, NULL)))
	    {
	      const char *attr;

	      data->generator = (char *)c;

	      if ((attr = nxmle_find_attribute (cur, "uri", NULL)))
		data->generator_uri = (char *)attr;

	      if ((attr = nxmle_find_attribute (cur, "version", NULL)))
		data->generator_version = (char *)attr;
	    }

	  /* icon -> image_url */
	  else if (!strcmp (cur->value, "icon") && !data->image_url
		   && (c = nxmle_get_string (cur, NULL)))
	    data->image_url = (char *)c;

	  /* logo -> image_logo */
	  else if (!strcmp (cur->value, "logo") && !data->image_logo
		   && (c = nxmle_get_string (cur, NULL)))
	    data->image_logo = (char *)c;

	  /* category */
	  else if (!strcmp (cur->value, "category"))
	    __mrss_parser_atom_category (cur, &data->category);

	  /* entry -> item */
	  else if (!strcmp (cur->value, "entry"))
	    __mrss_parser_atom_entry (doc, cur, data);

	  else
	    {
	      mrss_tag_t *tag;
	      if ((tag = __mrss_parse_tag (doc, cur)))
		__mrss_parse_tag_insert (&data->other_tags, tag);
	    }

	}
    }

  *ret = data;

  return MRSS_OK;
}

static mrss_error_t
__mrss_parser_rss (mrss_version_t v, nxml_t * doc, nxml_data_t * cur,
		   mrss_t ** ret)
{
  mrss_t *data;
  const char *c, *attr;

  if (!(data = (mrss_t *) calloc (1, sizeof (mrss_t))))
    return MRSS_ERR_POSIX;

  data->element = MRSS_ELEMENT_CHANNEL;
  data->allocated = 1;
  data->version = v;

  if (doc->encoding && !(data->encoding = strdup (doc->encoding)))
    {
      mrss_free (data);
      return MRSS_ERR_POSIX;
    }

  if (data->version == MRSS_VERSION_1_0)
    {
      nxml_data_t *cur_channel = NULL;

      while (cur)
	{

	  if (!strcmp (cur->value, "channel"))
	    cur_channel = cur;

	  else if (!strcmp (cur->value, "image"))
	    __mrss_parser_rss_image (doc, cur, data);

	  else if (!strcmp (cur->value, "textinput"))
	    __mrss_parser_rss_textinput (doc, cur, data);

	  else if (!strcmp (cur->value, "item"))
	    __mrss_parser_rss_item (doc, cur, data);

	  cur = cur->next;
	}

      cur = cur_channel;
    }
  else
    {
      while (cur && strcmp (cur->value, "channel"))
	cur = cur->next;
    }

  if (!cur)
    {
      mrss_free (data);
      return MRSS_ERR_PARSER;
    }

  if (data->version == MRSS_VERSION_1_0)
    {
      if ((attr = nxmle_find_attribute (cur, "about", NULL)))
	data->about = (char *)attr;
    }

  for (cur = cur->children; cur; cur = cur->next)
    {
      if (cur->type == NXML_TYPE_ELEMENT)
	{
	  /* title */
	  if (!strcmp (cur->value, "title") && !data->title &&
	      (c = nxmle_get_string (cur, NULL)))
	    data->title = (char *)c;

	  /* description */
	  else if (!strcmp (cur->value, "description") && !data->description
		   && (c = nxmle_get_string (cur, NULL)))
	    data->description = (char *)c;

	  /* link */
	  else if (!strcmp (cur->value, "link") && !data->link
		   && (c = nxmle_get_string (cur, NULL)))
	    data->link = (char *)c;

	  /* language */
	  else if (!strcmp (cur->value, "language") && !data->language
		   && (c = nxmle_get_string (cur, NULL)))
	    data->language = (char *)c;

	  /* rating */
	  else if (!strcmp (cur->value, "rating") && !data->rating
		   && (c = nxmle_get_string (cur, NULL)))
	    data->rating = (char *)c;

	  /* copyright */
	  else if (!strcmp (cur->value, "copyright") && !data->copyright
		   && (c = nxmle_get_string (cur, NULL)))
	    data->copyright = (char *)c;

	  /* pubDate */
	  else if (!strcmp (cur->value, "pubDate") && !data->pubDate
		   && (c = nxmle_get_string (cur, NULL)))
	    data->pubDate = (char *)c;

	  /* lastBuildDate */
	  else if (!strcmp (cur->value, "lastBuildDate")
		   && !data->lastBuildDate
		   && (c = nxmle_get_string (cur, NULL)))
	    data->lastBuildDate = (char *)c;

	  /* docs */
	  else if (!strcmp (cur->value, "docs") && !data->docs
		   && (c = nxmle_get_string (cur, NULL)))
	    data->docs = (char *)c;

	  /* managingeditor */
	  else if (!strcmp (cur->value, "managingeditor")
		   && !data->managingeditor
		   && (c = nxmle_get_string (cur, NULL)))
	    data->managingeditor = (char *)c;

	  /* webMaster */
	  else if (!strcmp (cur->value, "webMaster") && !data->webMaster
		   && (c = nxmle_get_string (cur, NULL)))
	    data->webMaster = (char *)c;

	  /* image */
	  else if (!strcmp (cur->value, "image"))
	    __mrss_parser_rss_image (doc, cur, data);

	  /* textinput */
	  else if (!strcmp (cur->value, "textinput"))
	    __mrss_parser_rss_textinput (doc, cur, data);

	  /* skipHours */
	  else if (!strcmp (cur->value, "skipHours"))
	    __mrss_parser_rss_skipHours (doc, cur, data);

	  /* skipDays */
	  else if (!strcmp (cur->value, "skipDays"))
	    __mrss_parser_rss_skipDays (doc, cur, data);

	  /* item */
	  else if (!strcmp (cur->value, "item"))
	    __mrss_parser_rss_item (doc, cur, data);

	  /* category */
	  else if (!strcmp (cur->value, "category")
		   && (c = nxmle_get_string (cur, NULL)))
	    {
	      mrss_category_t *category;

	      if (!
		  (category =
		   (mrss_category_t *) calloc (1, sizeof (mrss_category_t))))
		{
		  mrss_free ((mrss_generic_t *) data);
		  free ((char *)c);
		  return MRSS_ERR_POSIX;
		}

	      category->element = MRSS_ELEMENT_CATEGORY;
	      category->allocated = 1;
	      category->category = (char *)c;

	      if ((attr = nxmle_find_attribute (cur, "domain", NULL)))
		category->domain = (char *)attr;

	      if (!data->category)
		data->category = category;
	      else
		{
		  mrss_category_t *tmp;

		  tmp = data->category;
		  while (tmp->next)
		    tmp = tmp->next;
		  tmp->next = category;
		}
	    }

	  /* enclosure */
	  else if (!strcmp (cur->value, "cloud") && !data->cloud)
	    {
	      data->cloud = (char *)nxmle_get_string (cur, NULL);

	      if (!data->cloud_domain
		  && (attr = nxmle_find_attribute (cur, "domain", NULL)))
		data->cloud_domain = (char *)attr;

	      if (!data->cloud_port
		  && (attr = nxmle_find_attribute (cur, "port", NULL)))
		data->cloud_port = atoi (attr);

	      if (!data->cloud_registerProcedure
		  && (attr =
		      nxmle_find_attribute (cur, "registerProcedure", NULL)))
		data->cloud_registerProcedure = (char *)attr;

	      if (!data->cloud_protocol
		  && (attr = nxmle_find_attribute (cur, "protocol", NULL)))
		data->cloud_protocol = (char *)attr;
	    }

	  /* generator */
	  else if (!strcmp (cur->value, "generator") && !data->generator
		   && (c = nxmle_get_string (cur, NULL)))
	    data->generator = (char *)c;

	  /* ttl */
	  else if (!strcmp (cur->value, "ttl") && !data->ttl
		   && (c = nxmle_get_string (cur, NULL)))
	    {
	      data->ttl = atoi (c);
	      free ((char *)c);
	    }

	  /* Other tags: */
	  else if (data->version != MRSS_VERSION_1_0
		   || strcmp (cur->value, "items"))
	    {
	      mrss_tag_t *tag;

	      if ((tag = __mrss_parse_tag (doc, cur)))
		__mrss_parse_tag_insert (&data->other_tags, tag);
	    }
	}
    }

  *ret = data;

  return MRSS_OK;
}

static mrss_error_t
__mrss_parser (nxml_t * doc, mrss_t ** ret)
{
  mrss_error_t r = MRSS_ERR_VERSION;
  nxml_data_t *cur;
  const char *c;

  if (!(cur = nxmle_root_element (doc, NULL)))
    return MRSS_ERR_PARSER;

  if (!strcmp (cur->value, "rss"))
    {
      if ((c = nxmle_find_attribute (cur, "version", NULL)))
	{
	  /* 0.91 VERSION */
	  if (!strcmp (c, "0.91"))
	    r =
	      __mrss_parser_rss (MRSS_VERSION_0_91, doc, cur->children, ret);

	  /* 0.92 VERSION */
	  else if (!strcmp (c, "0.92"))
	    r =
	      __mrss_parser_rss (MRSS_VERSION_0_92, doc, cur->children, ret);

	  /* 2.0 VERSION */
	  else if (!strcmp (c, "2.0"))
	    r = __mrss_parser_rss (MRSS_VERSION_2_0, doc, cur->children, ret);

	  else
	    r = MRSS_ERR_VERSION;

	  free ((char *)c);
	}

      else
	r = MRSS_ERR_VERSION;
    }

  else if (!strcmp (cur->value, "RDF"))
    r = __mrss_parser_rss (MRSS_VERSION_1_0, doc, cur->children, ret);

  else if (!strcmp (cur->value, "feed"))
    r = __mrss_parser_atom (doc, cur, ret);

  else
    r = MRSS_ERR_PARSER;

  return r;
}

/*************************** EXTERNAL FUNCTION ******************************/

mrss_error_t
mrss_parse_url (const char *url, mrss_t ** ret)
{
  return mrss_parse_url_with_options_and_error (url, ret, NULL, NULL);
}

mrss_error_t
mrss_parse_url_with_options (const char *url, mrss_t ** ret,
			     mrss_options_t * options)
{
  return mrss_parse_url_with_options_and_error (url, ret, options, NULL);
}

mrss_error_t
mrss_parse_url_with_options_and_error (const char *url, mrss_t ** ret,
				       mrss_options_t * options,
				       CURLcode * code)
{
  nxml_t *doc;
  mrss_error_t err;
  const char *buffer;
  size_t size;

  if (!url || !ret)
    return MRSS_ERR_DATA;

  if (nxml_new (&doc) != NXML_OK)
    return MRSS_ERR_POSIX;

  if (options)
    {
      if (options->timeout >= 0)
	nxml_set_timeout (doc, options->timeout);

      if (options->proxy)
	nxml_set_proxy (doc, options->proxy, options->proxy_authentication);

      if (options->authentication)
	nxml_set_authentication (doc, options->authentication);

      if (options->user_agent)
	nxml_set_user_agent (doc, options->user_agent);

      nxml_set_certificate (doc, options->certfile, options->password,
			    options->cacert, options->verifypeer);
    }

  if (!(buffer = __mrss_download_file (doc, url, &size, &err, code)))
    return err;

  if (nxml_parse_buffer (doc, buffer, size) != NXML_OK)
    {
      free ((char *)buffer);
      nxml_free (doc);

      return MRSS_ERR_PARSER;
    }

  if (!(err = __mrss_parser (doc, ret)))
    {
      if (!((*ret)->file = strdup (url)))
	{
	  free ((char *)buffer);

	  mrss_free (*ret);
	  nxml_free (doc);

	  return MRSS_ERR_POSIX;
	}

      (*ret)->size = size;
    }

  free ((char *)buffer);
  nxml_free (doc);

  return err;
}

mrss_error_t
mrss_parse_file (const char *file, mrss_t ** ret)
{
  nxml_t *doc;
  mrss_error_t err;
  struct stat st;

  if (!file || !ret)
    return MRSS_ERR_DATA;

  if (lstat (file, &st))
    return MRSS_ERR_POSIX;

  if (nxml_new (&doc) != NXML_OK)
    return MRSS_ERR_POSIX;

  if (nxml_parse_file (doc, file) != NXML_OK)
    {
      nxml_free (doc);
      return MRSS_ERR_PARSER;
    }

  if (!(err = __mrss_parser (doc, ret)))
    {
      if (!((*ret)->file = strdup (file)))
	{
	  nxml_free (doc);
	  mrss_free (*ret);

	  return MRSS_ERR_POSIX;
	}

      (*ret)->size = st.st_size;
    }

  nxml_free (doc);

  return err;
}

mrss_error_t
mrss_parse_buffer (const char *buffer, size_t size, mrss_t ** ret)
{
  nxml_t *doc;
  mrss_error_t err;

  if (!buffer || !size || !ret)
    return MRSS_ERR_DATA;

  if (nxml_new (&doc) != NXML_OK)
    return MRSS_ERR_POSIX;

  if (nxml_parse_buffer (doc, buffer, size))
    {
      nxml_free (doc);
      return MRSS_ERR_PARSER;
    }

  if (!(err = __mrss_parser (doc, ret)))
    (*ret)->size = size;

  nxml_free (doc);
  return err;
}

/* EOF */
