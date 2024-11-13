/*
    Copyright (c) 2006,8  Joey Schulze <joey@infodrom.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <locale.h>
#include <langinfo.h>
#include <string.h>
#include <iconv.h>
#include <errno.h>

char *charset = NULL;

/*
 * Determine the output character set
 */
void set_charset()
{
  setlocale (LC_CTYPE, "");

  charset = strdup (nl_langinfo(CODESET));
}

/*
 * Convert a word from an arbitrary charset into the output character set
 *
 * No conversion is performed when both charsets are equal
 */
char *convert_word(const char *encoding, char *inbuf, char *outbuf, size_t outbytesleft)
{
  iconv_t cd;
  char *inptr, *outptr;
  size_t inbytesleft;
  size_t nconv;
  size_t outsize;

  if (!charset || !strcasecmp (encoding, charset)) {
    memmove (outbuf, inbuf, strlen(inbuf)<outbytesleft?strlen(inbuf)+1:outbytesleft);
    outbuf[outbytesleft-1] = '\0';
    return outbuf;
  }

  outsize = outbytesleft;

  cd = iconv_open (charset, encoding);

  inbytesleft = strlen (inbuf)+1;
  inptr = inbuf;
  outptr = outbuf;

  while (1) {
    nconv = iconv (cd, &inptr, &inbytesleft, &outptr, &outbytesleft);

    if (nconv != -1)
      break;

    if (errno == EILSEQ && outsize-outbytesleft >= 0 && outbytesleft > 1) {
      outbuf[outsize-outbytesleft] = '?';
      outbuf[outsize-outbytesleft+1] = '\0';
      outbytesleft--;
      inbytesleft--;
      outptr++;
      inptr++;
    } else
      break;
  }

  iconv_close(cd);

  if (nconv == -1 && outsize-outbytesleft >= 0)
    outbuf[outsize-outbytesleft] = '\0';

  return outbuf;
}


/*
 * Needs to be called with LANG=de_DE.ISO-8859-1

void test_charset()
{
  char outbuf[100];
  size_t size = 99;

  memset (outbuf, 0, sizeof (outbuf));
  printf ("%s\n", convert_word ("UTF-8", "fÃ¼r ein", outbuf, size));
  printf ("%s\n", outbuf);
  if (!strcmp(outbuf, "für ein"))
    printf ("charset.c: test passed\n");
  else
    printf ("charset.c: test failed\n");
}

*/
