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

/*
 * UTF-8
 * 7bits:	0xxxxxxx
 * 11bits:	110xxxxx 10xxxxxx
 * 16bits:	1110xxxx 10xxxxxx 10xxxxxx
 * 21bits:	11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
 * 26bits:	111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
 * 31bits:	1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
 */

int64_t
__nxml_utf8 (unsigned char **buffer, size_t * size, int *byte)
{
  int64_t ret;
  unsigned char c0 = **buffer, c1, c2, c3, c4;

  if (c0 < 0x80 || *size < 2)
    {
      *byte = 1;
      ret = (int64_t) c0;
      return ret;
    }

  c1 = *(*buffer + 1);

  if ((c0 & 0xe0) == 0xc0 || *size < 3)
    {
      *byte = 2;
      ret = (((c0 & 0x1f) << 6) | (c1 & 0x3f));
      return ret;
    }

  c2 = *(*buffer + 2);

  if ((c0 & 0xf0) == 0xe0 || *size < 4)
    {
      *byte = 3;
      ret = (((c0 & 0x0f) << 12) | ((c1 & 0x3f) << 6) | (c2 & 0x3f));
      return ret;
    }

  c3 = *(*buffer + 3);

  if ((c0 & 0xf8) == 0xf0 || *size < 5)
    {
      *byte = 4;
      ret =
	(((c0 & 0x7) << 18) | ((c1 & 0x3f) << 12) | ((c2 & 0x3f) << 6) |
	 (c3 & 0x3f));
      return ret;
    }

  c4 = *(*buffer + 4);

  if ((c0 & 0xfc) == 0xf8)
    {
      *byte = 5;
      ret =
	(((c0 & 0x3) << 24) | ((c1 & 0x3f) << 18) | ((c2 & 0x3f) << 12) |
	 ((c3 & 0x3f) << 6) | (c4 & 0x3f));
      return ret;
    }

  *byte = 1;
  ret = (int64_t) c0;
  return ret;
}

#define __NXML_XTO8( x , b ) \
	    if(byte>=1023-b) { \
		    if(!(ret=realloc(ret, (j+b)*sizeof(char)))) return -1; \
		    byte=0; \
	    } \
            memcpy(&ret[j], x, b); \
	    j+=b; \
	    byte+=b;

static size_t
__nxml_utf16to8 (int le, unsigned char *buffer, size_t size,
		 unsigned char **ret_buffer)
{
  int64_t ch;
  int j = 0;
  int byte = 0;
  unsigned char *ret;

  if (!(ret = (unsigned char *) malloc (sizeof (unsigned char) * 1024)))
    return -1;

  while (size > 0)
    {
      if (le)
	{
	  if ((*buffer & 0xfc) == 0xd8 && (*(buffer + 2) & 0xfc) == 0xdc)
	    {
	      ch = ((*buffer & 0x03) << 18) + (*(buffer + 1) << 10) +
		((*(buffer + 2) & 0x03) << 8) + *(buffer + 3);

	      buffer += 4;
	      size -= 4;
	    }

	  else
	    {

	      ch = (*buffer << 8) + *(buffer + 1);
	      buffer += 2;
	      size -= 2;
	    }
	}

      else if ((*(buffer + 1) & 0xfc) == 0xd8
	       && (*(buffer + 3) & 0xfc) == 0xdc)
	{
	  ch = ((*(buffer + 1) & 0x03) << 18) + (*buffer << 10) +
	    ((*(buffer + 3) & 0x03) << 8) + *(buffer + 2);

	  buffer += 4;
	  size -= 4;
	}

      else
	{

	  ch = (*(buffer + 1) << 8) + *buffer;
	  buffer += 2;
	  size -= 2;
	}

      /* 8bit:  1000000 */
      if (ch < 0x80)
	{
	  __NXML_XTO8 ((void *) &ch, 1);
	}

      /* 11bit:  xx100000 xx000000
       *         1000 0000 0000 
       *      0x 8    0    0
       */
      else if (ch < 0x800)
	{
	  /* 11bits:    110xxxxx 10xxxxxx */
	  char a[2];
	  a[0] = (ch >> 6) | 0xc0;
	  a[1] = (ch & 0x3f) | 0x80;
	  __NXML_XTO8 ((void *) a, 2);
	}

      /* 16bit:  xxx10000 xx000000 xx000000
       *         1 0000 0000 0000 0000
       *      0x 1    0    0    0    0
       */
      else if (ch < 0x10000)
	{
	  /* 16bits:  1110xxxx 10xxxxxx 10xxxxxx */
	  char a[3];
	  a[0] = (ch >> 12) | 0xe0;
	  a[1] = ((ch >> 6) & 0x3f) | 0x80;
	  a[2] = (ch & 0x3f) | 0x80;
	  __NXML_XTO8 ((void *) a, 3);
	}

      /* 21bit:  xxxx1000 xx000000 xx000000 xx000000
       *         10 0000 0000 0000 0000 0000
       *      0x 2  0    0    0    0    0
       */
      else if (ch < 0x200000)
	{
	  /* 21bits:  11110xxx 10xxxxxx 10xxxxxx 10xxxxxx */
	  char a[4];
	  a[0] = (ch >> 18) | 0xf0;
	  a[1] = ((ch >> 12) & 0x3f);
	  a[2] = ((ch >> 6) & 0x3f);
	  a[3] = (ch & 0x3f);
	  __NXML_XTO8 ((void *) a, 4);
	}

      /* 26bit:  xxxxx100 xx000000 xx000000 xx000000 xx000000
       *         100 0000 0000 0000 0000 0000 0000
       *      0x 4   0    0    0    0    0    0
       */
      else if (ch < 0x4000000)
	{
	  /* 21bits:  111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx */
	  char a[5];
	  a[0] = (ch >> 24) | 0xf8;
	  a[1] = ((ch >> 18) & 0x3f);
	  a[2] = ((ch >> 12) & 0x3f);
	  a[3] = ((ch >> 6) & 0x3f);
	  a[4] = (ch & 0x3f);
	  __NXML_XTO8 ((void *) a, 5);
	}
    }

  ret[j] = 0;
  (*ret_buffer) = ret;

  return (size_t) j;
}

static size_t
__nxml_ucs4to8 (int type, unsigned char *buffer, size_t size,
		unsigned char **ret_buffer)
{
  int64_t ch;
  int j = 0;
  int byte = 0;
  unsigned char *ret;

  if (!(ret = (unsigned char *) malloc (sizeof (unsigned char) * 1024)))
    return -1;

  while (size > 0)
    {
      switch (type)
	{
	case 0:		/* 1234 */
	  ch =
	    (*buffer << 18) + (*(buffer + 1) << 12) + (*(buffer + 2) << 6) +
	    (*(buffer + 3));
	  break;

	case 1:		/* 4321 */
	  ch =
	    (*buffer) + (*(buffer + 1) << 6) + (*(buffer + 2) << 12) +
	    (*(buffer + 3) << 18);
	  break;

	case 2:		/* 2143 */
	  ch =
	    ((*buffer) << 12) + (*(buffer + 1) << 18) + (*(buffer + 2)) +
	    (*(buffer + 3) << 6);
	  break;

	case 3:		/* 3412 */
	  ch =
	    ((*buffer) << 6) + (*(buffer + 1)) + (*(buffer + 2) << 18) +
	    (*(buffer + 3) << 12);
	  break;

	}

      buffer += 4;
      size -= 4;

      /* 8bit:  1000000 */
      if (ch < 0x80)
	{
	  __NXML_XTO8 ((void *) &ch, 1);
	}

      /* 11bit:  xx100000 xx000000
       *         1000 0000 0000 
       *      0x 8    0    0
       */
      else if (ch < 0x800)
	{
	  /* 11bits:    110xxxxx 10xxxxxx */
	  char a[2];
	  a[0] = (ch >> 6) | 0xc0;
	  a[1] = (ch & 0x3f) | 0x80;
	  __NXML_XTO8 ((void *) a, 2);
	}

      /* 16bit:  xxx10000 xx000000 xx000000
       *         1 0000 0000 0000 0000
       *      0x 1    0    0    0    0
       */
      else if (ch < 0x10000)
	{
	  /* 16bits:  1110xxxx 10xxxxxx 10xxxxxx */
	  char a[3];
	  a[0] = (ch >> 12) | 0xe0;
	  a[1] = ((ch >> 6) & 0x3f) | 0x80;
	  a[2] = (ch & 0x3f) | 0x80;
	  __NXML_XTO8 ((void *) a, 3);
	}

      /* 21bit:  xxxx1000 xx000000 xx000000 xx000000
       *         10 0000 0000 0000 0000 0000
       *      0x 2  0    0    0    0    0
       */
      else if (ch < 0x200000)
	{
	  /* 21bits:  11110xxx 10xxxxxx 10xxxxxx 10xxxxxx */
	  char a[4];
	  a[0] = (ch >> 18) | 0xf0;
	  a[1] = ((ch >> 12) & 0x3f);
	  a[2] = ((ch >> 6) & 0x3f);
	  a[3] = (ch & 0x3f);
	  __NXML_XTO8 ((void *) a, 4);
	}

      /* 26bit:  xxxxx100 xx000000 xx000000 xx000000 xx000000
       *         100 0000 0000 0000 0000 0000 0000
       *      0x 4   0    0    0    0    0    0
       */
      else if (ch < 0x4000000)
	{
	  /* 21bits:  111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx */
	  char a[5];
	  a[0] = (ch >> 24) | 0xf8;
	  a[1] = ((ch >> 18) & 0x3f);
	  a[2] = ((ch >> 12) & 0x3f);
	  a[3] = ((ch >> 6) & 0x3f);
	  a[4] = (ch & 0x3f);
	  __NXML_XTO8 ((void *) a, 5);
	}
    }

  ret[j] = 0;
  (*ret_buffer) = ret;

  return (size_t) j;
}

int
__nxml_utf_detection (const char *r_buffer, size_t r_size, char **buffer,
		      size_t * size, nxml_charset_t * charset)
{
  /* Utf-8: 0x3c 0x3f 0x78 0x6d */
  if (strncmp (r_buffer, "<?xml", 5))
    {
      *charset = NXML_CHARSET_UTF8;
      return 0;
    }

  /* Utf-16LE: 0x00 0x3c 0x00 0x3f */
  if (*r_buffer == 0x00 && *(r_buffer + 1) == 0x3c && *(r_buffer + 2) == 0x00
      && *(r_buffer + 3) == 0x3f)
    {
      (*size) =
	__nxml_utf16to8 (1, (unsigned char *) r_buffer, r_size,
			 (unsigned char **) buffer);

      *charset = NXML_CHARSET_UTF16LE;

      return 1;
    }

  /* Utf-16BE: 0x3c 0x00 0x3f 0x00 */
  if (*r_buffer == 0x3c && *(r_buffer + 1) == 0x00 && *(r_buffer + 2) == 0x3f
      && *(r_buffer + 3) == 0x00)
    {
      (*size) =
	__nxml_utf16to8 (0, (unsigned char *) r_buffer, r_size,
			 (unsigned char **) buffer);

      *charset = NXML_CHARSET_UTF16BE;

      return 1;
    }

  /* UCS-4 (1234): 0x00 0x00 0x00 0x3c */
  if (*r_buffer == 0x00 && *(r_buffer + 1) == 0x00 && *(r_buffer + 2) == 0x00
      && *(r_buffer + 3) == 0x3c)
    {
      (*size) =
	__nxml_ucs4to8 (0, (unsigned char *) r_buffer, r_size,
			(unsigned char **) buffer);

      *charset = NXML_CHARSET_UCS4_1234;

      return 1;
    }

  /* UCS-4 (4321): 0x3c 0x00 0x00 0x00 */
  if (*r_buffer == 0x3c && *(r_buffer + 1) == 0x00 && *(r_buffer + 2) == 0x00
      && *(r_buffer + 3) == 0x00)
    {
      (*size) =
	__nxml_ucs4to8 (1, (unsigned char *) r_buffer, r_size,
			(unsigned char **) buffer);

      *charset = NXML_CHARSET_UCS4_4321;

      return 1;
    }

  /* UCS-4 (2143): 0x00 0x00 0x3c 0x00 */
  if (*r_buffer == 0x00 && *(r_buffer + 1) == 0x00 && *(r_buffer + 2) == 0x3c
      && *(r_buffer + 3) == 0x00)
    {
      (*size) =
	__nxml_ucs4to8 (2, (unsigned char *) r_buffer, r_size,
			(unsigned char **) buffer);

      *charset = NXML_CHARSET_UCS4_2143;

      return 1;
    }

  /* UCS-4 (3412): 0x00 0x3c 0x00 0x00 */
  if (*r_buffer == 0x00 && *(r_buffer + 1) == 0x3c && *(r_buffer + 2) == 0x00
      && *(r_buffer + 3) == 0x00)
    {
      (*size) =
	__nxml_ucs4to8 (3, (unsigned char *) r_buffer, r_size,
			(unsigned char **) buffer);

      *charset = NXML_CHARSET_UCS4_3412;

      return 1;
    }

  *charset = NXML_CHARSET_UNKNOWN;

  return 0;
}

int64_t
__nxml_int_charset (int ch, unsigned char *str, char *charset)
{
  if (!charset || strcasecmp (charset, "utf-8"))
    {
      str[0] = ch;
      return 1;
    }

  /* 8bit:  1000000 */
  if (ch < 0x80)
    {
      str[0] = ch;
      return 1;
    }

  /* 11bit:  xx100000 xx000000
   *         1000 0000 0000 
   *      0x 8    0    0
   */
  else if (ch < 0x800)
    {
      /* 11bits:    110xxxxx 10xxxxxx */
      str[0] = (ch >> 6) | 0xc0;
      str[1] = (ch & 0x3f) | 0x80;
      return 2;
    }

  /* 16bit:  xxx10000 xx000000 xx000000
   *         1 0000 0000 0000 0000
   *      0x 1    0    0    0    0
   */
  else if (ch < 0x10000)
    {
      /* 16bits:  1110xxxx 10xxxxxx 10xxxxxx */
      str[0] = (ch >> 12) | 0xe0;
      str[1] = ((ch >> 6) & 0x3f) | 0x80;
      str[2] = (ch & 0x3f) | 0x80;
      return 3;
    }

  /* 21bit:  xxxx1000 xx000000 xx000000 xx000000
   *         10 0000 0000 0000 0000 0000
   *      0x 2  0    0    0    0    0
   */
  else if (ch < 0x200000)
    {
      /* 21bits:  11110xxx 10xxxxxx 10xxxxxx 10xxxxxx */
      str[0] = (ch >> 18) | 0xf0;
      str[1] = ((ch >> 12) & 0x3f);
      str[2] = ((ch >> 6) & 0x3f);
      str[3] = (ch & 0x3f);
      return 4;
    }

  /* 26bit:  xxxxx100 xx000000 xx000000 xx000000 xx000000
   *         100 0000 0000 0000 0000 0000 0000
   *      0x 4   0    0    0    0    0    0
   */
  else if (ch < 0x4000000)
    {
      /* 21bits:  111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx */
      str[0] = (ch >> 24) | 0xf8;
      str[1] = ((ch >> 18) & 0x3f);
      str[2] = ((ch >> 12) & 0x3f);
      str[3] = ((ch >> 6) & 0x3f);
      str[4] = (ch & 0x3f);
      return 5;
    }

  return 0;
}

/* EOF */
