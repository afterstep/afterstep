/* This file contains code for unified image loading from file

 *  Copyright (c) 1998 Sasha Vasko <sasha at aftercode.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#define LIBASIMAGE
/*#define DEBUG_LOADIMAGE */

#include "../configure.h"

#include "../include/asapp.h"
#include "../libAfterImage/afterimage.h"
#include "../include/loadimg.h"

/* it can be adjusted via $SCREEN_GAMMA env. variable */
typedef struct pixmap_ref
{
	struct pixmap_ref *next;
	int           refcount;
	char         *name;
	Pixmap        pixmap;
	Pixmap        mask;
}
pixmap_ref_t;

static ASVisual *_as_ASVisual = NULL;
static pixmap_ref_t *pixmap_ref_first = NULL;
static int    use_pixmap_ref = 1;

/********************************************************************/
/* AfterStep ASVisual management 									*/
/********************************************************************/
ASVisual     *
GetASVisual ()
{
	if (_as_ASVisual == NULL)
	{
		int           screen = DefaultScreen (dpy);

		_as_ASVisual = create_asvisual (dpy, screen, DefaultDepth (dpy, screen), NULL);

	}
	return _as_ASVisual;
}

void
CleanupASVisual ()
{
	if (_as_ASVisual == NULL)
	{
		destroy_asvisual (_as_ASVisual, False);
		_as_ASVisual = NULL;
	}
}

/********************************************************************/
/* pixmpa reference counting                                        */
/********************************************************************/
int
set_use_pixmap_ref (int on)
{
	int           old = use_pixmap_ref;

	use_pixmap_ref = on;
	return old;
}

pixmap_ref_t *
pixmap_ref_new (const char *filename, Pixmap pixmap, Pixmap mask)
{
	pixmap_ref_t *ref = safemalloc (sizeof (pixmap_ref_t));

	ref->next = pixmap_ref_first;
	pixmap_ref_first = ref;
	ref->refcount = 1;
	ref->name = (filename == NULL) ? NULL : mystrdup (filename);
	ref->pixmap = pixmap;
	ref->mask = mask;
	return ref;
}

void
pixmap_ref_delete (pixmap_ref_t * ref)
{
	if (ref == NULL)
		return;

	if (pixmap_ref_first == ref)
		pixmap_ref_first = (*ref).next;
	else if (pixmap_ref_first != NULL)
	{
		pixmap_ref_t *ptr;

		for (ptr = pixmap_ref_first; (*ptr).next != NULL; ptr = (*ptr).next)
			if ((*ptr).next == ref)
				break;
		if ((*ptr).next == ref)
			(*ptr).next = (*ref).next;
	}

	if (ref->name != NULL)
		free (ref->name);
	if (ref->pixmap != None)
		XFreePixmap (dpy, ref->pixmap);
	if (ref->mask != None)
		XFreePixmap (dpy, ref->mask);

	free (ref);
}

pixmap_ref_t *
pixmap_ref_find_by_name (const char *filename)
{
	pixmap_ref_t *ref;

	for (ref = pixmap_ref_first; ref != NULL; ref = ref->next)
		if (!strcmp (ref->name, filename))
			break;
	return ref;
}

pixmap_ref_t *
pixmap_ref_find_by_pixmap (Pixmap pixmap)
{
	pixmap_ref_t *ref;

	for (ref = pixmap_ref_first; ref != NULL; ref = ref->next)
		if (ref->pixmap == pixmap)
			break;
	return ref;
}

pixmap_ref_t *
pixmap_ref_find_by_mask (Pixmap mask)
{
	pixmap_ref_t *ref;

	for (ref = pixmap_ref_first; ref != NULL; ref = ref->next)
		if (ref->mask == mask)
			break;
	return ref;
}

int
pixmap_ref_increment (pixmap_ref_t * ref)
{
	return ++ref->refcount;
}

int
pixmap_ref_decrement (pixmap_ref_t * ref)
{
	int           c = --ref->refcount;

/* don't delete the reference, the pixmap might be immediately reloaded;
   ** let the app tell us when to purge the pixmap via pixmap_ref_purge() */
/*
   if (c <= 0)
   pixmap_ref_delete(ref);
 */
	return c;
}

int
pixmap_ref_purge (void)
{
	int           done = 0, count;

	for (count = -1; !done; count++)
	{
		pixmap_ref_t *ref;

		for (ref = pixmap_ref_first; ref != NULL; ref = ref->next)
			if (ref->refcount <= 0)
			{
				pixmap_ref_delete (ref);
				break;
			}
		done = (ref == NULL);
	}
	return count;
}


Pixmap
LoadImageWithMask (Display * dpy, Window w, unsigned long max_colors, const char *realfilename, Pixmap * pMask)
{
	Pixmap        p = None;
	pixmap_ref_t *ref = NULL;

	if (use_pixmap_ref)
	{
		ref = pixmap_ref_find_by_name (realfilename);
		if (ref != NULL)
		{
			pixmap_ref_increment (ref);
			*pMask = ref->mask;
			return ref->pixmap;
		}
	}

	if (pMask)
		*pMask = None;
	p = file2pixmap (GetASVisual (), w, realfilename, pMask);

	if (pMask && *pMask)
	{
		int           dumm;
		unsigned int  udumm, depth;
		Window        root;

		XGetGeometry (dpy, *pMask, &root, &dumm, &dumm, &udumm, &udumm, &udumm, &depth);
		show_progress ("Loaded image [%s]; mask has depth of %d", realfilename, depth);
	}

	if (p == None)
	{
		fprintf (stderr, "%s:Failed to load image [%s].\n", MyName, realfilename);
	}

	if (use_pixmap_ref && p)
	{
		pixmap_ref_new (realfilename, p, *pMask);
	}
	return p;
}

Pixmap
LoadImageWithMaskAndScale (Display * dpy, Window w, unsigned long max_colors, const char *realfilename,
						   unsigned int to_width, unsigned int to_height, Pixmap * pMask)
{
	Pixmap        p = None;
	ASImage      *im;
	double        gamma;
	char         *gamma_str;

	if ((gamma_str = getenv ("SCREEN_GAMMA")) != NULL)
		gamma = atof (gamma_str);
	else
		gamma = SCREEN_GAMMA;
	if (gamma == 0.0)
		gamma = SCREEN_GAMMA;

	if ((im = file2ASImage (realfilename, 0xFFFFFFFF, gamma, 0, NULL)) != NULL)
	{
		if (to_width == 0)
			to_width = im->width;
		if (to_height == 0)
			to_height = im->height;
		if (im->width != to_width || im->height != to_height)
		{
			ASImage      *scaled_im = scale_asimage (GetASVisual (), im, to_width, to_height,
													 pMask ? ASA_XImage : ASA_ASImage,
													 0, ASIMAGE_QUALITY_DEFAULT);

			if (scaled_im)
			{
				destroy_asimage (&im);
				im = scaled_im;
			}
		}
		p = asimage2pixmap (GetASVisual (), w, im, NULL, True);
		if (pMask)
			*pMask = asimage2pixmap (GetASVisual (), w, im, NULL, True);
	}
	return p;
}

/* UnloadImage()
 * if pixmap is found in reference list, decrements reference count; else
 * XFreePixmap()'s the pixmap
 * returns number of remaining references to pixmap */
#if defined(LOG_LOADIMG_CALLS) && defined(DEBUG_ALLOCS)
int
l_UnloadImage (const char *file, int line, Pixmap pixmap)
{
	log_call (file, line, "UnloadImage", NULL);
#else
int
UnloadImage (Pixmap pixmap)
{
#endif
	if (use_pixmap_ref)
	{
		pixmap_ref_t *ref = pixmap_ref_find_by_pixmap (pixmap);

		if (ref != NULL)
			return pixmap_ref_decrement (ref);
	}
	XFreePixmap (dpy, pixmap);

	return 0;
}

/* UnloadMask()
 * if mask is found in the reference list and there is no corresponding
 * pixmap, decrements reference count; else if mask is found, does
 * nothing; else XFreePixmap()'s the mask
 * returns number of remaining references to mask */
#if defined(LOG_LOADIMG_CALLS) && defined(DEBUG_ALLOCS)
int
l_UnloadMask (const char *file, int line, Pixmap mask)
{
	log_call (file, line, "UnloadImage", NULL);
#else
int
UnloadMask (Pixmap mask)
{
#endif
	if (mask == None)
		return 0;

	if (use_pixmap_ref)
	{
		pixmap_ref_t *ref = pixmap_ref_find_by_mask (mask);

		if (ref == NULL)
			XFreePixmap (dpy, mask);
		else if (ref->pixmap == None)
			return pixmap_ref_decrement (ref);
		return (ref != NULL) ? ref->refcount : 0;
	}
	XFreePixmap (dpy, mask);
	return 0;
}
