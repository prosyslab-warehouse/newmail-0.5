/*
    Copyright (c) 2004,7  Joey Schulze <joey@infodrom.org>

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
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <errno.h>
#ifdef linux
#include <paths.h>
#else
#define _PATH_MAILDIR "/var/spool/mail"
#endif
#include "mbox.h"
#include "optdefs.h"

#define FOLDERS_DELTA	5

struct folder {
  char *path;
  char *prefix;
  off_t size;
};

struct folder **folders = NULL;
unsigned int numfolders = 0;
unsigned int maxfolders = 0;

void add_folder(char *path)
{
  struct folder *thisfolder;
  struct stat st;
  char *cp;
  int sh;

  if (numfolders == maxfolders) {
    if ((folders = (struct folder **)
	 realloc(folders,
		 (maxfolders + FOLDERS_DELTA) * sizeof(struct folder *))) == NULL) {
      perror("realloc");
      exit(1);
    }
    maxfolders += FOLDERS_DELTA;
  }

  if ((thisfolder = (struct folder *)malloc(sizeof(struct folder))) == NULL) {
    perror ("malloc");
    exit (1);
  }

  if ((cp = index(path, '=')) != NULL) {
    *cp++ = '\0';
    if ((thisfolder->prefix = (char *)strdup(cp)) == NULL) {
      perror ("strdup");
      exit (1);
    }
    strcpy(thisfolder->prefix, cp);
  } else {
    if (numfolders > 0) {
      /* More than one mailbox to monitor --> need prefix anyway */
      if ((cp = rindex(path, '/')) != NULL) {
	cp++;
	if ((thisfolder->prefix = (char *)strdup(cp)) == NULL) {
	  perror ("strdup");
	  exit (1);
	}
      } else {
	if ((thisfolder->prefix = (char *)strdup(path)) == NULL) {
	  perror ("strdup");
	  exit (1);
	}
      }
    } else
      thisfolder->prefix = NULL;
  }

  if ((thisfolder->path = (char *)strdup(path)) == NULL) {
    perror ("strdup");
    exit (1);
  }

  sh = stat(path, &st);

  if (sh == 0) {
    thisfolder->size = st.st_size;
  } else if (sh == -1 && errno == ENOENT) {
    thisfolder->size = 0;
  } else {
    free (thisfolder->path);
    free (thisfolder);
    return;
  }

  folders[numfolders++] = thisfolder;
}

void add_default_folder()
{
  char path[128];
  char *env;
  uid_t uid;
  struct passwd *pw;

  if ((env = getenv("MAIL")) == NULL) {
    uid = getuid();
    if ((pw = getpwuid(uid)) == NULL) {
      perror ("getpwuid");
      exit (1);
    }

    snprintf (path, sizeof(path), "%s/%s", _PATH_MAILDIR, pw->pw_name);
  } else {
    snprintf (path, sizeof(path), env);
  }

  add_folder(path);
}

void fix_prefix()
{
  char *cp;

  if (numfolders < 2)
    return;

  if (folders[0]->prefix != NULL)
    return;

  if ((cp = rindex(folders[0]->path, '/')) != NULL) {
    cp++;
    if ((folders[0]->prefix = (char *)strdup(cp)) == NULL) {
      perror ("strdup");
      exit (1);
    }
  } else {
    if ((folders[0]->prefix = (char *)strdup(folders[0]->path)) == NULL) {
      perror ("strdup");
      exit (1);
    }
  }
}

void watch_folders(int opt_flags)
{
  unsigned int i;
  int newmail = 0;

  for (i=0; i < numfolders; i++)
    newmail |= watch_mbox(folders[i]->path, folders[i]->prefix, &folders[i]->size, opt_flags);

  if (newmail) {
    if (!(opt_flags & OPT_WINDOW)) {
      if (opt_flags & OPT_BELL)
	putchar ('\007');
      putchar ('\n');
    }
    fflush (stdout);
  }
}
