#include "../../configure.h"

#ifndef NO_TEXTURE
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Intrinsic.h>
#include <unistd.h>
#include <stdio.h>

#include "../../include/aftersteplib.h"
#include "../../include/afterstep.h"
#include "../../include/parse.h"
#include "../../include/misc.h"
#include "../../include/style.h"
#include "../../include/screen.h"
#include "../../include/module.h"

#include "../../include/loadimg.h"
#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif /* SHAPE */

static void   frame_set_sizes ();

typedef struct
{
	MyIcon          handle[8];
}
framed_win;

int           DecorateFrames = 0;
char         *FrameN = NULL;
char         *FrameS = NULL;
char         *FrameE = NULL;
char         *FrameW = NULL;
char         *FrameNE = NULL;
char         *FrameNW = NULL;
char         *FrameSE = NULL;
char         *FrameSW = NULL;

static framed_win ff;

/* load the images */
static int
frame_load_images ()
{
	if (FrameSE != NULL )
		GetIconFromFile (FrameSE, &(ff.handle[FR_SE]), 0);
	if (FrameSW != NULL )
		GetIconFromFile (FrameSW, &(ff.handle[FR_SW]), 0);
	if (FrameNE != NULL )
		GetIconFromFile (FrameNE, &(ff.handle[FR_NE]), 0);
	if (FrameNW != NULL )
		GetIconFromFile (FrameNW, &(ff.handle[FR_NW]), 0);
	if (FrameS != NULL )
		GetIconFromFile (FrameS, &(ff.handle[FR_S]), 0);
	if (FrameN != NULL )
		GetIconFromFile (FrameN, &(ff.handle[FR_N]), 0);
	if (FrameE != NULL )
		GetIconFromFile (FrameE, &(ff.handle[FR_E]), 0);
	if (FrameW != NULL )
		GetIconFromFile (FrameW, &(ff.handle[FR_W]), 0);

	return 1;
}

void
frame_init (int free_resources)
{
	int           i;

	if (free_resources)
	{
		if (FrameN != NULL)
			free (FrameN);
		if (FrameS != NULL)
			free (FrameS);
		if (FrameE != NULL)
			free (FrameE);
		if (FrameW != NULL)
			free (FrameW);
		if (FrameNE != NULL)
			free (FrameNE);
		if (FrameSE != NULL)
			free (FrameSE);
		if (FrameNW != NULL)
			free (FrameNW);
		if (FrameSW != NULL)
			free (FrameSW);
		for (i = 0; i < 8; i++)
			free_icon_resources( ff.handle[i] );
	}
	FrameN = NULL;
	FrameS = NULL;
	FrameE = NULL;
	FrameW = NULL;
	FrameNE = NULL;
	FrameSE = NULL;
	FrameNW = NULL;
	FrameSW = NULL;
	for (i = 0; i < 8; i++)
		memset(&(ff.handle[i]), 0x00, sizeof(MyIcon) );
}

/* create the gc and fill Scr.frame* with pix sizes */

void
frame_create_gcs ()
{
	frame_load_images ();
	frame_set_sizes ();
	return;
}

/* draw the frame */

void
frame_draw_frame (ASWindow * t)
{
	int           i;

	for (i = 0; i < 8; i++)
	{
		if (ff.handle[i].pix != None)
			XSetWindowBackgroundPixmap (dpy, t->fw[i], ff.handle[i].pix);
#ifdef SHAPE
		if (ff.handle[i].mask != None)
			XShapeCombineMask (dpy, t->fw[i], ShapeBounding, 0, 0, ff.handle[i].mask, ShapeSet);
#endif /* SHAPE */
		XClearWindow (dpy, t->fw[i]);
	}
	XSetWindowBackgroundPixmap (dpy, t->frame, None);
	XFlush (dpy);
	return;
}

/* free all data, should be called on restart or exit */

void
frame_free_data (ASWindow * tw, Bool all)
{
	int           i;

	for (i = 0; i < 8; i++)
	{
		if (all)
		{
			free_icon_resources( ff.handle[i] );
			memset(&(ff.handle[i]), 0x00, sizeof(MyIcon) );			
		}
		if (tw != NULL)
			XDeleteContext (dpy, tw->fw[i], ASContext);
	}
	return;
}

/* fill the Screen struct with frame sizes */
static void
frame_set_sizes ()
{
	int           w, h, i;

	for (i = 0; i < 8; i++)
	{
		if (ff.handle[i].image != NULL)
		{
			w = ff.handle[i].width ;
			h = ff.handle[i].height ;
			fprintf (stderr, "no: %d w: %d h: %d\n", i, w, h);
			Scr.fs[i].w = w;
			Scr.fs[i].h = h;
		} else
		{
			Scr.fs[i].w = 0;
			Scr.fs[i].h = 0;
		}
	}
}
void
frame_set_positions (ASWindow * tmp_win)
{
	int           i, px, py, pw, ph;

	/* get geometry of parent of client window */
	get_parent_geometry (tmp_win, tmp_win->frame_width, tmp_win->frame_height, &px, &py, &pw, &ph);
	/* get outside geometry of frame */
	px += tmp_win->bw - Scr.fs[FR_W].w;
	py += tmp_win->bw - Scr.fs[FR_N].h;
	pw += Scr.fs[FR_W].w + Scr.fs[FR_E].w;
	ph += Scr.fs[FR_N].h + Scr.fs[FR_S].h;
	for (i = 0; i < 8; i++)
	{
		switch (i)
		{
		 case FR_N:
			 tmp_win->fp[i].x = px + Scr.fs[FR_NW].w;
			 tmp_win->fp[i].y = py;
			 tmp_win->fp[i].w = pw - Scr.fs[FR_NW].w - Scr.fs[FR_NE].w;
			 tmp_win->fp[i].h = Scr.fs[FR_N].h;
			 break;
		 case FR_S:
			 tmp_win->fp[i].x = px + Scr.fs[FR_SW].w;
			 tmp_win->fp[i].y = py + ph - Scr.fs[FR_S].h;
			 tmp_win->fp[i].w = pw - Scr.fs[FR_SW].w - Scr.fs[FR_SE].w;
			 tmp_win->fp[i].h = Scr.fs[FR_S].h;
			 break;
		 case FR_E:
			 tmp_win->fp[i].x = px + pw - Scr.fs[FR_E].w;
			 tmp_win->fp[i].y = py + Scr.fs[FR_NE].h;
			 tmp_win->fp[i].w = Scr.fs[FR_E].w;
			 tmp_win->fp[i].h = ph - Scr.fs[FR_NE].h - Scr.fs[FR_SE].h;
			 break;
		 case FR_W:
			 tmp_win->fp[i].x = px;
			 tmp_win->fp[i].y = py + Scr.fs[FR_NW].h;
			 tmp_win->fp[i].w = Scr.fs[FR_W].w;
			 tmp_win->fp[i].h = ph - Scr.fs[FR_NW].h - Scr.fs[FR_SW].h;
			 break;
		 case FR_NW:
			 tmp_win->fp[i].x = px;
			 tmp_win->fp[i].y = py;
			 tmp_win->fp[i].w = Scr.fs[FR_NW].w;
			 tmp_win->fp[i].h = Scr.fs[FR_NW].h;
			 break;
		 case FR_NE:
			 tmp_win->fp[i].x = px + pw - Scr.fs[FR_NE].w;
			 tmp_win->fp[i].y = py;
			 tmp_win->fp[i].w = Scr.fs[FR_NE].w;
			 tmp_win->fp[i].h = Scr.fs[FR_NE].h;
			 break;
		 case FR_SE:
			 tmp_win->fp[i].x = px + pw - Scr.fs[FR_SE].w;
			 tmp_win->fp[i].y = py + ph - Scr.fs[FR_SE].h;
			 tmp_win->fp[i].w = Scr.fs[FR_SE].w;
			 tmp_win->fp[i].h = Scr.fs[FR_SE].h;
			 break;
		 case FR_SW:
			 tmp_win->fp[i].x = px;
			 tmp_win->fp[i].y = py + ph - Scr.fs[FR_SW].h;
			 tmp_win->fp[i].w = Scr.fs[FR_SW].w;
			 tmp_win->fp[i].h = Scr.fs[FR_SW].h;
			 break;
		}
	}
}

Bool
frame_create_windows (ASWindow * tmp_win)
{
	unsigned long valuemask;
	XSetWindowAttributes attributes;
	int           i;

	if (!ASWIN_HFLAGS(tmp_win, AS_Frame))
		return False;

	valuemask = CWBackPixmap | CWBorderPixel | CWCursor | CWEventMask;
	attributes.background_pixmap = ParentRelative;
	attributes.border_pixel = Scr.asv->black_pixel;
	if (Scr.flags & BackingStore)
	{
		valuemask |= CWBackingStore;
		attributes.backing_store = WhenMapped;
	}
	attributes.event_mask = (ButtonPressMask | ButtonReleaseMask | ExposureMask |
							 EnterWindowMask | LeaveWindowMask);
	frame_set_positions (tmp_win);

	for (i = 0; i < 8; i++)
	{
		if (ff.handle[i].pix != None)
		{
			switch (i)
			{
			 case FR_N:
				 attributes.cursor = Scr.ASCursors[TOP];
				 break;
			 case FR_S:
				 attributes.cursor = Scr.ASCursors[BOTTOM];
				 break;
			 case FR_E:
				 attributes.cursor = Scr.ASCursors[RIGHT];
				 break;
			 case FR_W:
				 attributes.cursor = Scr.ASCursors[LEFT];
				 break;
			 case FR_NW:
				 attributes.cursor = Scr.ASCursors[TOP_LEFT];
				 break;
			 case FR_NE:
				 attributes.cursor = Scr.ASCursors[TOP_RIGHT];
				 break;
			 case FR_SE:
				 attributes.cursor = Scr.ASCursors[BOTTOM_RIGHT];
				 break;
			 case FR_SW:
				 attributes.cursor = Scr.ASCursors[BOTTOM_LEFT];
				 break;
			}
			tmp_win->fw[i] =
				create_visual_window (Scr.asv, tmp_win->frame, tmp_win->fp[i].x,
									  tmp_win->fp[i].y, tmp_win->fp[i].w, tmp_win->fp[i].h, 0,
									  InputOutput, valuemask, &attributes);
			XSaveContext (dpy, tmp_win->fw[i], ASContext, (caddr_t) tmp_win);
		}
	}
	return True;
}

#endif /* !NO_TEXTURE */
