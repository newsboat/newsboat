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

char *
__nxml_string_no_space (char *str)
{
  char *ret;
  int i, j;
  int q;
  int size;

  if (!str)
    return NULL;

  size = strlen (str);

  if (!(ret = (char *) malloc (sizeof (char) * (size + 1))))
    return NULL;

  for (q = i = j = 0; i < size; i++)
    {
      if (*(str + i) == 0xd)
	continue;

      if (*(str + i) == 0xa || *(str + i) == 0x9 || *(str + i) == 0x20)
	{
	  if (!q)
	    {
	      q = 1;
	      ret[j++] = *(str + i);
	    }
	}

      else
	{
	  q = 0;
	  ret[j++] = *(str + i);
	}
    }

  ret[j] = 0;

  return ret;
}

__nxml_string_t *
__nxml_string_new (void)
{
  __nxml_string_t *st;

  if (!(st = (__nxml_string_t *) calloc (1, sizeof (__nxml_string_t))))
    return NULL;

  return st;
}

char *
__nxml_string_free (__nxml_string_t * st)
{
  char *ret;

  if (!st)
    return NULL;

  ret = st->string;
  free (st);

  return ret;
}

int
__nxml_string_add (__nxml_string_t * st, char *what, size_t size)
{
  if (!st || !*what)
    return 1;

  if (size <= 0)
    size = strlen (what);

  if (!st->size)
    {
      if (!(st->string = (char *) malloc (sizeof (char) * (size + 1))))
	return 1;
    }
  else
    {
      if (!
	  (st->string =
	   (char *) realloc (st->string,
			     sizeof (char) * (size + st->size + 1))))
	return 1;
    }

  memcpy (st->string + st->size, what, size);
  st->size += size;
  st->string[st->size] = 0;

  return 0;
}

/* EOF */
