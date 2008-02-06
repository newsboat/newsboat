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

static void
__nxml_write_escape_string (void (*func) (void *, char *, ...), void *obj,
			    char *str)
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

static int
__nxml_write_haslines (char *what)
{
  while (what && *what)
    {
      if (*what == '\n')
	return 1;
      what++;
    }

  return 0;
}

static void
__nxml_write_indent (void (*func) (void *, char *, ...), void *obj,
		     int indent)
{
  int i;
  for (i = 0; i < indent; i++)
    func (obj, "  ");
}

static void
__nxml_write_newline (void (*func) (void *, char *, ...), void *obj)
{
  func (obj, "\n");
}

static void
__nxml_write_data_text (nxml_data_t * data,
			void (*func) (void *, char *, ...), void *obj)
{
  __nxml_write_escape_string (func, obj, data->value);
}

static void
__nxml_write_data_comment (nxml_data_t * data,
			   void (*func) (void *, char *, ...), void *obj)
{
  func (obj, "<!--%s-->", data->value);
}

static void
__nxml_write_data_pi (nxml_data_t * data, void (*func) (void *, char *, ...),
		      void *obj)
{
  func (obj, "<?%s?>", data->value);
}

static void
__nxml_write_data_doctype (nxml_doctype_t * data,
			   void (*func) (void *, char *, ...), void *obj)
{
  func (obj, "<!DOCTYPE %s %s>", data->name, data->value);
}

static void
__nxml_write_data_element (nxml_data_t * data,
			   void (*func) (void *, char *, ...), void *obj)
{
  nxml_attr_t *attr;

  func (obj, "<");
  if (data->ns && data->ns->prefix)
    func (obj, "%s:", data->ns->prefix);
  func (obj, "%s", data->value);

  attr = data->attributes;
  while (attr)
    {
      func (obj, " ");

      if (attr->ns && attr->ns->prefix)
	func (obj, "%s:", attr->ns->prefix);
      func (obj, "%s=\"", attr->name);

      __nxml_write_escape_string (func, obj, attr->value);
      func (obj, "\"");
      attr = attr->next;
    }

  if (!data->children)
    func (obj, " /");

  func (obj, ">");
}

static void
__nxml_write_data (nxml_t * nxml, nxml_data_t * data,
		   void (*func) (void *, char *, ...), void *obj, int indent)
{
  nxml_data_t *tmp;
  int i;

  switch (data->type)
    {
    case NXML_TYPE_TEXT:
      if (data->children || data->next || __nxml_write_haslines (data->value)
	  || (data->parent && data->parent->children != data))
	{
	  i = 1;
	  __nxml_write_indent (func, obj, indent);
	}
      else
	i = 0;

      __nxml_write_data_text (data, func, obj);

      if (i)
	__nxml_write_newline (func, obj);
      break;

    case NXML_TYPE_COMMENT:
      __nxml_write_indent (func, obj, indent);
      __nxml_write_data_comment (data, func, obj);
      __nxml_write_newline (func, obj);
      break;

    case NXML_TYPE_PI:
      __nxml_write_indent (func, obj, indent);
      __nxml_write_data_pi (data, func, obj);
      __nxml_write_newline (func, obj);
      break;

    default:
      __nxml_write_indent (func, obj, indent);
      __nxml_write_data_element (data, func, obj);

      if (!data->children || data->children->type != NXML_TYPE_TEXT
	  || data->children->next
	  || __nxml_write_haslines (data->children->value))
	__nxml_write_newline (func, obj);

      break;
    }

  if (data->children)
    {
      tmp = data->children;

      while (tmp)
	{
	  __nxml_write_data (nxml, tmp, func, obj, indent + 1);
	  tmp = tmp->next;
	}

      if (data->type == NXML_TYPE_ELEMENT)
	{
	  if (!data->children || data->children->type != NXML_TYPE_TEXT
	      || data->children->next || data->children->children
	      || __nxml_write_haslines (data->children->value))
	    __nxml_write_indent (func, obj, indent);

	  func (obj, "</");
	  if (data->ns && data->ns->prefix)
	    func (obj, "%s:", data->ns->prefix);
	  func (obj, "%s>", data->value);

	  __nxml_write_newline (func, obj);
	}
    }
}

static nxml_error_t
__nxml_write_real (nxml_t * nxml, void (*func) (void *, char *, ...),
		   void *obj)
{
  nxml_data_t *data;
  nxml_doctype_t *doctype;

  func (obj, "<?xml");

  func (obj, " version=\"");

  switch (nxml->version)
    {
    case NXML_VERSION_1_0:
      func (obj, "1.0");
      break;
    default:
      func (obj, "1.1");
    }

  func (obj, "\"");

  if (nxml->encoding)
    func (obj, " encoding=\"%s\"", nxml->encoding);

  func (obj, " standalone=\"%s\"?>\n\n", nxml->standalone ? "yes" : "no");

  doctype = nxml->doctype;

  while (doctype)
    {
      __nxml_write_indent (func, obj, 0);
      __nxml_write_data_doctype (doctype, func, obj);
      __nxml_write_newline (func, obj);

      doctype = doctype->next;
    }

  data = nxml->data;

  while (data)
    {
      __nxml_write_data (nxml, data, func, obj, 0);

      data = data->next;
    }

  return NXML_OK;
}

static void
__nxml_file_write (void *obj, char *str, ...)
{
  va_list va;

  va_start (va, str);
  vfprintf ((FILE *) obj, str, va);
  va_end (va);
}

static void
__nxml_buffer_write (void *obj, char *str, ...)
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

      strcpy (*buffer, s);
    }
  else
    {
      if (!(*buffer = (char *) realloc (*buffer,
					sizeof (char) * (strlen (*buffer) +
							 len + 1))))
	return;

      strcat (*buffer, s);
    }
}

/*************************** EXTERNAL FUNCTION ******************************/

nxml_error_t
nxml_write_file (nxml_t * nxml, const char *file)
{
  FILE *fl;
  nxml_error_t ret;

  if (!nxml || !file)
    return NXML_ERR_DATA;

  if (!(fl = fopen (file, "wb")))
    return NXML_ERR_POSIX;

  ret = __nxml_write_real (nxml, __nxml_file_write, fl);
  fclose (fl);

  return ret;
}

nxml_error_t
nxml_write_buffer (nxml_t * nxml, char **buffer)
{
  if (!nxml || !buffer)
    return NXML_ERR_DATA;

  return __nxml_write_real (nxml, __nxml_buffer_write, buffer);
}

/* EOF */
