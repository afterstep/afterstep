/*
 * Copyright (C) 1999 Sasha Vasko <sasha at aftercode.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <signal.h>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/time.h>

#include "../configure.h"
#include "../include/asapp.h"

#include "../include/afterstep.h"
#include "../include/mystyle.h"
#include "../include/screen.h"
#include "../include/module.h"
#include "../include/wmprops.h"
#include "../libAfterImage/afterimage.h"

void
InitAtoms (Display * dpy, ASAtom * atoms)
{
	int           i;

	for (i = 0; atoms[i].name != NULL; i++)
		atoms[i].atom = XInternAtom (dpy, atoms[i].name, False);
}

void          DeadPipe (int nonsense);

int
My_XNextEvent (Display * dpy, int x_fd, int as_fd, void (*as_msg_handler) (unsigned long type, unsigned long *body),
			   XEvent * event)
{
	fd_set        in_fdset;
	ASMessage     msg;
	static int    miss_counter = 0;
	struct timeval tv;
	struct timeval *t = NULL;

	if (as_msg_handler == NULL || as_fd < 0 || x_fd < 0)
		return 0;

	if (XPending (dpy))
	{
		XNextEvent (dpy, event);
		miss_counter = 0;
		return 1;
	} else
	{
		FD_ZERO (&in_fdset);
		FD_SET (x_fd, &in_fdset);
		FD_SET (as_fd, &in_fdset);
		if (timer_delay_till_next_alarm ((time_t *) & tv.tv_sec, (time_t *) & tv.tv_usec))
			t = &tv;
#ifdef __hpux
		while (select (MAX (x_fd, as_fd) + 1, (int *)&in_fdset, 0, 0, t) == -1)
			if (errno != EINTR)
				break;
#else
		while (select (MAX (x_fd, as_fd) + 1, &in_fdset, 0, 0, t) == -1)
			if (errno != EINTR)
				break;
#endif
		if (FD_ISSET (x_fd, &in_fdset))
		{
			if (XPending (dpy))
			{
				XNextEvent (dpy, event);
				miss_counter = 0;
				return 1;
			}
			if (++miss_counter > 100)
				DeadPipe (0);
		}
		if (FD_ISSET (as_fd, &in_fdset))
			if (ReadASPacket (as_fd, msg.header, &(msg.body)) > 0)
			{
				as_msg_handler (msg.header[1], msg.body);
				free (msg.body);
			}
	}
	return 0;
}


