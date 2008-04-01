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
__mrss_write_string (void (*func) (void *, char *, ...), void *obj, char *str)
{
  int i;
  int len;
  char buf[1024];
  int j;

#define __NXML_CHECK_BUF \
           if(j==sizeof(buf)-1) { buf[j]=0; func(obj, "%s",buf); j=0; }

  if (!str)
    return;

  len = strlen (str);

  for (j = i = 0; i < len; i++)
    {
      if (str[i] == '\r')
	continue;

      else if (str[i] == '<')
	{
	  buf[j++] = '&';
	  __NXML_CHECK_BUF;
	  buf[j++] = 'l';
	  __NXML_CHECK_BUF;
	  buf[j++] = 't';
	  __NXML_CHECK_BUF;
	  buf[j++] = ';';
	  __NXML_CHECK_BUF;
	}

      else if (str[i] == '>')
	{
	  buf[j++] = '&';
	  __NXML_CHECK_BUF;
	  buf[j++] = 'g';
	  __NXML_CHECK_BUF;
	  buf[j++] = 't';
	  __NXML_CHECK_BUF;
	  buf[j++] = ';';
	  __NXML_CHECK_BUF;
	}

      else if (str[i] == '&')
	{
	  buf[j++] = '&';
	  __NXML_CHECK_BUF;
	  buf[j++] = 'a';
	  __NXML_CHECK_BUF;
	  buf[j++] = 'm';
	  __NXML_CHECK_BUF;
	  buf[j++] = 'p';
	  __NXML_CHECK_BUF;
	  buf[j++] = ';';
	  __NXML_CHECK_BUF;
	}

      else if (str[i] == '\'')
	{
	  buf[j++] = '&';
	  __NXML_CHECK_BUF;
	  buf[j++] = 'a';
	  __NXML_CHECK_BUF;
	  buf[j++] = 'p';
	  __NXML_CHECK_BUF;
	  buf[j++] = 'o';
	  __NXML_CHECK_BUF;
	  buf[j++] = 's';
	  __NXML_CHECK_BUF;
	  buf[j++] = ';';
	  __NXML_CHECK_BUF;
	}

      else if (str[i] == '\"')
	{
	  buf[j++] = '&';
	  __NXML_CHECK_BUF;
	  buf[j++] = 'q';
	  __NXML_CHECK_BUF;
	  buf[j++] = 'u';
	  __NXML_CHECK_BUF;
	  buf[j++] = 'o';
	  __NXML_CHECK_BUF;
	  buf[j++] = 't';
	  __NXML_CHECK_BUF;
	  buf[j++] = ';';
	  __NXML_CHECK_BUF;
	}

      else
	{
	  buf[j++] = str[i];
	  __NXML_CHECK_BUF;
	}
    }

  if (j)
    {
      buf[j] = 0;
      func (obj, "%s", buf);
      j = 0;
    }
}

static void
__mrss_write_real_image (mrss_t * mrss, void (*func) (void *, char *, ...),
			 void *obj)
{
  if (mrss->image_title || mrss->image_url || mrss->image_link
      || mrss->image_width || mrss->image_height || mrss->description)
    {

      func (obj, "  %s<image>\n",
	    mrss->version == MRSS_VERSION_1_0 ? "" : "  ");

      if (mrss->image_title)
	{
	  func (obj, "    %s<title>",
		mrss->version == MRSS_VERSION_1_0 ? "" : "  ");
	  __mrss_write_string (func, obj, mrss->image_title);
	  func (obj, "</title>\n");
	}

      if (mrss->image_url)
	{
	  func (obj, "      %s<url>",
		mrss->version == MRSS_VERSION_1_0 ? "" : "  ");
	  __mrss_write_string (func, obj, mrss->image_url);
	  func (obj, "</url>\n");
	}

      if (mrss->image_link)
	{
	  func (obj, "      %s<link>",
		mrss->version == MRSS_VERSION_1_0 ? "" : "  ");
	  __mrss_write_string (func, obj, mrss->image_link);
	  func (obj, "</link>\n");
	}

      if (mrss->version != MRSS_VERSION_1_0)
	{
	  if (mrss->image_width)
	    func (obj, "      <width>%d</width>\n", mrss->image_width);

	  if (mrss->image_height)
	    func (obj, "      <height>%d</height>\n", mrss->image_height);

	  if (mrss->image_description)
	    {
	      func (obj, "      <description>");
	      __mrss_write_string (func, obj, mrss->image_description);
	      func (obj, "</description>\n");
	    }
	}

      func (obj, "  %s</image>\n",
	    mrss->version == MRSS_VERSION_1_0 ? "" : "  ");
    }
}

static void
__mrss_write_real_textinput (mrss_t * mrss,
			     void (*func) (void *, char *, ...), void *obj)
{
  if (mrss->textinput_title || mrss->textinput_description
      || mrss->textinput_name || mrss->textinput_link)
    {

      func (obj, "  %s<textinput>\n",
	    mrss->version == MRSS_VERSION_1_0 ? "" : "  ");

      if (mrss->textinput_title)
	{
	  func (obj, "    %s<title>",
		mrss->version == MRSS_VERSION_1_0 ? "" : "  ");
	  __mrss_write_string (func, obj, mrss->textinput_title);
	  func (obj, "</title>\n");
	}

      if (mrss->textinput_description)
	{
	  func (obj, "    %s<description>",
		mrss->version == MRSS_VERSION_1_0 ? "" : "  ");
	  __mrss_write_string (func, obj, mrss->textinput_description);
	  func (obj, "</description>\n");
	}

      if (mrss->textinput_name)
	{
	  func (obj, "    %s<name>",
		mrss->version == MRSS_VERSION_1_0 ? "" : "  ");
	  __mrss_write_string (func, obj, mrss->textinput_name);
	  func (obj, "</name>\n");
	}

      if (mrss->textinput_link)
	{
	  func (obj, "    %s<link>",
		mrss->version == MRSS_VERSION_1_0 ? "" : "  ");
	  __mrss_write_string (func, obj, mrss->textinput_link);
	  func (obj, "</link>\n");
	}

      func (obj, "    </textinput>\n");
    }
}

static void
__mrss_write_real_cloud (mrss_t * mrss, void (*func) (void *, char *, ...),
			 void *obj)
{
  if ((mrss->version == MRSS_VERSION_0_92
       || mrss->version == MRSS_VERSION_2_0) && (mrss->cloud
						 || mrss->cloud_domain
						 || mrss->cloud_port
						 || mrss->cloud_path
						 || mrss->
						 cloud_registerProcedure
						 || mrss->cloud_protocol))
    {
      func (obj, "    <cloud");

      if (mrss->cloud_domain)
	{
	  func (obj, " domain=\"");
	  __mrss_write_string (func, obj, mrss->cloud_domain);
	  func (obj, "\"");
	}

      if (mrss->cloud_port)
	func (obj, " port=\"%d\"", mrss->cloud_port);

      if (mrss->cloud_path)
	{
	  func (obj, " path=\"");
	  __mrss_write_string (func, obj, mrss->cloud_path);
	  func (obj, "\"");
	}

      if (mrss->cloud_registerProcedure)
	{
	  func (obj, " registerProcedure=\"");
	  __mrss_write_string (func, obj, mrss->cloud_registerProcedure);
	  func (obj, "\"");
	}

      if (mrss->cloud_protocol)
	{
	  func (obj, " protocol=\"");
	  __mrss_write_string (func, obj, mrss->cloud_protocol);
	  func (obj, "\"");
	}

      if (mrss->cloud)
	func (obj, ">%s</cloud>\n", mrss->cloud);
      else
	func (obj, " />\n", mrss->cloud);
    }
}

static void
__mrss_write_real_skipHours (mrss_t * mrss,
			     void (*func) (void *, char *, ...), void *obj)
{
  if (mrss->skipHours && mrss->version != MRSS_VERSION_1_0)
    {
      mrss_hour_t *hour;

      func (obj, "    <skipHours>\n");

      hour = mrss->skipHours;
      while (hour)
	{
	  func (obj, "      <hour>");
	  __mrss_write_string (func, obj, hour->hour);
	  func (obj, "</hour>\n");

	  hour = hour->next;
	}

      func (obj, "    </skipHours>\n");
    }
}

static void
__mrss_write_real_skipDays (mrss_t * mrss, void (*func) (void *, char *, ...),
			    void *obj)
{
  if (mrss->skipDays && mrss->version != MRSS_VERSION_1_0)
    {
      mrss_day_t *day;

      func (obj, "    <skipDays>\n");

      day = mrss->skipDays;
      while (day)
	{
	  func (obj, "      <day>");
	  __mrss_write_string (func, obj, day->day);
	  func (obj, "</day>\n");
	  day = day->next;
	}

      func (obj, "    </skipDays>\n");
    }
}

static void
__mrss_write_real_category (mrss_t * mrss, mrss_category_t * category,
			    void (*func) (void *, char *, ...), void *obj)
{
  if ((mrss->version == MRSS_VERSION_0_92
       || mrss->version == MRSS_VERSION_2_0) && mrss->category)
    {
      while (category)
	{
	  func (obj, "    <category");

	  if (category->domain)
	    {
	      func (obj, " domain=\"");
	      __mrss_write_string (func, obj, category->domain);
	      func (obj, "\"");
	    }

	  func (obj, ">");
	  __mrss_write_string (func, obj, category->category);
	  func (obj, "</category>\n");

	  category = category->next;
	}
    }
}

static void
__mrss_write_real_tag (mrss_tag_t * tag, void (*func) (void *, char *, ...),
		       void *obj, int idx)
{
  mrss_attribute_t *attribute;

  struct mrss_ns_t
  {
    char *ns;
    struct mrss_ns_t *next;
  } *namespaces = NULL, *nstmp;

  int i;

  while (tag)
    {
      if (tag->ns)
	{
	  if (!(namespaces = calloc (1, sizeof (struct mrss_ns_t))))
	    return;

	  namespaces->ns = tag->ns;
	}

      for (attribute = tag->attributes; attribute;
	   attribute = attribute->next)
	{
	  if (attribute->ns)
	    {
	      for (nstmp = namespaces; nstmp; nstmp = nstmp->next)
		if (!strcmp (nstmp->ns, attribute->ns))
		  break;

	      if (nstmp)
		continue;

	      if (!(nstmp = calloc (1, sizeof (struct mrss_ns_t))))
		return;

	      nstmp->ns = attribute->ns;
	      nstmp->next = namespaces->next;
	      namespaces->next = nstmp;
	    }
	}


      for (i = 0; i < idx; i++)
	func (obj, "  ");

      if (tag->ns)
	func (obj, "  <ns0:%s", tag->name);
      else
	func (obj, "  <%s", tag->name);

      for (i = 0, nstmp = namespaces; nstmp; nstmp = nstmp->next)
	func (obj, " xmlns:ns%d=\"%s\"", i++, nstmp->ns);

      for (attribute = tag->attributes; attribute;
	   attribute = attribute->next)
	{
	  if (attribute->ns)
	    {
	      for (i = 0, nstmp = namespaces; nstmp; i++, nstmp = nstmp->next)
		{
		  if (!strcmp (nstmp->ns, attribute->ns))
		    {
		      func (obj, " ns%d:%s=\"%s\"", i, attribute->name,
			    attribute->value);
		      break;
		    }
		}
	    }
	  else
	    func (obj, " %s=\"%s\"", attribute->name, attribute->value);
	}

      if (tag->value)
	{
	  func (obj, ">");
	  __mrss_write_string (func, obj, tag->value);
	}

      if (tag->children)
	{
	  if (!tag->value)
	    func (obj, ">\n");

	  func (obj, "\n");
	  __mrss_write_real_tag (tag->children, func, obj, idx + 1);
	}

      if (tag->children || tag->value)
	{
	  if (tag->children)
	    for (i = 0; i < idx; i++)
	      func (obj, "  ");

	  if (tag->ns)
	    func (obj, "</ns0:%s>\n", tag->name);
	  else
	    func (obj, "</%s>\n", tag->name);

	}
      else
	func (obj, "/>\n");

      while (namespaces)
	{
	  nstmp = namespaces;
	  namespaces = namespaces->next;
	  free (nstmp);
	}

      tag = tag->next;
    }
}

static void
__mrss_write_real_item (mrss_t * mrss, void (*func) (void *, char *, ...),
			void *obj)
{
  mrss_item_t *item = mrss->item;

  while (item)
    {
      func (obj, "  %s<item>\n",
	    mrss->version == MRSS_VERSION_1_0 ? "" : "  ");

      if (item->title)
	{
	  func (obj, "    %s<title>",
		mrss->version == MRSS_VERSION_1_0 ? "" : "  ");
	  __mrss_write_string (func, obj, item->title);
	  func (obj, "</title>\n");
	}

      if (item->link)
	{
	  func (obj, "    %s<link>",
		mrss->version == MRSS_VERSION_1_0 ? "" : "  ");
	  __mrss_write_string (func, obj, item->link);
	  func (obj, "</link>\n");
	}

      if (item->description)
	{
	  func (obj, "    %s<description>",
		mrss->version == MRSS_VERSION_1_0 ? "" : "  ");
	  __mrss_write_string (func, obj, item->description);
	  func (obj, "</description>\n");
	}

      if (mrss->version == MRSS_VERSION_2_0)
	{
	  if (item->author)
	    {
	      func (obj, "      <author>");
	      __mrss_write_string (func, obj, item->author);
	      func (obj, "</author>\n");
	    }

	  if (item->comments)
	    {
	      func (obj, "      <comments>");
	      __mrss_write_string (func, obj, item->comments);
	      func (obj, "</comments>\n");
	    }

	  if (item->pubDate)
	    {
	      func (obj, "      <pubDate>");
	      __mrss_write_string (func, obj, item->pubDate);
	      func (obj, "</pubDate>\n");
	    }

	  if (item->guid)
	    {
	      func (obj, "      <guid isPermaLink=\"%s\">",
		    item->guid_isPermaLink ? "true" : "false");
	      __mrss_write_string (func, obj, item->guid);
	      func (obj, "</guid>\n");
	    }

	}

      if (mrss->version == MRSS_VERSION_2_0
	  || mrss->version == MRSS_VERSION_0_92)
	{
	  if (item->source || item->source_url)
	    {
	      func (obj, "      <source");

	      if (item->source_url)
		{
		  func (obj, " url=\"");
		  __mrss_write_string (func, obj, item->source_url);
		  func (obj, "\"");
		}

	      if (item->source)
		{
		  func (obj, ">");
		  __mrss_write_string (func, obj, item->source);
		  func (obj, "</source>\n");
		}
	      else
		func (obj, " />\n");
	    }
	}

      if (item->enclosure || item->enclosure_length || item->enclosure_type)
	{
	  func (obj, "      <enclosure");

	  if (item->enclosure_url)
	    {
	      func (obj, " url=\"");
	      __mrss_write_string (func, obj, item->enclosure_url);
	      func (obj, "\"");
	    }

	  if (item->enclosure_length)
	    func (obj, " length=\"%d\"", item->enclosure_length);

	  if (item->enclosure_type)
	    {
	      func (obj, " type=\"");
	      __mrss_write_string (func, obj, item->enclosure_type);
	      func (obj, "\"");
	    }

	  if (item->enclosure)
	    {
	      func (obj, ">");
	      __mrss_write_string (func, obj, item->enclosure);
	      func (obj, "</enclosure>\n");
	    }
	  else
	    func (obj, " />\n");

	}

      __mrss_write_real_category (mrss, item->category, func, obj);

      if (item->other_tags)
	__mrss_write_real_tag (item->other_tags, func, obj, 1);

      func (obj, "  %s</item>\n",
	    mrss->version == MRSS_VERSION_1_0 ? "" : "  ");

      item = item->next;
    }

}

static void
__mrss_write_real_atom_category (mrss_category_t * category,
				 void (*func) (void *, char *, ...),
				 void *obj)
{
  while (category)
    {
      func (obj, "    <category");

      if (category->domain)
	{
	  func (obj, " scheme=\"");
	  __mrss_write_string (func, obj, category->domain);
	  func (obj, "\"");
	}

      if (category->category)
	{
	  func (obj, " term=\"");
	  __mrss_write_string (func, obj, category->category);
	  func (obj, "\"");
	}

      if (category->label)
	{
	  func (obj, " label=\"");
	  __mrss_write_string (func, obj, category->label);
	  func (obj, "\"");
	}

      func (obj, "/>\n");

      category = category->next;
    }
}

static void
__mrss_write_real_atom_entry (mrss_t * mrss,
			      void (*func) (void *, char *, ...), void *obj)
{
  mrss_item_t *item = mrss->item;

  while (item)
    {
      func (obj, "  <entry>\n");

      if (item->title)
	{
	  func (obj, "    <title");

	  if (item->title_type)
	    {
	      func (obj, " type=\"");
	      __mrss_write_string (func, obj, item->title_type);
	      func (obj, "\"");
	    }

	  func (obj, ">");
	  __mrss_write_string (func, obj, item->title);
	  func (obj, "</title>\n");
	}

      if (item->link)
	{
	  func (obj, "    <link>");
	  __mrss_write_string (func, obj, item->link);
	  func (obj, "</link>\n");
	}

      if (item->description)
	{
	  func (obj, "    <summary");

	  if (item->description_type)
	    {
	      func (obj, " type=\"");
	      __mrss_write_string (func, obj, item->description_type);
	      func (obj, "\"");
	    }

	  func (obj, ">");
	  __mrss_write_string (func, obj, item->description);
	  func (obj, "</summary>\n");
	}

      if (item->copyright)
	{
	  func (obj, "    <rights>");
	  __mrss_write_string (func, obj, item->copyright);
	  func (obj, "</rights>\n");
	}

      if (item->author)
	{
	  func (obj, "    <author>\n");
	  func (obj, "      <name>");
	  __mrss_write_string (func, obj, item->author);
	  func (obj, "</name>\n");

	  if (item->author_email)
	    {
	      func (obj, "      <email>");
	      __mrss_write_string (func, obj, item->author_email);
	      func (obj, "</email>\n");
	    }

	  if (item->author_uri)
	    {
	      func (obj, "      <uri>");
	      __mrss_write_string (func, obj, item->author_uri);
	      func (obj, "</uri>\n");
	    }

	  func (obj, "    </author>\n");
	}

      if (item->contributor)
	{
	  func (obj, "    <contributor>\n");
	  func (obj, "      <name>");
	  __mrss_write_string (func, obj, item->contributor);
	  func (obj, "</name>\n");

	  if (item->contributor_email)
	    {
	      func (obj, "      <email>");
	      __mrss_write_string (func, obj, item->contributor_email);
	      func (obj, "</email>\n");
	    }

	  if (item->contributor_uri)
	    {
	      func (obj, "      <uri>");
	      __mrss_write_string (func, obj, item->contributor_uri);
	      func (obj, "</uri>\n");
	    }

	  func (obj, "    </contributor>\n");
	}

      /* TODO */
      if (mrss->pubDate)
	{
	  if (mrss->version == MRSS_VERSION_ATOM_1_0)
	    {
	      func (obj, "  <published>");
	      __mrss_write_string (func, obj, item->pubDate);
	      func (obj, "</published>\n");
	    }
	  else
	    {
	      func (obj, "  <issued>");
	      __mrss_write_string (func, obj, item->pubDate);
	      func (obj, "</issued>\n");
	    }
	}

      if (item->guid)
	{
	  func (obj, "    <id>");
	  __mrss_write_string (func, obj, item->guid);
	  func (obj, "</id>\n");
	}

      __mrss_write_real_atom_category (item->category, func, obj);

      if (item->other_tags)
	__mrss_write_real_tag (item->other_tags, func, obj, 1);

      func (obj, "  </entry>\n");

      item = item->next;
    }
}

static mrss_error_t
__mrss_write_atom (mrss_t * mrss, void (*func) (void *, char *, ...),
		   void *obj)
{
  func (obj, "<feed xmlns=\"http://www.w3.org/2005/Atom\"");

  if (mrss->language)
    func (obj, " xml:lang=\"%s\"", mrss->language);

  if (mrss->version == MRSS_VERSION_ATOM_0_3)
    func (obj, " version=\"0.3\"");

  func (obj, ">\n");

  func (obj, "  <title");

  if (mrss->title_type)
    {
      func (obj, " type=\"");
      __mrss_write_string (func, obj, mrss->title_type);
      func (obj, "\"");
    }

  func (obj, ">");
  __mrss_write_string (func, obj, mrss->title);
  func (obj, "</title>\n");

  if (mrss->description)
    {
      if (mrss->version == MRSS_VERSION_ATOM_1_0)
	{
	  func (obj, "  <subtitle");

	  if (mrss->description_type)
	    {
	      func (obj, " type=\"");
	      __mrss_write_string (func, obj, mrss->description_type);
	      func (obj, "\"");
	    }

	  func (obj, ">");
	  __mrss_write_string (func, obj, mrss->description);
	  func (obj, "</subtitle>\n");
	}
      else
	{
	  func (obj, "  <tagline");

	  if (mrss->description_type)
	    {
	      func (obj, " type=\"");
	      __mrss_write_string (func, obj, mrss->description_type);
	      func (obj, "\"");
	    }

	  func (obj, ">");
	  __mrss_write_string (func, obj, mrss->description);
	  func (obj, "</tagline>\n");
	}
    }

  if (mrss->link)
    {
      func (obj, "  <link href=\"");
      __mrss_write_string (func, obj, mrss->link);
      func (obj, "\"/>\n");
    }

  if (mrss->id)
    {
      func (obj, "  <id>");
      __mrss_write_string (func, obj, mrss->id);
      func (obj, "</id>\n");
    }

  if (mrss->copyright)
    {
      func (obj, "  <rights");

      if (mrss->copyright_type)
	{
	  func (obj, " type=\"");
	  __mrss_write_string (func, obj, mrss->copyright_type);
	  func (obj, "\"");
	}

      func (obj, ">");
      __mrss_write_string (func, obj, mrss->copyright);
      func (obj, "</rights>\n");
    }

  /* TODO */
  if (mrss->lastBuildDate)
    {
      func (obj, "  <updated>");
      __mrss_write_string (func, obj, mrss->lastBuildDate);
      func (obj, "</updated>\n");
    }

  if (mrss->managingeditor)
    {
      func (obj, "  <author>\n");
      func (obj, "    <name>");
      __mrss_write_string (func, obj, mrss->managingeditor);
      func (obj, "</name>\n");

      if (mrss->managingeditor_email)
	{
	  func (obj, "    <email>");
	  __mrss_write_string (func, obj, mrss->managingeditor_email);
	  func (obj, "</email>\n");
	}

      if (mrss->managingeditor_uri)
	{
	  func (obj, "    <uri>");
	  __mrss_write_string (func, obj, mrss->managingeditor_uri);
	  func (obj, "</uri>\n");
	}

      func (obj, "  </author>\n");
    }

  if (mrss->generator)
    {
      func (obj, "  <generator");

      if (mrss->generator_uri)
	{
	  func (obj, " uri=\"");
	  __mrss_write_string (func, obj, mrss->generator_uri);
	  func (obj, "\"");
	}

      if (mrss->generator_version)
	{
	  func (obj, " version=\"");
	  __mrss_write_string (func, obj, mrss->generator_version);
	  func (obj, "\"");
	}

      func (obj, ">");
      __mrss_write_string (func, obj, mrss->generator);
      func (obj, "</generator>\n");
    }

  if (mrss->image_url)
    {
      func (obj, "  <icon>");
      __mrss_write_string (func, obj, mrss->image_url);
      func (obj, "</icon>\n");
    }

  if (mrss->image_logo)
    {
      func (obj, "  <logo>");
      __mrss_write_string (func, obj, mrss->image_logo);
      func (obj, "</logo>\n");
    }

  __mrss_write_real_atom_category (mrss->category, func, obj);

  __mrss_write_real_atom_entry (mrss, func, obj);

  if (mrss->other_tags)
    __mrss_write_real_tag (mrss->other_tags, func, obj, 0);

  func (obj, "</feed>\n");

  return MRSS_OK;
}

static mrss_error_t
__mrss_write_real (mrss_t * mrss, void (*func) (void *, char *, ...),
		   void *obj)
{
  func (obj, "<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n");

  if (mrss->version == MRSS_VERSION_1_0)
    func (obj,
	  "<rdf:RDF\n"
	  "  xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\"\n"
	  "  xmlns=\"http://purl.org/rss/1.0/\">\n");

  else if (mrss->version == MRSS_VERSION_ATOM_1_0
	   || mrss->version == MRSS_VERSION_ATOM_0_3)
    return __mrss_write_atom (mrss, func, obj);

  else
    {
      func (obj, "<rss version=\"");

      switch (mrss->version)
	{
	case MRSS_VERSION_0_91:
	  func (obj, "0.91");
	  break;

	case MRSS_VERSION_0_92:
	  func (obj, "0.92");
	  break;

	case MRSS_VERSION_2_0:
	  func (obj, "2.0");
	  break;

	default:
	  break;
	}

      func (obj, "\">\n");
    }

  if (mrss->version == MRSS_VERSION_1_0 && mrss->about)
    {
      func (obj, "  <channel rdf:about=\"");
      __mrss_write_string (func, obj, mrss->about);
      func (obj, "\">\n");
    }

  else
    func (obj, "  <channel>\n");

  if (mrss->title)
    {
      func (obj, "    <title>");
      __mrss_write_string (func, obj, mrss->title);
      func (obj, "</title>\n");
    }

  if (mrss->description)
    {
      func (obj, "    <description>");
      __mrss_write_string (func, obj, mrss->description);
      func (obj, "</description>\n");
    }

  if (mrss->link)
    {
      func (obj, "    <link>");
      __mrss_write_string (func, obj, mrss->link);
      func (obj, "</link>\n");
    }

  if (mrss->language && mrss->version != MRSS_VERSION_1_0)
    {
      func (obj, "    <language>");
      __mrss_write_string (func, obj, mrss->language);
      func (obj, "</language>\n");
    }

  if (mrss->rating && mrss->version != MRSS_VERSION_1_0)
    {
      func (obj, "    <rating>");
      __mrss_write_string (func, obj, mrss->rating);
      func (obj, "</rating>\n");
    }

  if (mrss->copyright && mrss->version != MRSS_VERSION_1_0)
    {
      func (obj, "    <copyright>");
      __mrss_write_string (func, obj, mrss->copyright);
      func (obj, "</copyright>\n");
    }

  if (mrss->pubDate && mrss->version != MRSS_VERSION_1_0)
    {
      func (obj, "    <pubDate>");
      __mrss_write_string (func, obj, mrss->pubDate);
      func (obj, "</pubDate>\n");
    }

  if (mrss->lastBuildDate && mrss->version != MRSS_VERSION_1_0)
    {
      func (obj, "    <lastBuildDate>");
      __mrss_write_string (func, obj, mrss->lastBuildDate);
      func (obj, "</lastBuildDate>\n");
    }

  if (mrss->docs && mrss->version != MRSS_VERSION_1_0)
    {
      func (obj, "    <docs>");
      __mrss_write_string (func, obj, mrss->docs);
      func (obj, "</docs>\n");
    }

  if (mrss->managingeditor && mrss->version != MRSS_VERSION_1_0)
    {
      func (obj, "    <managingeditor>");
      __mrss_write_string (func, obj, mrss->managingeditor);
      func (obj, "</managingeditor>\n");
    }

  if (mrss->webMaster && mrss->version != MRSS_VERSION_1_0)
    {
      func (obj, "    <webMaster>");
      __mrss_write_string (func, obj, mrss->webMaster);
      func (obj, "</webMaster>\n");
    }

  if (mrss->version == MRSS_VERSION_2_0)
    {
      if (mrss->generator)
	{
	  func (obj, "    <generator>");
	  __mrss_write_string (func, obj, mrss->generator);
	  func (obj, "</generator>\n");
	}

      if (mrss->ttl)
	func (obj, "    <ttl>%d</ttl>\n", mrss->ttl);
    }

  if (mrss->version == MRSS_VERSION_1_0)
    {
      if (mrss->image_url)
	{
	  func (obj, "    <image rdf:resource=\"");
	  __mrss_write_string (func, obj, mrss->image_url);
	  func (obj, "\" />\n");
	}

      if (mrss->textinput_link)
	{
	  func (obj, "    <textinput rdf:resource=\"");
	  __mrss_write_string (func, obj, mrss->textinput_link);
	  func (obj, "\" />\n");
	}

      if (mrss->item)
	{
	  mrss_item_t *item = mrss->item;

	  func (obj, "    <items>\n" "      <rdf:Seq>\n");
	  while (item)
	    {
	      func (obj, "        <rdf:li rdf:resource=\"");
	      __mrss_write_string (func, obj, item->link);
	      func (obj, "\" />\n");
	      item = item->next;
	    }
	  func (obj, "      </rdf:Seq>\n" "    </items>\n");
	}

      func (obj, "  </channel>\n");
    }

  __mrss_write_real_image (mrss, func, obj);

  __mrss_write_real_textinput (mrss, func, obj);

  __mrss_write_real_cloud (mrss, func, obj);

  __mrss_write_real_skipHours (mrss, func, obj);

  __mrss_write_real_skipDays (mrss, func, obj);

  __mrss_write_real_category (mrss, mrss->category, func, obj);

  if (mrss->other_tags)
    __mrss_write_real_tag (mrss->other_tags, func, obj, 1);

  __mrss_write_real_item (mrss, func, obj);

  if (mrss->version != MRSS_VERSION_1_0)
    func (obj, "  </channel>\n" "</rss>\n");
  else
    func (obj, "</rdf:RDF>\n");

  return MRSS_OK;
}

static void
__mrss_file_write (void *obj, char *str, ...)
{
  va_list va;

  va_start (va, str);
  vfprintf ((FILE *) obj, str, va);
  va_end (va);
}

static void
__mrss_buffer_write (void *obj, char *str, ...)
{
  va_list va;
  char s[4096];
  int len;
  char **buffer = (char **) obj;

  va_start (va, str);
  len = vsnprintf (s, sizeof (s), str, va);
  va_end (va);

  if (!*buffer)
    {
      if (!(*buffer = (char *) malloc (sizeof (char) * (len + 1))))
	return;

      strncpy (*buffer, s, len + 1);
    }
  else
    {
      if (!(*buffer = (char *) realloc (*buffer,
					sizeof (char) * (strlen (*buffer) +
							 len + 1))))
	return;
      strncat (*buffer, s, len + 1);
    }
}

/*************************** EXTERNAL FUNCTION ******************************/

mrss_error_t
mrss_write_file (mrss_t * mrss, const char *file)
{
  FILE *fl;
  mrss_error_t ret;

  if (!mrss || !file)
    return MRSS_ERR_DATA;

  if (!(fl = fopen (file, "wb")))
    return MRSS_ERR_POSIX;

  ret = __mrss_write_real (mrss, __mrss_file_write, fl);
  fclose (fl);

  return ret;
}

mrss_error_t
mrss_write_buffer (mrss_t * mrss, char **buffer)
{
  if (!mrss || !buffer)
    return MRSS_ERR_DATA;

  return __mrss_write_real (mrss, __mrss_buffer_write, buffer);
}

/* EOF */
