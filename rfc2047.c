/*
    Copyright (c) 2006-8  Joey Schulze <joey@infodrom.org>

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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "charset.h"

/*
 * Index_hex and Index_64 imported from Mutt:handler.c
 * decode_quotedprintable() and decode_base64() as well
 */
int Index_hex[128] = {
  -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
  -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
  -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
  0, 1, 2, 3,  4, 5, 6, 7,  8, 9,-1,-1, -1,-1,-1,-1,
  -1,10,11,12, 13,14,15,-1, -1,-1,-1,-1, -1,-1,-1,-1,
  -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
  -1,10,11,12, 13,14,15,-1, -1,-1,-1,-1, -1,-1,-1,-1,
  -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1
};

int Index_64[128] = {
  -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
  -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
  -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,62, -1,-1,-1,63,
  52,53,54,55, 56,57,58,59, 60,61,-1,-1, -1,-1,-1,-1,
  -1, 0, 1, 2,  3, 4, 5, 6,  7, 8, 9,10, 11,12,13,14,
  15,16,17,18, 19,20,21,22, 23,24,25,-1, -1,-1,-1,-1,
  -1,26,27,28, 29,30,31,32, 33,34,35,36, 37,38,39,40,
  41,42,43,44, 45,46,47,48, 49,50,51,-1, -1,-1,-1,-1
};
#define hexval(c) Index_hex[(unsigned int)(c)]
#define base64val(c) Index_64[(unsigned int)(c)]


/*
 * Decode a string from quoted printable
 */
char *decode_quotedprintable(char *buf)
{
  char *inp, *outp;
  char *endp;

  endp = buf+strlen(buf);
  
  for (inp = outp = buf; inp < endp; inp++) {
    if (*inp == '_')
      *(outp++) = ' ';
    else if (*inp == '=' && inp+2 < endp &&
	     (!(*(inp+1) & ~127) && hexval(*(inp+1)) != -1) &&
	     (!(*(inp+2) & ~127) && hexval(*(inp+2)) != -1)) {
      *(outp++) = (hexval(*(inp+1)) << 4) | hexval(*(inp+2));
      inp += 2;
    } else
      *(outp++) = *inp;
  }
  *outp = '\0';

  return buf;
}

/*
 * Decode a string from base43
 */
char *decode_base64(char *buf)
{
  char *inp, *outp;
  char *endp;
  int c, b = 0, k = 0;

  endp = buf+strlen(buf);

  for (inp = outp = buf; inp < endp; inp++) {
    if (*inp == '=')
      break;
    if ((*inp & ~127) || (c = base64val(*inp)) == -1)
      continue;
    if (k + 6 >= 8)
      {
	k -= 2;
	*(outp++) = b | (c >> k);
	b = c << (8 - k);
      }
    else
      {
	b |= c << (k + 2);
	k += 6;
      }
    *outp = '\0';
  }

  return buf;
}

/*
 * converts a header line into the current character set if required
 */
char *convert_header(char *buf)
{
  char *charset;
  int encoding;
  char *inp, *outp, *encstart, *wordp, *encp, *endp;
  char *decode;
  char *output;
  size_t size;
  size_t outlen;

  endp = buf+strlen(buf);
  inp = buf;

  outlen = strlen(buf)+1;
  if ((output = (char *)malloc(outlen)) == NULL) {
    perror ("malloc");
    return buf;
  }

  memset(output, 0, outlen);
  outp = output;

  while ((encstart = strstr (inp, "=?"))) {
    if (encstart != inp) {
      // -1 nur falls vorher kein encoded-word
      memcpy (outp, inp, encstart-inp);
      outp += encstart-inp;
    }
    charset = encstart+2;

    if ((encp = strchr (charset, '?')) == NULL) {
      memcpy (outp, inp, strlen(inp)+1);
      break;
    }

    *encp = '\0';

    if ((encp + 3 >= endp) || !strchr ("BbQq", *encp) || (*(encp+2) != '?')) {
      *encp = '?';
      memcpy (outp, inp, strlen(inp)+1);
      outp += strlen(inp)+1;
      inp += strlen(inp)+1;
      break;
    }

    encoding = toupper(*(encp+1));

    inp = encp + 3;

    if ((wordp = strstr (inp, "?=")) == NULL) {
      memcpy (outp, inp, strlen(inp)+1);
      outp += strlen(inp)+1;
      inp += strlen(inp)+1;
      break;
    }

    *wordp = '\0';
    wordp += 2;

    /* Look for next =?, spaces will be eaten between two encoded-words */
    if (*wordp && *wordp == ' ' &&
	*(wordp+1) && *(wordp+1) == '=' &&
	*(wordp+2) && *(wordp+2) == '?')
      wordp++;

    switch (encoding) {
    case 'B':
      decode = decode_base64 (inp);
      break;
    case 'Q':
      decode = decode_quotedprintable (inp);
      break;
    default:
      decode = inp;
      break;
    }

    size = outlen - strlen(output);
    decode = convert_word (charset, decode, outp, size);

    outp += strlen(decode);

    inp = wordp;
  }

  if (strlen(inp))
    memcpy (outp, inp, strlen(inp)+1);

  memcpy (buf, output, strlen(output)+1);
  free (output);

  return buf;
}


/*
 * Needs to be called with LANG=de_DE.ISO-8859-1

#include <stdio.h>

void test_rfc2047()
{
  char *qp;
  char *b64;
  char *subject;

  set_charset();

  qp = strdup ("f=FCr");
  printf ("\t%s -> ", qp);
  b64 = decode_quotedprintable (qp);
  printf ("%s\n", qp);
  if (!strcmp (qp, "für"))
    printf ("rfc2047.c: decode_quotedprintable() passed\n");
  else
    printf ("rfc2047.c: decode_quotedprintable() failed\n");
  free (qp);

  qp = strdup ("f=FCr_Umlaute_=EFm_Sub");
  printf ("\t%s -> ", qp);
  b64 = decode_quotedprintable (qp);
  printf ("%s\n", qp);
  if (!strcmp (qp, "für Umlaute ïm Sub"))
    printf ("rfc2047.c: decode_quotedprintable() passed\n");
  else
    printf ("rfc2047.c: decode_quotedprintable() failed\n");
  free (qp);

  b64 = strdup ("ZvxyINxtbOT8dOs=");
  printf ("\t%s -> ", b64);
  b64 = decode_base64 (b64);
  printf ("%s\n", b64);
  if (!strcmp (b64, "für Ümläütë"))
    printf ("rfc2047.c: decode_base64() passed\n");
  else
    printf ("rfc2047.c: decode_base64() failed\n");
  free (b64);

  subject = strdup ("Vorschlag =?ISO-8859-1?Q?f=FCr?= ein Eintrittskonzept");
  printf ("\t%s\n", subject);
  subject = convert_header (subject);
  printf ("\t%s\n", subject);
  if (!strcmp (subject, "Vorschlag für ein Eintrittskonzept"))
    printf ("rfc2047.c: convert_header() passed\n");
  else
    printf ("rfc2047.c: convert_header() failed\n");
  free (subject);

  subject = strdup ("=?utf-8?q?Google=3A_Chat_f=C3=BCr_die_Ewigkeit?=");
  printf ("\t%s\n", subject);
  subject = convert_header (subject);
  printf ("\t%s\n", subject);
  if (!strcmp (subject, "Google: Chat für die Ewigkeit"))
    printf ("rfc2047.c: convert_header() passed\n");
  else
    printf ("rfc2047.c: convert_header() failed\n");
  free (subject);

  subject = strdup ("=?iso-8859-1?B?ZvxyINxtbOT8dOs=?= testen");
  printf ("\t%s\n", subject);
  subject = convert_header (subject);
  printf ("\t%s\n", subject);
  if (!strcmp (subject, "für Ümläütë testen"))
    printf ("rfc2047.c: convert_header() passed\n");
  else
    printf ("rfc2047.c: convert_header() failed\n");
  free (subject);

}
*/
