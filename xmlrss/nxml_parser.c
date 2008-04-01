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
#include "nxml_internal.h"

static int
__nxml_parse_unique_attribute (nxml_attr_t * attr, const char *name)
{
  /* 
   * Rule [40] - Well-formedness contraint: Unique Att Spec
   */

  while (attr)
    {
      if (!strcmp (attr->name, name))
	return 1;

      attr = attr->next;
    }

  return 0;
}

static char *
__nxml_parse_string (nxml_t * doc, char *buffer, size_t size)
{
  char *real;
  unsigned int i;
  unsigned int q;
  __nxml_string_t *ret;

  ret = __nxml_string_new ();

  for (q = i = 0; i < size; i++)
    {
      if (*(buffer + i) == 0xd)
	continue;

      if (*(buffer + i) == 0xa || *(buffer + i) == 0x9
	  || *(buffer + i) == 0x20)
	{
	  if (!q)
	    {
	      if (!doc->priv.textindent)
		q = 1;

	      __nxml_string_add (ret, buffer + i, 1);
	    }
	}

      else if (*(buffer + i) == '&')
	{
	  if (!strncmp (buffer + i, "&lt;", 4))
	    {
	      __nxml_string_add (ret, "<", 1);
	      i += 3;
	      q = 0;
	    }

	  else if (!strncmp (buffer + i, "&gt;", 4))
	    {
	      __nxml_string_add (ret, ">", 1);
	      i += 3;
	      q = 0;
	    }

	  else if (!strncmp (buffer + i, "&amp;", 5))
	    {
	      __nxml_string_add (ret, "&", 1);
	      i += 4;
	      q = 0;
	    }

	  else if (!strncmp (buffer + i, "&apos;", 6))
	    {
	      __nxml_string_add (ret, "'", 1);
	      i += 5;
	      q = 0;
	    }

	  else if (!strncmp (buffer + i, "&quot;", 6))
	    {
	      __nxml_string_add (ret, "\"", 1);
	      i += 5;
	      q = 0;
	    }

	  else if (*(buffer + i + 1) == '#')
	    {
	      char buf[1024];
	      unsigned int k = i;
	      unsigned int last;

	      while (*(buffer + k) != ';' && k < size)
		k++;

	      last = k - (i + 2) > sizeof (buf) ? sizeof (buf) : k - (i + 2);
	      strncpy (buf, buffer + i + 2, last);
	      buf[last] = 0;

	      if (buf[0] != 'x')
		last = atoi (buf);
	      else
		last = __nxml_atoi (&buf[1]);

	      if ((last =
		   __nxml_int_charset (last, (unsigned char *) buf,
				       doc->encoding)) > 0)
		__nxml_string_add (ret, buf, last);
	      else
		__nxml_string_add (ret, buffer + i, 1);

	      i += k - i;
	      q = 0;
	    }

	  else
	    {
	      __nxml_entity_t *entity;
	      char buf[1024];
	      unsigned int k = i;
	      unsigned int last;

	      while (*(buffer + k) != ';' && k < size)
		k++;

	      last = k - (i + 1) > sizeof (buf) ? sizeof (buf) : k - (i + 1);
	      strncpy (buf, buffer + i + 1, last);
	      buf[last] = 0;

	      for (entity = doc->priv.entities; entity; entity = entity->next)
		{
		  if (!strcmp (entity->name, buf))
		    {
		      __nxml_string_add (ret, entity->entity,
					 strlen (entity->entity));
		      break;
		    }

		}

	      if (!entity)
		__nxml_string_add (ret, buffer + i, 1);
	      else
		i += strlen (entity->name) + 1;

	      q = 0;
	    }
	}

      else
	{
	  q = 0;
	  __nxml_string_add (ret, buffer + i, 1);
	}
    }

  if (!(real = __nxml_string_free (ret)))
    real = strdup ("");

  return real;
}

static char *
__nxml_parse_get_attr (nxml_t * doc, char **buffer, size_t * size)
{
  char attr[1024];
  unsigned int i;
  int byte;
  int64_t ch;

  if (!*size)
    return NULL;

  if (!__NXML_NAMESTARTCHARS)
    return NULL;

  memcpy (&attr[0], *buffer, byte);

  i = byte;
  (*buffer) += byte;
  (*size) -= byte;

  while (__NXML_NAMECHARS && *size && i < sizeof (attr) - 1)
    {
      memcpy (&attr[i], *buffer, byte);

      i += byte;
      (*buffer) += byte;
      (*size) -= byte;
    }

  if (**buffer != 0x20 && **buffer != 0x9 && **buffer != 0xa
      && **buffer != 0xd && **buffer != '=')
    {
      (*buffer) -= i;
      (*size) += i;
      return NULL;
    }

  i += __nxml_escape_spaces (doc, buffer, size);

  if (**buffer != '=')
    {
      (*buffer) -= i;
      (*size) += i;
      return NULL;
    }

  (*buffer)++;
  (*size)--;

  __nxml_escape_spaces (doc, buffer, size);

  attr[i] = 0;
  return strdup (attr);
}

static nxml_error_t
__nxml_parse_get_attribute (nxml_t * doc, char **buffer, size_t * size,
			    nxml_attr_t ** attr)
{
  /* 
   * Rule [41] - Attribute ::= Name Eq AttValue
   * Rule [25] - Eq ::= S? '=' S?
   * Rule [5]  - Name ::= NameStartChar (NameChar)*
   * Rule [4]  - NameStartChar ::= ":" | [A-Z] | ["_"] | [a-z] | Unicode...
   * Rule [4a] - NameChar ::= NameStarChar | "-" | "." | [0-9] | Unicode...
   * Rule [10] - AttValue ::= '"' ([^<&"] | Reference)* '"' |
   *                          "'" ([^<&'] | Reference)* "'"
   */
  char *tag, *value, *v;

  if (!*size)
    return NXML_OK;

  *attr = NULL;

  __nxml_escape_spaces (doc, buffer, size);

  if (!(tag = __nxml_parse_get_attr (doc, buffer, size)))
    return NXML_OK;

  if (!(value = __nxml_get_value (doc, buffer, size)))
    {
      free (tag);

      if (doc->priv.func)
	doc->priv.func ("%s: expected value of attribute (line %d)\n",
			doc->file ? doc->file : "", doc->priv.line);
      return NXML_ERR_PARSER;
    }

  if (!(v = __nxml_parse_string (doc, value, strlen (value))))
    {
      free (tag);
      return NXML_ERR_POSIX;
    }

  free (value);
  value = v;

  __nxml_escape_spaces (doc, buffer, size);

  if (!(*attr = (nxml_attr_t *) calloc (1, sizeof (nxml_attr_t))))
    {
      free (tag);
      free (value);

      return NXML_ERR_POSIX;
    }

  (*attr)->name = tag;
  (*attr)->value = value;

  return NXML_OK;
}

static nxml_error_t
__nxml_parse_cdata (nxml_t * doc, char **buffer, size_t * size,
		    nxml_data_t ** data)
{
  /*
   * Rule [18] - CDSect ::= CDStart CData CDEnd 
   * Rule [19] - CDStart ::= '<![CDATA['
   * Rule [20] - CData ::= (Char * - (Char * ']]>' Char *))
   * Rule [21] - CDEnd ::= ']]>'
   */

  unsigned int i = 0;
  nxml_data_t *t;
  char *value;

  while (i < *size)
    {
      if (!strncmp (*buffer + i, "]]>", 3))
	break;

      if (*(*buffer + i) == 0xa && doc->priv.func)
	doc->priv.line++;

      i++;
    }

  if (strncmp (*buffer + i, "]]>", 3))
    {
      if (doc->priv.func)
	doc->priv.func ("%s: expected ']]>' as close of a CDATA (line %d)\n",
			doc->file ? doc->file : "", doc->priv.line);

      return NXML_ERR_PARSER;
    }

  if (!(t = (nxml_data_t *) calloc (1, sizeof (nxml_data_t))))
    return NXML_ERR_POSIX;

  t->doc = doc;

  if (!(value = (char *) malloc (sizeof (char) * (i + 1))))
    {
      free (t);
      return NXML_ERR_POSIX;
    }

  strncpy (value, *buffer, i);
  value[i] = 0;

  t->value = value;
  t->type = NXML_TYPE_TEXT;

  (*buffer) += i + 3;
  (*size) -= i + 3;

  *data = t;

  return NXML_OK;
}

static void
__nxml_parse_entity (nxml_t * doc, char **buffer, size_t * size)
{
  /*
   * [70]       EntityDecl ::=          GEDecl | PEDecl
   * [71]       GEDecl     ::=          '<!ENTITY' S Name S EntityDef S? '>'
   * [72]       PEDecl     ::=          '<!ENTITY' S '%' S Name S PEDef S? '>'
   * [73]       EntityDef  ::=          EntityValue | (ExternalID NDataDecl?)
   * [74]       PEDef      ::=          EntityValue | ExternalID
   */

  unsigned int i;
  char name[1024];
  char *entity;
  int byte;
  int64_t ch;

  __nxml_escape_spaces (doc, buffer, size);

  if (strncmp (*buffer, "<!ENTITY", 8))
    {
      unsigned int q = i = 0;

      while ((*(*buffer + i) != '>' || q) && i < *size)
	{
	  if (*(*buffer + i) == '<')
	    q++;

	  else if (*(*buffer + i) == '>')
	    q--;

	  i++;
	}

      if (*(*buffer) == '>')
	i++;

      (*buffer) += i;
      (*size) -= i;
      return;
    }

  *buffer += 8;
  *size -= 8;

  __nxml_escape_spaces (doc, buffer, size);

  /* Name */
  if (!__NXML_NAMESTARTCHARS)
    {
      int q = i = 0;

      while ((*(*buffer + i) != '>' || q) && i < *size)
	{
	  if (*(*buffer + i) == '<')
	    q++;

	  else if (*(*buffer + i) == '>')
	    q--;

	  i++;
	}

      if (*(*buffer) == '>')
	i++;

      (*buffer) += i;
      (*size) -= i;
      return;
    }

  memcpy (&name[0], *buffer, byte);

  i = byte;
  (*buffer) += byte;
  (*size) -= byte;

  while (__NXML_NAMECHARS && *size && i < sizeof (name) - 1)
    {
      memcpy (&name[i], *buffer, byte);

      i += byte;
      (*buffer) += byte;
      (*size) -= byte;
    }

  name[i] = 0;

  if (!i || !strcmp (name, "%"))
    {
      int q = i = 0;

      while ((*(*buffer + i) != '>' || q) && i < *size)
	{
	  if (*(*buffer + i) == '<')
	    q++;

	  else if (*(*buffer + i) == '>')
	    q--;

	  i++;
	}

      if (*(*buffer) == '>')
	i++;

      (*buffer) += i;
      (*size) -= i;
      return;
    }

  __nxml_escape_spaces (doc, buffer, size);

  entity = __nxml_get_value (doc, buffer, size);

  __nxml_escape_spaces (doc, buffer, size);

  if (**buffer == '>')
    {
      (*buffer)++;
      (*size)--;
    }

  if (entity)
    {
      __nxml_entity_t *e;

      if (!(e = calloc (1, sizeof (__nxml_entity_t))))
	{
	  free (entity);
	  return;
	}

      if (!(e->name = strdup (name)))
	{
	  free (e);
	  free (entity);
	  return;
	}

      e->entity = entity;

      if (!doc->priv.entities)
	doc->priv.entities = e;
      else
	{
	  __nxml_entity_t *tmp = doc->priv.entities;

	  while (tmp->next)
	    tmp = tmp->next;

	  tmp->next = e;
	}
    }
}

static nxml_error_t
__nxml_parse_doctype (nxml_t * doc, char **buffer, size_t * size,
		      int *doctype)
{
  /*
   * Rule [28] - doctypedecl ::= '<!DOCTYPE' S Name (S ExternalID)? S? 
   *                             ('[' intSubset '] S?)? '>'
   */

  nxml_doctype_t *t;
  nxml_doctype_t *tmp;
  char str[1024];
  char *value;
  unsigned int i;
  int byte;
  int64_t ch;
  unsigned int q = 0;

  __nxml_escape_spaces (doc, buffer, size);

  if (!__NXML_NAMESTARTCHARS)
    {
      if (doc->priv.func)
	doc->priv.func ("%s: abnormal char '%c' (line %d)\n",
			doc->file ? doc->file : "", **buffer, doc->priv.line);
      return NXML_ERR_PARSER;
    }

  memcpy (&str[0], *buffer, byte);

  i = byte;
  (*buffer) += byte;
  (*size) -= byte;

  while (__NXML_NAMECHARS && *size && i < sizeof (str) - 1)
    {
      memcpy (&str[i], *buffer, byte);

      i += byte;
      (*buffer) += byte;
      (*size) -= byte;
    }

  str[i] = 0;

  if (!(t = (nxml_doctype_t *) calloc (1, sizeof (nxml_doctype_t))))
    return NXML_ERR_POSIX;

  t->doc = doc;

  if (!(t->name = strdup (str)))
    {
      free (t);
      return NXML_ERR_POSIX;
    }

  __nxml_escape_spaces (doc, buffer, size);

  i = 0;
  while ((*(*buffer + i) != '>' || q) && i < *size)
    {
      if (*(*buffer + i) == '<')
	q++;

      else if (*(*buffer + i) == '>')
	q--;

      if (*(*buffer + i) == 0xa && doc->priv.func)
	doc->priv.line++;

      i++;
    }

  if (*(*buffer + i) != '>')
    {
      if (doc->priv.func)
	doc->priv.func ("%s: expected '>' as close of a doctype (line %d)\n",
			doc->file ? doc->file : "", doc->priv.line);

      free (t->value);
      free (t);
      return NXML_ERR_PARSER;
    }

  if (!(value = __nxml_parse_string (doc, *buffer, i)))
    {
      free (t->value);
      free (t);
      return NXML_ERR_POSIX;
    }

  t->value = value;

  (*buffer) += i + 1;
  (*size) -= i + 1;

  tmp = doc->doctype;
  if (!tmp)
    doc->doctype = t;

  else
    {
      while (tmp->next)
	tmp = tmp->next;

      tmp->next = t;
    }

  *doctype = 1;

  while (value && *value && *value != '[')
    value++;

  if (value && *value == '[')
    {
      unsigned int s;

      value++;
      s = strlen (value);

      while (s > 0 && value && *value)
	__nxml_parse_entity (doc, &value, (size_t *)&s);
    }

  return NXML_OK;
}

static nxml_error_t
__nxml_parse_comment (nxml_t * doc, char **buffer, size_t * size,
		      nxml_data_t ** data)
{
  /* 
   * Rule [15] - Comment ::= '<!--' ((Char - '-') | ('-' (Char - '-')))* '-->'
   */

  unsigned int i = 0;
  nxml_data_t *t;
  char *value;

  while (strncmp (*buffer + i, "-->", 3) && i < *size)
    {
      if (*(*buffer + i) == 0xa && doc->priv.func)
	doc->priv.line++;
      i++;
    }

  if (strncmp (*buffer + i, "-->", 3))
    {
      if (doc->priv.func)
	doc->priv.func ("%s: expected '--' as close comment (line %d)\n",
			doc->file ? doc->file : "", doc->priv.line);

      return NXML_ERR_PARSER;
    }

  if (!(t = (nxml_data_t *) calloc (1, sizeof (nxml_data_t))))
    return NXML_ERR_POSIX;

  t->doc = doc;

  if (!(value = __nxml_parse_string (doc, *buffer, i)))
    {
      free (t);
      return NXML_ERR_POSIX;
    }

  t->value = value;

  (*buffer) += i + 3;
  (*size) -= i + 3;

  t->type = NXML_TYPE_COMMENT;

  *data = t;

  return NXML_OK;
}

static nxml_error_t
__nxml_parse_pi (nxml_t * doc, char **buffer, size_t * size,
		 nxml_data_t ** data)
{
  /* 
   * Rule [16] - PI ::= '<?' PITarget (S (Char * - (Char * '?>' Char *)))? 
   *                    '?>'
   * Rule [17] - PITarget ::= Name - (('X' | 'x') ('M' | 'm') ('L' | 'l'))
   */

  unsigned int i = 0;
  nxml_data_t *t;
  char *value;

  if (!*size)
    return NXML_OK;

  *data = NULL;

  (*buffer) += 1;
  (*size) -= 1;

  while (strncmp (*buffer + i, "?>", 2) && i < *size)
    {
      if (*(*buffer + i) == 0xa && doc->priv.func)
	doc->priv.line++;
      i++;
    }

  if (strncmp (*buffer + i, "?>", 2))
    {
      if (doc->priv.func)
	doc->priv.func ("%s: expected '?' as close pi tag (line %d)\n",
			doc->file ? doc->file : "", doc->priv.line);

      return NXML_ERR_PARSER;
    }

  if (!strncasecmp (*buffer, "?xml", 4))
    {
      if (doc->priv.func)
	doc->priv.func ("%s: pi xml is reserved! (line %d)\n",
			doc->file ? doc->file : "", doc->priv.line);

      return NXML_ERR_PARSER;
    }

  if (!(t = (nxml_data_t *) calloc (1, sizeof (nxml_data_t))))
    return NXML_ERR_POSIX;

  t->doc = doc;

  if (!(value = __nxml_parse_string (doc, *buffer, i)))
    {
      free (t);
      return NXML_ERR_POSIX;
    }

  t->value = value;

  (*buffer) += i + 2;
  (*size) -= i + 2;

  t->type = NXML_TYPE_PI;

  *data = t;

  return NXML_OK;
}

static nxml_error_t
__nxml_parse_other (nxml_t * doc, char **buffer, size_t * size,
		    nxml_data_t ** data, int *doctype)
{
  /* Tags '<!'... */

  *data = NULL;
  *doctype = 0;

  if (!*size)
    return NXML_OK;

  (*buffer) += 1;
  (*size) -= 1;

  __nxml_escape_spaces (doc, buffer, size);

  if (!strncmp (*buffer, "[CDATA[", 7))
    {
      (*buffer) += 7;
      (*size) -= 7;

      return __nxml_parse_cdata (doc, buffer, size, data);
    }

  else if (!strncmp (*buffer, "--", 2))
    {
      (*buffer) += 2;
      (*size) -= 2;

      return __nxml_parse_comment (doc, buffer, size, data);
    }

  else if (!strncmp (*buffer, "DOCTYPE", 7))
    {
      (*buffer) += 7;
      (*size) -= 7;

      return __nxml_parse_doctype (doc, buffer, size, doctype);
    }

  else
    {
      if (doc->priv.func)
	doc->priv.func ("%s: abnormal tag (line %d)\n",
			doc->file ? doc->file : "", doc->priv.line);

      return NXML_ERR_PARSER;
    }
}

static nxml_error_t
__nxml_parse_text (nxml_t * doc, char **buffer, size_t * size,
		   nxml_data_t ** data)
{
  unsigned int i = 0;
  nxml_data_t *t;
  char *value;
  char *value_new;

  *data = NULL;

  if (!*size)
    return NXML_OK;

  while (*(*buffer + i) != '<' && i < *size)
    {
      if (*(*buffer + i) == 0xa && doc->priv.func)
	doc->priv.line++;
      i++;
    }

  if (!(t = (nxml_data_t *) calloc (1, sizeof (nxml_data_t))))
    return NXML_ERR_POSIX;

  t->doc = doc;

  if (!(value = __nxml_parse_string (doc, *buffer, i)))
    {
      free (t);
      return NXML_ERR_POSIX;
    }

  (*buffer) += i;
  (*size) -= i;

  value_new = __nxml_trim (value);
  free (value);

  if (!value_new)
    {
      free (t);
      return NXML_ERR_POSIX;
    }

  t->value = value_new;
  t->type = NXML_TYPE_TEXT;

  *data = t;

  return NXML_OK;
}

static nxml_error_t
__nxml_parse_close (nxml_t * doc, char **buffer, size_t * size,
		    nxml_data_t ** data)
{
  /* 
   * Rule [42] - ETag ::= '</' Name S? '>'
   */

  nxml_data_t *tag;
  char str[1024];
  unsigned int i;
  int byte;
  int64_t ch;

  *data = NULL;

  if (!*size)
    return NXML_OK;

  if (!__NXML_NAMESTARTCHARS)
    {
      if (doc->priv.func)
	doc->priv.func ("%s: abnormal char '%c' (line %d)\n",
			doc->file ? doc->file : "", **buffer, doc->priv.line);
      return NXML_ERR_PARSER;
    }

  memcpy (&str[0], *buffer, byte);

  i = byte;
  (*buffer) += byte;
  (*size) -= byte;

  while (__NXML_NAMECHARS && *size && i < sizeof (str) - 1)
    {
      memcpy (&str[i], *buffer, byte);

      i += byte;
      (*buffer) += byte;
      (*size) -= byte;
    }

  str[i] = 0;

  __nxml_escape_spaces (doc, buffer, size);

  if (**buffer != '>')
    {
      if (doc->priv.func)
	doc->priv.func ("%s: expected char '>' after tag %s (line %d)\n",
			doc->file ? doc->file : "", str, doc->priv.line);
      return NXML_ERR_PARSER;
    }

  (*buffer) += 1;
  (*size) -= 1;

  if (!(tag = (nxml_data_t *) calloc (1, sizeof (nxml_data_t))))
    return NXML_ERR_POSIX;

  tag->doc = doc;

  if (!(tag->value = strdup (str)))
    {
      free (tag);
      return NXML_ERR_POSIX;
    }

  tag->type = NXML_TYPE_ELEMENT_CLOSE;

  *data = tag;

  return NXML_OK;
}

static nxml_error_t
__nxml_parse_get_tag (nxml_t * doc, char **buffer, size_t * size,
		      nxml_data_t ** data, int *doctype)
{
  /* Parse the element... */
  nxml_attr_t *attr, *last;
  nxml_data_t *tag, *child, *child_last;
  nxml_error_t err;

  char str[1024];
  unsigned int i;
  int closed = 0;
  int byte;
  int64_t ch;

  *data = NULL;
  *doctype = 0;

  if (!*size)
    return NXML_OK;

  __nxml_escape_spaces (doc, buffer, size);

  /* Text */
  if (**buffer != '<')
    return __nxml_parse_text (doc, buffer, size, data);

  (*buffer) += 1;
  (*size) -= 1;

  /* Comment, CData, DocType or other elements */
  if (**buffer == '!')
    return __nxml_parse_other (doc, buffer, size, data, doctype);

  /* PI */
  else if (**buffer == '?')
    return __nxml_parse_pi (doc, buffer, size, data);

  /* Close tag */
  else if (**buffer == '/')
    {
      (*buffer) += 1;
      (*size) -= 1;
      return __nxml_parse_close (doc, buffer, size, data);
    }

  __nxml_escape_spaces (doc, buffer, size);

  if (!__NXML_NAMESTARTCHARS)
    {
      if (doc->priv.func)
	doc->priv.func ("%s: abnormal char '%c' (line %d)\n",
			doc->file ? doc->file : "", **buffer, doc->priv.line);
      return NXML_ERR_PARSER;
    }

  memcpy (&str[0], *buffer, byte);

  i = byte;
  (*buffer) += byte;
  (*size) -= byte;

  while (__NXML_NAMECHARS && *size && i < sizeof (str) - 1)
    {
      memcpy (&str[i], *buffer, byte);

      i += byte;
      (*buffer) += byte;
      (*size) -= byte;
    }

  str[i] = 0;

  if (**buffer != 0x20 && **buffer != 0x9 && **buffer != 0xa
      && **buffer != 0xd && **buffer != '>' && **buffer != '/')
    {
      (*buffer) -= i;
      (*size) += i;

      if (doc->priv.func)
	doc->priv.func ("%s: abnormal char '%c' after tag %s (line %d)\n",
			doc->file ? doc->file : "", *(*buffer + i), str,
			doc->priv.line);

      return NXML_ERR_PARSER;
    }

  if (!(tag = (nxml_data_t *) calloc (1, sizeof (nxml_data_t))))
    return NXML_ERR_POSIX;

  tag->doc = doc;

  if (!(tag->value = strdup (str)))
    {
      free (tag);
      return NXML_ERR_POSIX;
    }

  last = NULL;

  /* Get attribute: */
  while (!(err = __nxml_parse_get_attribute (doc, buffer, size, &attr))
	 && attr)
    {
      if (!*size)
	    return NXML_ERR_PARSER;

      if (__nxml_parse_unique_attribute (tag->attributes, attr->name))
	{
	  if (doc->priv.func)
	    doc->priv.
	      func ("%s: Duplicate attribute '%s' in tag '%s' (line %d)\n",
		    doc->file ? doc->file : "", attr->name, tag->value,
		    doc->priv.line);

	  nxml_free_attribute (attr);
	  nxml_free_data (tag);

	  return NXML_ERR_PARSER;
	}

      if (last)
	last->next = attr;
      else
	tag->attributes = attr;

      last = attr;
    }

  if (err)
    {
      nxml_free_data (tag);
      return err;
    }

  /* Closed element */
  if (**buffer == '/')
    {
      closed++;
      (*buffer) += 1;
      (*size) -= 1;

      __nxml_escape_spaces (doc, buffer, size);
    }

  if (**buffer != '>')
    {
      if (doc->priv.func)
	doc->priv.func ("%s: expected char '>' after tag %s (line %d)\n",
			doc->file ? doc->file : "", tag->value,
			doc->priv.line);

      nxml_free_data (tag);
      return NXML_ERR_PARSER;
    }

  (*buffer) += 1;
  (*size) -= 1;

  if (!closed)
    {
      child_last = NULL;

      /* Search children: */
      while (!
	     (err = __nxml_parse_get_tag (doc, buffer, size, &child, doctype))
	     && (*doctype || child))
	{
	  if (*doctype)
	    continue;

	  /* If the current child, break: */
	  if (child->type == NXML_TYPE_ELEMENT_CLOSE)
	    {
	      if (!strcmp (child->value, tag->value))
		{
		  closed = 1;
		  nxml_free_data (child);
		  break;
		}

	      else
		{
		  if (doc->priv.func)
		    doc->priv.
		      func ("%s: expected tag '/%s' and not '%s' (line %d)\n",
			    doc->file ? doc->file : "", tag->value,
			    child->value, doc->priv.line);

		  nxml_free_data (child);
		  nxml_free_data (tag);

		  return NXML_ERR_PARSER;
		}
	    }

	  /* Set the parent */
	  child->parent = tag;

	  if (child_last)
	    child_last->next = child;
	  else
	    tag->children = child;

	  child_last = child;
	}

      if (err)
	{
	  nxml_free_data (tag);
	  return err;
	}
    }

  tag->type = NXML_TYPE_ELEMENT;

  if (!closed)
    {
      if (doc->priv.func)
	doc->priv.func ("%s: expected tag '/%s' (line %d)\n",
			doc->file ? doc->file : "", tag->value,
			doc->priv.line);

      nxml_free_data (tag);
      return NXML_ERR_PARSER;
    }

  *data = tag;
  return NXML_OK;
}

static nxml_error_t
__nxml_parse_buffer (nxml_t * nxml, const char *r_buffer, size_t r_size)
{
  /* 
   * Rule [1] - Document ::= prolog element Misc* - Char* RestrictedChar Char*
   */

  nxml_attr_t *attr;
  nxml_error_t err;
  nxml_charset_t charset;
  nxml_data_t *tag, *last, *root;
  int doctype;

  int freed;

  char *buffer = NULL;
  size_t size;

  if (!r_buffer || !nxml)
    return NXML_ERR_DATA;

  if (!r_size)
    r_size = strlen (r_buffer);

  switch ((freed =
	   __nxml_utf_detection (r_buffer, r_size, &buffer, &size, &charset)))
    {
    case 0:
      buffer = (char *)r_buffer;
      size = r_size;
      break;

    case -1:
      return NXML_ERR_POSIX;
    }

  nxml->priv.line = 1;
  nxml->version = NXML_VERSION_1_0;
  nxml->standalone = 1;

  /* 
   * Rule [22] - prolog ::= XMLDecl Misc* (doctypedecl Misc*)?
   * Rule [23] - XMLDecl ::= '<?xml' VersionInfo EncodingDecl? SDDecl? S? '?>'
   */
  if (!strncmp (buffer, "<?xml ", 6))
    {
      buffer += 6;
      size -= 6;

      if ((err =
	   __nxml_parse_get_attribute (nxml, &buffer, &size,
				       &attr)) != NXML_OK)
	{
	  nxml_empty (nxml);

	  if (freed)
	    free (buffer);

	  return err;
	}

      if (!attr)
	{
	  if (nxml->priv.func)
	    nxml->priv.func ("%s: expected 'version' attribute (line %d)\n",
			     nxml->file ? nxml->file : "", nxml->priv.line);

	  if (freed)
	    free (buffer);

	  return NXML_ERR_PARSER;
	}

      if (!strcmp (attr->value, "1.0"))
	nxml->version = NXML_VERSION_1_0;

      else if (!strcmp (attr->value, "1.1"))
	nxml->version = NXML_VERSION_1_1;

      else
	{
	  if (nxml->priv.func)
	    nxml->priv.
	      func ("libnxml 0.18.1-newsbeuter suports only xml 1.1 or 1.0 (line %d)\n",
		    nxml->priv.line);

	  if (freed)
	    free (buffer);

	  return NXML_ERR_PARSER;
	}

      nxml_free_attribute (attr);

      while (!(err = __nxml_parse_get_attribute (nxml, &buffer, &size, &attr))
	     && attr)
	{
	  if (!strcmp (attr->name, "standalone"))
	    {
	      if (!strcmp (attr->value, "yes"))
		nxml->standalone = 1;

	      else
		nxml->standalone = 0;
	    }

	  else if (!strcmp (attr->name, "encoding"))
	    {
	      nxml->encoding = strdup (attr->value);

	      if (!nxml->encoding)
		{
		  nxml_empty (nxml);
		  nxml_free_attribute (attr);

		  if (freed)
		    free (buffer);

		  return NXML_ERR_POSIX;
		}
	    }

	  else
	    {

	      if (nxml->priv.func)
		nxml->priv.func ("%s: unexpected attribute '%s' (line %d)\n",
				 nxml->file ? nxml->file : "", attr->name,
				 nxml->priv.line);

	      nxml_empty (nxml);
	      nxml_free_attribute (attr);

	      if (freed)
		free (buffer);

	      return NXML_ERR_PARSER;
	    }

	  nxml_free_attribute (attr);
	}

      if (err || strncmp (buffer, "?>", 2))
	{
	  if (nxml->priv.func)
	    nxml->priv.func ("%s expected '?>' (line %d)\n",
			     nxml->file ? nxml->file : "", nxml->priv.line);

	  nxml_empty (nxml);

	  if (freed)
	    free (buffer);

	  return NXML_ERR_PARSER;
	}

      buffer += 2;
      size -= 2;
    }

  root = last = NULL;
  while (!(err = __nxml_parse_get_tag (nxml, &buffer, &size, &tag, &doctype))
	 && (doctype || tag))
    {
      if (doctype)
	continue;

      if (tag->type == NXML_TYPE_ELEMENT && !root)
	root = tag;

      if (last)
	last->next = tag;
      else
	nxml->data = tag;

      last = tag;
    }

  if (err)
    {
      nxml_empty (nxml);

      if (freed)
	free (buffer);

      return NXML_ERR_PARSER;
    }

  if (!root)
    {
      if (nxml->priv.func)
	nxml->priv.func ("%s: No root element founded!\n",
			 nxml->file ? nxml->file : "");

      nxml_empty (nxml);

      if (freed)
	free (buffer);

      return NXML_ERR_PARSER;
    }

  if (freed)
    free (buffer);

  nxml->charset_detected = charset;

  __nxml_namespace_parse (nxml);

  return NXML_OK;
}

/* EXTERNAL FUNCTIONS *******************************************************/

nxml_error_t
nxml_parse_url (nxml_t * nxml, const char *url)
{
  nxml_error_t err;
  char *buffer;
  size_t size;

  if (!url || !nxml)
    return NXML_ERR_DATA;

  if ((err = nxml_download_file (nxml, url, &buffer, &size)) != NXML_OK)
    return err;

  if (nxml->file)
    free (nxml->file);

  if (!(nxml->file = strdup (url)))
    {
      nxml_empty (nxml);
      return NXML_ERR_POSIX;
    }

  nxml->size = size;

  nxml_empty (nxml);

  err = __nxml_parse_buffer (nxml, buffer, size);

  free (buffer);

  return err;
}

nxml_error_t
nxml_parse_file (nxml_t * nxml, const char *file)
{
  nxml_error_t err;
  char *buffer;
  struct stat st;
  int fd, ret;
  ssize_t len;

  if (!file || !nxml)
    return NXML_ERR_DATA;

  if (stat (file, &st))
    return NXML_ERR_POSIX;

  if ((fd = open (file, O_RDONLY)) < 0)
    return NXML_ERR_POSIX;

  if (!(buffer = (char *) malloc (sizeof (char) * (st.st_size + 1))))
    return NXML_ERR_POSIX;

  len = 0;

  while (len < st.st_size)
    {
      if ((ret = read (fd, buffer + len, st.st_size - len)) <= 0)
	{
	  free (buffer);
	  close (fd);
	  return NXML_ERR_POSIX;
	}

      len += ret;
    }

  buffer[len] = 0;
  close (fd);

  nxml_empty (nxml);

  if (nxml->file)
    free (nxml->file);

  if (!(nxml->file = strdup (file)))
    {
      nxml_empty (nxml);
      free (buffer);
      return NXML_ERR_POSIX;
    }

  nxml->size = st.st_size;

  err = __nxml_parse_buffer (nxml, buffer, st.st_size);

  free (buffer);
  return err;
}

nxml_error_t
nxml_parse_buffer (nxml_t * nxml, const char *buffer, size_t size)
{
  if (!buffer || !nxml)
    return NXML_ERR_DATA;

  nxml_empty (nxml);

  if (nxml->file)
    free (nxml->file);

  if (!(nxml->file = strdup ("buffer")))
    {
      nxml_empty (nxml);
      return NXML_ERR_POSIX;
    }

  nxml->size = size;

  return __nxml_parse_buffer (nxml, buffer, size);
}

/* EOF */
