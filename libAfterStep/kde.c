/****************************************************************************
 *
 * Copyright (c) 2005 Sasha Vasko <sasha at aftercode.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.   See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 ****************************************************************************/

#define LOCAL_DEBUG
#include "../configure.h"

#include "asapp.h"
#include "afterstep.h"
#include "wmprops.h"
#include "desktop_category.h"
#include "kde.h"

/* KDE IPC implementation : */

void 
KIPC_sendMessage(KIPC_Message msg, Window w, int data)
{
    XEvent ev;
    ev.xclient.type = ClientMessage;
    ev.xclient.display = dpy;
    ev.xclient.window = w;
    ev.xclient.message_type = _KIPC_COMM_ATOM;
    ev.xclient.format = 32;
    ev.xclient.data.l[0] = msg;
    ev.xclient.data.l[1] = data;
    XSendEvent(dpy, w, False, 0L, &ev);
}


