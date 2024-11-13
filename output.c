/*
    Copyright (c) 2004  Joey Schulze <joey@infodrom.org>

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

/*
    From Elm 2.4pl25ME+43:

    The output format is either:

          newmail:
             >> New mail from <user> - <subject>
             >> Priority mail from <user> - <subject>

             >> <folder>: from <user> - <subject>
             >> <folder>: Priority from <user> - <subject>

          newmail -w:
             <user> - <subject>
             Priority: <user> - <subject>

             <folder>: <user> - <subject>
             <folder>: Priority: <user> - <subject>

 */

#include <stdio.h>
#include "rfc2047.h"
#include "optdefs.h"

#define MAIL_FROM	"Mail from"
#define PRIO_FROM	"Priority mail from"
#define PRIO_WIN	"Priority"

void emit(char *prefix, char *realname, char *subject, int priority, int opt_flags)
{
  if (!(opt_flags & OPT_RAW)) {
    realname = convert_header (realname);
    subject = convert_header (subject);
  }

  if (opt_flags & OPT_WINDOW) {
    if (prefix != NULL)
      printf ("%s: ", prefix);
    if (priority)
      printf ("%s: ", PRIO_WIN);
    printf ("%s - %s\n", realname, subject);
  } else {
    printf ("\r>> ");
    if (prefix != NULL)
      printf ("%s: ", prefix);

      if (priority)
	printf ("%s %s - %s\n", PRIO_FROM, realname, subject);
      else
	printf ("%s %s - %s\n", MAIL_FROM, realname, subject);
  }
}
