/*
    Copyright (c) 2004,6  Joey Schulze <joey@infodrom.org>

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
#include <unistd.h>
#include <signal.h>
#include "charset.h"
#include "folder.h"
#include "optdefs.h"
#include "version.h"

void usage()
{
  printf("%s %s -- %s\n", PROGNAME, VERSION, COPYRIGHT);

  printf("\n%s [options] [mailbox [mailbox [..]]]\n", PROGNAME);
  printf("-b      Ring a bell when a new set of mails arrived\n");
  printf("-i <n>  Interval between checking the mailbox(es)\n");
  printf("-r      Raw output, i.e. no character conversion\n");
  printf("-w      Run as window application in foreground\n");
}

int main(int argc, char *argv[])
{
  int opt_flags = 0;
  unsigned int opt_interval = 60;
  int c;
  int tmp_i;

  while ((c = getopt(argc, argv, "bhi:rw")) != -1) {
    switch (c) {
    case 'b':
      opt_flags |= OPT_BELL;
      break;
    case 'h':
      usage();
      return 0;
    case 'i':
      tmp_i = atoi(optarg);
      if (tmp_i > 0)
	opt_interval = tmp_i;
      else
	fprintf (stderr, "Interval not greater as zero, ignoring\n");
      break;
    case 'r':
      opt_flags |= OPT_RAW;
      break;
    case 'w':
      opt_flags |= OPT_WINDOW;
      break;
    }
  }

  if (optind == argc)
    add_default_folder();
  else
    while (optind < argc)
      add_folder (argv[optind++]);
  fix_prefix();

  if (!(opt_flags & OPT_WINDOW)) {
    if (fork())
      return 0;

    signal (SIGINT, SIG_IGN);
    signal (SIGQUIT, SIG_IGN);
  }

  set_charset();

  while (1) {
    /* Loop over all files */
    watch_folders (opt_flags);

    /* wait for the next incarnation */
    sleep (opt_interval);

    /* Terminate if we lost out tty */
    if (!isatty(1))
      return 1;
  }

  /* Not reached */
  return 0;
}
