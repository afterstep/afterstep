/*
 * Copyright (c) 2002 Sasha Vasko <sasha@aftercode.net>
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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>

/* #define LOCAL_DEBUG */

#include "../include/aftersteplib.h"
#include "../include/afterstep.h"
#include "../include/loadimg.h"
#include "../libAfterImage/afterimage.h"
#include "../include/screen.h"

void
asimage2icon (ASImage * im, icon_t * icon, Bool ignore_alpha)
{
	icon->image = im;
	if (im)
	{
		icon->pix = asimage2pixmap (Scr.asv, Scr.Root, icon->image, NULL, False);
		if (!ignore_alpha)
		{
			int           depth = check_asimage_alpha (Scr.asv, im);

			if (depth > 0)
				icon->mask = asimage2alpha (Scr.asv, Scr.Root, im, NULL, False, True);
			if (depth == 8)
				icon->alpha = asimage2alpha (Scr.asv, Scr.Root, im, NULL, False, False);
		}
		icon->width = im->width;
		icon->height = im->height;
	}
}

void
free_icon_resources (icon_t icon)
{
	if (icon.pix)
		UnloadImage (icon.pix);
	if (icon.mask)
		UnloadMask (icon.mask);
	if (icon.alpha)
		UnloadMask (icon.alpha);
	if (icon.image)
		safe_asimage_destroy (icon.image);
}

void
destroy_icon(icon_t **picon)
{
    icon_t *pi = *picon ;
    if( pi )
    {
        if (pi->pix)
            UnloadImage(pi->pix);
        if (pi->mask)
            UnloadMask (pi->mask);
        if (pi->alpha)
            UnloadMask (pi->alpha);
        if (pi->image)
            safe_asimage_destroy (pi->image);
        free( pi );
        *picon = NULL ;
    }
}

