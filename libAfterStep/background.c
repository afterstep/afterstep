/*
 * Copyright (c) 1999 Sasha Vasko <sasha at aftercode.net>
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/*#define DO_CLOCKING      */

#define TRUE 1
#define FALSE 0

#include "../configure.h"

#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#define DO_CLOCKING
#ifdef DO_CLOCKING
#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
#endif

#include <X11/Intrinsic.h>

#include "../include/afterbase.h"
#include "asapp.h"
#include "afterstep.h"
#include "screen.h"
#include "mystyle.h"
#include "background.h"


#ifdef DEBUG_BACKGROUNDS
#define LOG1(a)       fprintf( stderr, a );
#define LOG2(a,b)    fprintf( stderr, a, b );
#define LOG3(a,b,c)    fprintf( stderr, a, b, c );
#define LOG4(a,b,c,d)    fprintf( stderr, a, b, c, d );
#else
#define LOG1(a)
#define LOG2(a,b)
#define LOG3(a,b,c)
#define LOG4(a,b,c,d)
#endif

/***************************************************/
/* this all should go into asimagelib/background.c */
static Atom   RootPixmapProperty = None;

ASDeskBack   *
FindDeskBack (ASDeskBackArray * backs, long desk)
{
	unsigned long i;

	for (i = 0; i < backs->desks_num; i++)
		if (backs->desks[i].desk == desk)
			return &(backs->desks[i]);

	return NULL;
}

int
IsPurePixmap (ASDeskBack * back)
{
	return (back->data_type == XA_PIXMAP && back->data.pixmap != None && back->MyStyle == None);
}

void
BackgroundSetPixmap (Pixmap pix)
{
	Pixmap        current_root = GetRootPixmap (RootPixmapProperty);

	if (current_root == pix)
	{
		XClearWindow (dpy, Scr.Root);
		return;
	}

	XSetWindowBackgroundPixmap (dpy, Scr.Root, pix);
	XClearWindow (dpy, Scr.Root);

	/* we shell be setting root pixmap ID in X property with specifyed ID */
	if (RootPixmapProperty != None)
	{
		XChangeProperty (dpy, Scr.Root, RootPixmapProperty, XA_PIXMAP, 32, PropModeReplace, (char *)&pix, 1);
		XFlush (dpy);						   /* so that everyone has time to process this change
											      * before we go ahead and delete old background
											    */
	}
}

Pixmap
BackgroundSetMyStyle (char *style_name)
{
	MyStyle      *style;
	Pixmap        pix = None;

	/* get MySyle pointer */
	if ((style = mystyle_find (style_name)) != NULL)
	{
		LOG2 ("\nBackgroundSetMyStyle( %s ) ", style_name)
	  	if ((style->set_flags & F_BACKPIXMAP) && style->texture_type != 129)
		{
			/* we don't want to free this pixmap ourselves */
			BackgroundSetPixmap (style->back_icon.pix);
			return None;
		} else if (style->set_flags & F_BACKGRADIENT)
			pix = mystyle_make_pixmap (style, Scr.MyDisplayWidth, Scr.MyDisplayHeight, None);
		if (pix == None)
		{
			GC            backGC, foreGC;

			pix = XCreatePixmap (dpy, Scr.Root, 1, 1, Scr.d_depth);
/*			mystyle_get_global_gcs (style, &foreGC, &backGC, NULL, NULL);
			XDrawPoint (dpy, pix, (backGC != None) ? backGC : foreGC, 1, 1);
  */		}
		BackgroundSetPixmap (pix);
	}
	/* we will need this pixmap ID later to destroy it gracefully */
	return pix;
}

/******************************************************************/
/*     stuff for running external app to set root background      */
/******************************************************************/
#ifdef HAVE_SYS_WAIT_H
#define WAIT_CHILDREN(pstatus)  waitpid(-1, pstatus, WNOHANG)
#elif defined (HAVE_WAIT3)
#define WAIT_CHILDREN(pstatus)  wait3(pstatus, WNOHANG, NULL)
#else
#define WAIT_CHILDREN(pstatus)  (-1)
#endif

int           DrawChildPID = 0;
void
sigchild_handler (int signum)
{
	int           pid;
	int           status;

	signal (SIGCHLD, sigchild_handler);
	LOG3 ("\n%s:Entering SigChild_handler(%lu)", MyName, time (NULL)) while (1)
	{
		pid = WAIT_CHILDREN (&status);
		if (pid < 0 || pid == DrawChildPID)
			DrawChildPID = 0;
		if (pid == 0 || pid < 0)
			break;
	}
LOG3 ("\n%s:Exiting SigChild_handler(%lu)", MyName, time (NULL))}

/*
   This should return 0 if process of running external app to draw background completed or killed.
   otherwise it returns > 0
 */
int
CheckForDrawChild (int kill_it_to_death)
{
	int           i;
	int           status;

	LOG3 ("\n%s:CheckingForDrawChild(%lu)....", MyName, time (NULL));
	if (DrawChildPID > 0)
	{
		LOG2 ("\n Child has been started with PID (%d).", DrawChildPID);
		if (kill_it_to_death > 0)
		{
			kill (DrawChildPID, SIGTERM);
			for (i = 0; i < 10; i++)		   /* give it 10 sec to terminate */
			{
				sleep (1);
				if (WAIT_CHILDREN (&status) <= 0)
					break;
			}
			if (i >= 10)
				kill (DrawChildPID, SIGKILL);  /* no more mercy */
			DrawChildPID = 0;
		}
	} else if (DrawChildPID < 0)
		DrawChildPID = 0;

	LOG3 ("Done(%lu). Child PID on exit = %d.", time (NULL), DrawChildPID);
	return DrawChildPID;
}

void
BackgroundSetCommand (char *cmd)
{
	signal (SIGCHLD, sigchild_handler);
	if (!(DrawChildPID = fork ()))			   /* child process */
	{
		execl ("/bin/sh", "/bin/sh", "-c", cmd, (char *)NULL);

		/* if all is fine then the thread will exit here */
		/* so displaying error if not                    */
		fprintf (stderr, "\n%s: bad luck running command [%s] to set root pixmap.\n", MyName, cmd);
		exit (0);							   /*thread completed */
	}
}

void
KillOldDrawing ()
{
	CheckForDrawChild (1);
}

/* frees all the resources that we currently don't need :
   - pixmaps used for MyStyle's
 */
void
BackgroundCleanup (ASDeskBackArray * backs, long desk)
{
	unsigned long i;

	for (i = 0; i < backs->desks_num; i++)
		if (backs->desks[i].desk != desk && backs->desks[i].data.pixmap != None)
		{
			if (backs->desks[i].data_type != XA_PIXMAP && backs->desks[i].MyStyle != None)
			{
				XFreePixmap (dpy, backs->desks[i].data.pixmap);
				backs->desks[i].data.pixmap = None;
			}
		}
}


/******************************************************************/
/*        application interface                                   */
/******************************************************************/

#ifdef DEBUG_BACKGROUNDS
void
PrintDeskBackArray (ASDeskBackArray * backs)
{
	int           i = 0;

	fprintf (stderr, "\n Number of backgrounds = %d", backs->desks_num);
	for (i = 0; i < backs->desks_num; i++)
		fprintf (stderr, "\n %d. Desk #%ld type %ld data %ld, MyStyle %ld",
				 i, backs->desks[i].desk, backs->desks[i].data_type,
				 backs->desks[i].data.atom, backs->desks[i].MyStyle);

}
#endif

void
SetRootPixmapPropertyID (Atom id)
{
	RootPixmapProperty = id;
}

void
BackgroundSetForDesk (ASDeskBackArray * backs, long desk)
{
	ASDeskBack   *back;

	if (desk != 10000 && backs)
	{
		LOG2 ("\ntrying to find data for desk %d", desk) if ((back = FindDeskBack (backs, desk)) != NULL)
		{
			LOG3 ("\ndesk %d found with pixmap %ld", desk, back->data.pixmap) KillOldDrawing ();
			if ((back->data_type == XA_PIXMAP) || (back->MyStyle != None && back->data.pixmap != None))
				BackgroundSetPixmap (back->data.pixmap);
			else
			{
				char         *text = XGetAtomName (dpy, ((back->MyStyle != None) ? back->MyStyle : back->data.atom));

				LOG3 ("\ndesk %d has data [%s]", desk, text) if (text != NULL)
				{
					if (back->MyStyle != None)
						back->data.pixmap = BackgroundSetMyStyle (text);
					else
						BackgroundSetCommand (text);

					XFree (text);
				}
			}
		}
		BackgroundCleanup (backs, desk);
	}
}

void
FreeDeskBackArray (ASDeskBackArray * backs, int free_pixmaps)
{
	if (backs->desks)
	{
		int           i, k;

		for (i = 0; i < backs->desks_num && free_pixmaps; i++)
		{

			if (backs->desks[i].data_type == XA_PIXMAP || backs->desks[i].MyStyle)
				if (backs->desks[i].data.pixmap != None)
				{
					if (backs->desks[i].desk == Scr.CurrentDesk && RootPixmapProperty != None)
					{
						Pixmap        pix = None;

						XChangeProperty (dpy, Scr.Root, RootPixmapProperty, XA_PIXMAP, 32, PropModeReplace,
										 (char *)&pix, 1);
						XFlush (dpy);
						/* so that everyone has time to process this change
						 * before we go ahead and delet old background
						 */
					}
					/* checking if we have already freed this pixmap */
					for (k = 0; k < i; k++)
						if (backs->desks[k].data_type == XA_PIXMAP || backs->desks[k].MyStyle)
							if (backs->desks[k].data.pixmap == backs->desks[i].data.pixmap)
								break;

					if (k >= i)
                        XFreePixmap(dpy,backs->desks[i].data.pixmap);
				}
		}
		free (backs->desks);
		backs->desks = NULL;
    }
	backs->desks_num = 0;
}

ASDeskBackArray *
UpdateDeskBackArray (ASDeskBackArray * old_info, ASDeskBackArray * new_info)
{
	ASDeskBackArray *updated;
	int           i, k;

	updated = (ASDeskBackArray *) safemalloc (sizeof (ASDeskBackArray));
	updated->desks_num = new_info->desks_num;
	for (k = 0; k < old_info->desks_num; k++)
		if (FindDeskBack (new_info, old_info->desks[k].desk) == NULL)
			updated->desks_num++;

	updated->desks = (ASDeskBack *) safemalloc (sizeof (ASDeskBack) * updated->desks_num);
	for (i = 0; i < new_info->desks_num; i++)
		updated->desks[i] = new_info->desks[i];

	for (k = 0; k < old_info->desks_num; k++)
		if (FindDeskBack (new_info, old_info->desks[k].desk) == NULL)
		{
			updated->desks[i] = old_info->desks[k];
			i++;
		}

	return updated;
}

/******************************************************************/
/*          Backgrounds info X property manipulation              */
/******************************************************************/

void
SetBackgroundsProperty (ASDeskBackArray * backs, Atom property)
{
	if (backs == NULL || property == None)
		return;
	if (backs->desks == NULL || backs->desks_num <= 0)
		return;
	set_as_property (Scr.Root, property, (unsigned long *)(backs->desks), backs->desks_num * sizeof (ASDeskBack),
					 (1 << 8) + 0);
}

void
GetBackgroundsProperty (ASDeskBackArray * backs, Atom property)
{
	unsigned long version;
	unsigned long *data;

	if (backs == NULL || property == None)
		return;
	if (backs->desks != NULL || backs->desks_num > 0)
		FreeDeskBackArray (backs, FALSE);
	data = get_as_property (Scr.Root, property, (size_t *) & (backs->desks_num), &version);
	if (data && backs->desks_num > 0 && version == (1 << 8) + 0)
	{
		backs->desks = (ASDeskBack *) safemalloc (backs->desks_num);
		memcpy (backs->desks, data, backs->desks_num);
		XFree (data);
		backs->desks_num = backs->desks_num / sizeof (ASDeskBack);
	}
}
