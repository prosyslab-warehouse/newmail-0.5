/*
    Copyright (c) 2004,8  Joey Schulze <joey@infodrom.org>

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

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utime.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include "output.h"

/* #define FROM_DETECTION */

#define HDR_LEN	1024

struct mail {
  char from[HDR_LEN];
  char subject[HDR_LEN];
  int priority;
  off_t size;
};

#define TAB	0x09
#define LWSP	0x20

/*
 * Like strncpy() but with terminated result.
 */
char *stringcopy(char *dest, const char *src, size_t n)
{
  strncpy (dest, src, n-1);
  dest[n-1] = '\0';
  return dest;
}

/*
 * Strips balanced quotes in the middle of the realname
 */
char *strip_quotes(char *name)
{
  static char realname[HDR_LEN];
  char *cpl, *cpr;
  char *cp, *xp;

  if ((cpl = index(name, '"')) != NULL && (cpr = index(cpl+1, '"')) != NULL) {
    for (cp=name,xp=realname; *cp && xp < realname+sizeof(realname)-1; cp++) {
      if (cp != cpl && cp != cpr)
	*xp++ = *cp;
    }
    *xp = '\0';
    return realname;
  } else
    return name;
}

/*
 * Extract the realname from a mail address
 *
 * From: "Log Kristian Koehntopp" <joey@infodrom.org>
 * From: "Jérôme" ATHIAS <jerome@athias.fr>
 * From: frank@kuesterei.ch (=?iso-8859-1?q?Frank_K=FCster?=)
 * From: root@luonnotar.infodrom.org (Cron Daemon)
 * From: <JoMaBusch@web.de>
 * From: root@luonnotar.infodrom.org
*/
char *realname(char *from)
{
  static char name[HDR_LEN];
  char *cpl, *cpr;

  name[0] = '\0';

  /* From: REALNAME <login@host.domain> */
  if ((cpr = index(from, '<')) != NULL && index(from, '>') != NULL) {
    if (cpr > from) cpr--;

    /* Strip trailing spaces */
    while (cpr > from && isspace(*cpr)) cpr--;

    /* Strip leading spaces */
    cpl=from;while (*cpl && isspace(*cpl)) cpl++;

    /* Strip balanced surrounding quotes */
    if (*cpl == '"' && *cpr == '"') { cpl++;cpr--; }

    if (cpr > cpl) {
      stringcopy (name, cpl,
		  sizeof(name) < cpr-cpl+2?sizeof(name):cpr-cpl+2);

      if (index(name, '"') != NULL)
	stringcopy (name, strip_quotes(name), sizeof(name));
    } else {
      /* Apparently no realname included */
      cpl = index(from, '<');
      cpr = index(from, '>');
      stringcopy (name, cpl+1,
		  sizeof(name) < cpr-cpl?sizeof(name):cpr-cpl);
    }

  /* From: login@host.domain (REALNAME) */
  } else if ((cpl = index(from, '(')) != NULL && (cpr = index(from, ')')) != NULL) {
    stringcopy (name, cpl+1,
		sizeof(name) < cpr-cpl?sizeof(name):cpr-cpl);

  /* From: login@host.domain */
  } else {
    /* Strip leading spaces */
    cpl=from;while (*cpl && isspace(*cpl)) cpl++;
    for (cpr=cpl; *cpr && !isspace(*cpr); cpr++);
    stringcopy (name, cpl,
		sizeof(name) < cpr-cpl+1?sizeof(name):cpr-cpl+1);
  }

  return name;
}

/*
 * Tries to extract useful content from the From_ line
 */
char *reduce_from_(char *from_)
{
  static char name[HDR_LEN];
  char *cpl, *cpr;

  for (cpl=from_; *cpl &&  isspace(*cpl); cpl++);
  for (cpr=cpl;   *cpr && !isspace(*cpr); cpr++);

  if (cpr > cpl)
    stringcopy (name, cpl, 
	        sizeof(name) < cpr-cpl+1?sizeof(name):cpr-cpl+1);
  return name;
}

int inspect_mbox(char *path, char *prefix, off_t size, int opt_flags)
{
  FILE *f;
  char buf[HDR_LEN];
  char tmp[512];
  char *cp;
  int inheader = 1;
  int readnewline = 0;
  int newmail = 0;
  int lookahead;

  char from_[HDR_LEN] = "";
  char from[HDR_LEN] = "";
  char realfrom[HDR_LEN] = "";
#ifdef FROM_DETECTION
  char to[HDR_LEN] = "";
#endif
  char subject[HDR_LEN] = "";
  int priority = 0;

  if ((f = fopen(path, "r")) == NULL)
    return 0;

  if (size > 0 && fseek(f, size, SEEK_SET) != 0)
    return 0;

  while (!feof(f)) {
    if ((cp = fgets(buf, sizeof(buf), f)) == NULL)
      continue;
    if (strlen(buf) > 0 && buf[strlen(buf)-1] == '\n') {
      buf[strlen(buf)-1] = '\0';
      if (strlen(buf) > 0 && buf[strlen(buf)-1] == '\r')
	buf[strlen(buf)-1] = '\0';

      if (inheader && !feof(f)) {
	while ((lookahead=fgetc(f)) == TAB || lookahead == LWSP) {
	  if (buf[strlen(buf)-1] != LWSP)
	    strncat (buf, " ", sizeof(buf)-strlen(buf)-1);
	  if ((cp = fgets(tmp, sizeof(tmp), f)) != NULL) {
	    strncat (buf, tmp, sizeof(buf)-strlen(buf)-1);
	    if (strlen(buf) > 0 && buf[strlen(buf)-1] == '\n')
	      buf[strlen(buf)-1] = '\0';
	    if (strlen(buf) > 0 && buf[strlen(buf)-1] == '\r')
	      buf[strlen(buf)-1] = '\0';

	    if (strlen(tmp) > 0 && buf[strlen(tmp)-1] == '\n') {
	      /* Read the remainder */
	      while (!feof(f) && fgets(tmp, sizeof(tmp), f) != NULL) {
		if (strlen(tmp) > 0 && tmp[strlen(tmp)-1] == '\n')
		  break;
	      }
	    }
	  }
	}
	/* Rewind by one character for next read */
	if (lookahead != EOF)
	  fseek(f, -1, SEEK_CUR);
      }
    } else {
      /* Read the remainder */
      while (!feof(f) && fgets(tmp, sizeof(tmp), f) != NULL) {
	if (strlen(tmp) > 0 && tmp[strlen(tmp)-1] == '\n')
	  break;
      }
    }

    if (inheader) {
      if (strlen(buf) == 0) {
	inheader = 0;

	if (strlen(from))
	  stringcopy (realfrom, realname(from), sizeof(realfrom));
	else if (strlen(from_))
	  stringcopy (realfrom, reduce_from_(from_), sizeof(realfrom));
	else
	  realfrom[0] = '\0';

	if (realfrom[0] != '\0' || subject[0] != '\0') {
	  emit (prefix, realfrom, subject, priority, opt_flags);
	  newmail = 1;
	}

#ifdef FROM_DETECTION
	from_[0] = from[0] = to[0] = subject[0] = '\0';
#else
	from_[0] = from[0] = subject[0] = '\0';
#endif
	priority = 0;
      } else {
	if (strncmp(buf, "From ", 5) == 0)
	  stringcopy (from_, buf+5, sizeof(from_));
	else if (strncasecmp(buf, "From: ", 6) == 0)
	  stringcopy (from, buf+6, sizeof(from));
#ifdef FROM_DETECTION
	else if (strncasecmp(buf, "To: ", 4) == 0)
	  stringcopy (to, buf+4, sizeof(to));
#endif
	else if (strncasecmp(buf, "Subject: ", 9) == 0)
	  stringcopy (subject, buf+9, sizeof(subject));
	else if (strncasecmp(buf, "Priority: urgent", 16) == 0 && buf[16] == '\0')
	  priority = 1;
	else if (strncasecmp(buf, "X-Priority: 1", 13) == 0 && buf[13] == '\0')
	  priority = 1;
      }
    } else if (strlen(buf) == 0) {
      readnewline = 1;
    } else if (readnewline && strncasecmp(buf, "From ", 5) == 0) {
      inheader = 1;
      readnewline = 0;
      stringcopy (from_, buf+5, sizeof(from_));
    } else
      readnewline = 0;
  }

  fclose(f);

  return newmail;
}

int watch_mbox(char *path, char *prefix, off_t *size, int opt_flags)
{
  struct stat st;
  struct utimbuf timbuf;
  int newmail = 0;

  if (stat(path, &st) == 0) {
    if (st.st_size > *size)
      if (access(path, R_OK) == 0) {
	timbuf.actime = st.st_atime;
	timbuf.modtime = st.st_mtime;

	newmail = inspect_mbox(path, prefix, *size, opt_flags);

	utime(path, &timbuf);
      }
    *size = st.st_size;
    return newmail;
  } else {
    *size = 0;
  }

  return 0;
}
