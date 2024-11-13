#   Copyright (c) 2004,6  Joey Schulze <joey@infodrom.org>
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

prefix = /usr

SOURCE = $(wildcard *.c)
OBJS = $(SOURCE:%.c=%.o)

# DEFINES = -DDEBUG

CFLAGS = -O2 -Wall $(DEFINES)
# CFLAGS = -g -Wall $(DEFINES)

newmail: $(OBJS)

clean:
	rm -f newmail $(OBJS) *~

install: newmail
	install -s -o root -g root -m 755 newmail   $(prefix)/bin
	install    -o root -g root -m 644 newmail.1 $(prefix)/share/man/man1
