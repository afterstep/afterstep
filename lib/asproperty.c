/*
 * Copyright (c) 1999 Ethan Fischer <allanon@crystaltokyo.com>
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

#include "../configure.h"

#include <stdio.h>
#include <stdlib.h>
#include <X11/X.h>
#include <X11/Xatom.h>

#include "../include/aftersteplib.h"


void
set_as_property (Display * dpy, Window w, Atom name, unsigned long *data, size_t data_size, unsigned long version)
{
  unsigned long *buffer;

  buffer = (unsigned long *) safemalloc (2 * sizeof (unsigned long) + data_size);
  /* set the property version to 1.0 */
  buffer[0] = version;
  /* the size of meaningful data to store */
  buffer[1] = data_size;
  /* fill in the properties */
  memcpy (&(buffer[2]), data, data_size);

  XChangeProperty (dpy, w, name, XA_INTEGER, 32, PropModeReplace, (unsigned char *) buffer, 2 + data_size / sizeof (unsigned long));
  free (buffer);
}

unsigned long *
get_as_property (Display * dpy, Window w, Atom name, size_t * data_size, unsigned long *version)
{
  unsigned long *data = NULL;
  unsigned long *header;
  int actual_format;
  Atom actual_type;
  unsigned long junk;

  /* try to get the property version and data size */
  if (XGetWindowProperty (dpy, w, name, 0, 2, False, AnyPropertyType, &actual_type, &actual_format, &junk, &junk, (unsigned char **) &header) != Success)
    return NULL;
  if (header == NULL)
    return NULL;

  *version = header[0];
  *data_size = header[1];
  XFree (header);
  if (actual_type == XA_INTEGER)
    {
      /* try to get the actual information */
      if (XGetWindowProperty (dpy, w, name, 2, *data_size / sizeof (unsigned long), False, AnyPropertyType, &actual_type, &actual_format, &junk, &junk, (unsigned char **) &data) != Success)
	  data = NULL;
    }

  return data;

}
