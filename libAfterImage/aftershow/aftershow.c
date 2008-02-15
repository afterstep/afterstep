/*
 * Copyright (c) 2008 Sasha Vasko <sasha at aftercode.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#undef LOCAL_DEBUG
#undef DO_CLOCKING

#ifdef _WIN32
#include "../win32/config.h"
#else
#include "../config.h"
#endif


#include <string.h>
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
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif

#include <errno.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/un.h>							   /* for struct sockaddr_un */

#ifdef _WIN32
# include "../win32/afterbase.h"
#else
# include "../afterbase.h"
#endif
#include "../afterimage.h"

#include "aftershow.h"

/**********************************************************************************/
/* main portion of AfterShow daemon                                               */
/**********************************************************************************/

Bool InitContext (AfterShowContext *context, int argc, char **argv);
Bool ConnectGUI (AfterShowContext *context);
Bool CheckInstance (AfterShowContext *context);
Bool SetupComms (AfterShowContext *context);
void HandleEvents (AfterShowContext *context);
void ShutdownXScreen (AfterShowContext *ctx, int screen);
void ShutdownWin32Screen (AfterShowContext *ctx, int screen);
void ShutdownClient (AfterShowContext *ctx, int channel);
void Shutdown (AfterShowContext *ctx);

void show_usage (Bool short_form)
{
	printf( "AfterShow - AfterImage XML processing and display daemon.\n"
			"Usage: aftershow [-h|--help] [-f] [-s] [-d|--display <display_string>]\n"
			"  -h --help      - display this message;\n"
			"  -f             - fork - go in background immidiately on startup;\n"
			"  -s             - single screen mode - sirvice only the first screen of display;\n"
			"  -d --display <display_string> - connect to X display specified by display_string;\n"
			"  -V <level>     - set output verbosity level - 0 for no output;\n" );
}


int 
main (int argc, char **argv)
{
	AfterShowContext context;
	/* 
	 * 1) Connect to windowing system
	 * 2) Check for another instance running for the same user
	 * 3) Create UD Socket
	 * 4) Handle Events
	 */
	set_application_name(argv[0]);
	set_output_threshold(0);  /* be real quiet by default ! */
	
	if (!InitContext(&context, argc, argv))
		return EXIT_FAILURE;

	if (!ConnectGUI(&context))
		return EXIT_FAILURE;
		
	if (!CheckInstance(&context))
	{
		show_error( "Another instance is already running. Exiting!");
		return EXIT_FAILURE;
	}
		
	if (!SetupComms(&context))
	{
		show_error( "Another instance is already running. Exiting!");
		return EXIT_FAILURE;
	}

	if (get_flags(context.flags, AfterShow_DoFork))
    {
		pid_t pid = fork ();
		
		if (pid < 0)
        {
        	show_error ("unable to fork daemon, aborting.");
          	return EXIT_FAILURE;
        }
      	else if (pid > 0)
        	_exit (EXIT_SUCCESS);
    }

	HandleEvents(&context);
   	return EXIT_SUCCESS;
}

/**********************************************************************************
 * Implementation : 
 **********************************************************************************/
Bool InitContext (AfterShowContext *ctx, int argc, char **argv)
{
	int i;
	memset( ctx, 0x00, sizeof(AfterShowContext));
	for (i = 1 ; i < argc ; ++i) 
	{
		if (argv[i][0] == '-')
		{
			if (argv[i][1] != '-') /* short option */
			{
				if (argv[i][2] == '\0')
					switch (argv[i][1])
					{
						case 'h': 	show_usage(False); return False;
						case 'f': 	set_flags(ctx->flags, AfterShow_DoFork); break;
						case 's': 	set_flags(ctx->flags, AfterShow_SingleScreen); break;
						case 'V': 	if (i + 1 < argc)
										set_output_threshold(atoi(argv[++i]));
									break;
						case 'd': 	if (i + 1 < argc)
									{
										if (ctx->display) free (ctx->display);
										ctx->display = strdup(argv[++i]);
									}
									break;
						default :
							show_error ("unrecognized option \"%s\"", argv[i]);
							show_usage(False); 
							return False;
					}
			}else /* long option */
			{
				if (strcmp(&(argv[i][2]), "help") == 0)
				{ 
					show_usage(False); return False; 
				}else
				{
					show_error ("unrecognized option \"%s\"", argv[i]);
					show_usage(False); 
					return False;
				}
			}
		}
	}
	
	return True;
}

Bool ConnectGUI (AfterShowContext *ctx)
{
#ifndef X_DISPLAY_MISSING
	int i;
	AfterShowXScreen *scr;
	ctx->gui.x.dpy = XOpenDisplay(ctx->display);
	if (ctx->gui.x.dpy != NULL)
	{
		ctx->gui.x.fd = XConnectionNumber (ctx->gui.x.dpy);
		ctx->gui.x.screens_num = get_flags(ctx->flags, AfterShow_SingleScreen)?1:ScreenCount (ctx->gui.x.dpy);	
		ctx->gui.x.screens = scr = safecalloc(ctx->gui.x.screens_num, sizeof(AfterShowXScreen));

		for (i = 0; i < ctx->gui.x.screens_num; ++i)
		{
			scr->screen = i;
			scr->root = RootWindow(ctx->gui.x.dpy, scr->screen);
			scr->root_width = DisplayWidth (ctx->gui.x.dpy, scr->screen);
			scr->root_height = DisplayHeight (ctx->gui.x.dpy, scr->screen);
			++scr;
		}
		ctx->gui.x.valid = True;
		show_progress( "X display \"%s\" connected. Servicing %d screens.", ctx->display?ctx->display:"", ctx->gui.x.screens_num);
	}else
	{
		if (ctx->display)
	        show_error("failed to initialize X Window session for display \"%s\"", ctx->display);
		else
			show_error("failed to initialize X Window session for default display");
	}
#endif
  /* TODO: add win32 code */

	return 	(ctx->gui.x.valid || ctx->gui.win32.valid);
}

Bool CheckInstance (AfterShowContext *ctx)
{
#ifndef X_DISPLAY_MISSING
	int i;
	AfterShowXScreen *scr = ctx->gui.x.screens;
	XEvent event;
	
	for (i = 0; i < ctx->gui.x.screens_num; ++i)
	{
		char        tmp[64];
        CARD32      data = 0xAAAAAAAA;
		Window      old_selection_owner = None;
		int 		tick_count;
		
		/* intern selection atom */		
		sprintf (tmp, "_AFTERSHOW_S%d", i);
		scr->selection_window = XCreateSimpleWindow (ctx->gui.x.dpy, scr->root, 0, 0, 5, 5, 0, 0, None);
		if (scr->selection_window)
		{
			scr->_XA_AFTERSHOW_S = XInternAtom (ctx->gui.x.dpy, tmp, False);
			XSelectInput (ctx->gui.x.dpy, scr->selection_window, PropertyChangeMask);

		   /* now we need to obtain a valid timestamp : */

        	XChangeProperty (ctx->gui.x.dpy, scr->selection_window, scr->_XA_AFTERSHOW_S, scr->_XA_AFTERSHOW_S, 32, PropModeAppend, (unsigned char *)&data, 1);
			XWindowEvent(ctx->gui.x.dpy, scr->selection_window, PropertyChangeMask, &event); 
			scr->selection_time = event.xproperty.time;

			old_selection_owner = XGetSelectionOwner (ctx->gui.x.dpy, scr->_XA_AFTERSHOW_S);
			/* now we are ready to try and accure selection : */
			show_progress( "Attempting to accure AfterShow selection on screen %d ...", i );
			XSetSelectionOwner (ctx->gui.x.dpy, scr->_XA_AFTERSHOW_S, scr->selection_window, scr->selection_time);
			XSync (ctx->gui.x.dpy, False);
			/* give old owner 10 seconds to release selection : */
			start_ticker(1);
			for (tick_count = 0; tick_count < 100; tick_count++)
			{
    	    	Window present_owner = XGetSelectionOwner (ctx->gui.x.dpy, scr->_XA_AFTERSHOW_S);
	        	if ( present_owner == scr->selection_window)
				{
					scr->do_service = True;
					break;
				}
				wait_tick();
			}

			if (scr->do_service && old_selection_owner != None)
			{
				/* we need to wait for previous owner to shutdown, which
				 * will be indicated by its selection window becoming invalid.
				 */
				tick_count = 0 ;
				while (aftershow_validate_drawable( ctx, old_selection_owner))
				{
					wait_tick();
					if (++tick_count  == 200)
					{ /* if old owner failed to shutdown in 20 seconds -
					   * then just kill it ! */
						XKillClient (ctx->gui.x.dpy, old_selection_owner);
						show_warning ("Previous AfterShow daemon failed to shutdown in allowed 60 seconds - killing it.");
					}
					if (tick_count > 300)
					{
						scr->do_service = False;
						show_error("Previous AfterShow failed to shutdown in allowed 30 seconds - Something is terribly wrong - ignoring screen %d.", i );
						break;
					}
				}
			}
			if (scr->do_service)
			{
				show_progress ("AfterShow selection on screen %d accured successfuly!", i);
				scr->asv = create_asvisual (ctx->gui.x.dpy, scr->screen, DefaultDepth(ctx->gui.x.dpy, scr->screen), NULL);
				ctx->gui.x.serviced_screens_num++;
			}else
			{
				if (scr->selection_window)
				{
					XDestroyWindow (ctx->gui.x.dpy, scr->selection_window);
					scr->selection_window = None;
				}
			}
		}
		++scr;
	}
#endif

	return (ctx->gui.x.serviced_screens_num>0);
}

Bool SetupComms (AfterShowContext *ctx)
{
	int first_screen = 0;
	char *path = getenv("TMPDIR");
	if (path == NULL)
		path = "/tmp";

	if( access( path, W_OK ) != 0 )
	    if( (path = getenv( "HOME" )) == NULL )
			return False ;

	if (ctx->gui.x.valid)
	{
		for ( first_screen = 0; first_screen < ctx->gui.x.screens_num; ++first_screen)
			if (ctx->gui.x.screens[first_screen].do_service)
				break;
	}
	
    ctx->socket_name = safemalloc (strlen(path) + 1 + 18 + 32 + 32 + 1);
	sprintf (ctx->socket_name, "%s/aftershow-connect.%d.%d", path, getuid(), first_screen);
	show_progress ("Socket name set to \"%s\".", ctx->socket_name);
	
	ctx->min_fd = ctx->socket_fd = socket_listen (ctx->socket_name);

#ifndef X_DISPLAY_MISSING
	if (ctx->gui.x.valid && ctx->gui.x.fd > ctx->socket_fd)
		ctx->min_fd = ctx->gui.x.fd;
#endif	
		
	/* determine max nuber of file descriptors we could have : */
#ifdef _SC_OPEN_MAX
	ctx->fd_width = sysconf (_SC_OPEN_MAX);
#else
	ctx->fd_width = getdtablesize ();
#endif
#ifdef FD_SETSIZE
	if (ctx->fd_width > FD_SETSIZE)
		ctx->fd_width = FD_SETSIZE;
#endif

	ctx->clients = safecalloc (ctx->fd_width, sizeof(AfterShowClient));
	return (ctx->socket_fd > 0);
}

/*************************************************************************** 
 * Window management code : 
 **************************************************************************/

AfterShowXScreen* 
XWindow2screen (AfterShowContext *ctx, Window w)
{
#ifndef X_DISPLAY_MISSING
	int i;
	for (i = 0 ; i < ctx->gui.x.screens_num; ++i)
		if (w == ctx->gui.x.screens[i].selection_window)
			return &(ctx->gui.x.screens[i]);
#endif	
	return NULL;
}

/*************************************************************************** 
 * IO handling code : 
 **************************************************************************/
void HandleXEvents (AfterShowContext *ctx);
void AcceptConnection (AfterShowContext *ctx);
void HandleInput (AfterShowContext *ctx, int client);
void HandleOutput (AfterShowContext *ctx, int client);
void HandleXML (AfterShowContext *ctx, int client);

void 
HandleEvents (AfterShowContext *ctx)
{
	while (ctx->gui.x.valid || ctx->gui.win32.valid)
	{
		fd_set  in_fdset, out_fdset;
		int     retval;
		int 	i = ctx->fd_width;
		/* struct timeval tv; */
		struct timeval *t = NULL;
    	int           max_fd = ctx->min_fd;
		AfterShowClient *clients = ctx->clients;
		LOCAL_DEBUG_OUT( "waiting pipes%s", "");
		FD_ZERO (&in_fdset);
		FD_ZERO (&out_fdset);

#ifndef X_DISPLAY_MISSING
		if (ctx->gui.x.valid && ctx->gui.x.serviced_screens_num)
		{
			FD_SET (ctx->gui.x.fd, &in_fdset);
			XSync (ctx->gui.x.dpy, False);
		}
#endif
		FD_SET(ctx->socket_fd, &in_fdset);
		while (--i >= 0)
			if (clients[i].fd > 0)
			{
				FD_SET(clients[i].fd, &in_fdset);
				if (clients[i].fd > max_fd)
					max_fd = clients[i].fd;
				if (clients[i].out_buf || clients[i].xml_output_head)
					FD_SET(clients[i].fd, &out_fdset);
			}

	    retval = PORTABLE_SELECT(min (max_fd + 1, ctx->fd_width),&in_fdset,&out_fdset,NULL,t);

		if (retval > 0)
		{	
			if (FD_ISSET (ctx->socket_fd, &in_fdset))
				AcceptConnection (ctx);
			
#ifndef X_DISPLAY_MISSING
			if (FD_ISSET (ctx->gui.x.fd, &in_fdset))
				HandleXEvents (ctx);
#endif
			
			for (i = min(max_fd,ctx->fd_width); i >= 0; --i)
			{
				if (FD_ISSET (i, &in_fdset))
					HandleInput (ctx, i);
				if (FD_ISSET (i, &out_fdset))
					HandleOutput (ctx, i);
			}
		}

		for (i = min(max_fd,ctx->fd_width); i >= 0; --i)
			if (clients[i].xml_input_head)
				HandleXML (ctx, i);
	}
}

void 
HandleXEvents (AfterShowContext *ctx)
{
#ifndef X_DISPLAY_MISSING
	int events_count = XEventsQueued (ctx->gui.x.dpy, QueuedAfterReading);
	AfterShowXScreen *scr;
	while (events_count > 0 && ctx->gui.x.valid)
	{
		XEvent e;	
		XNextEvent (ctx->gui.x.dpy, &e);
		--events_count;
		switch (e.type)
		{
			case PropertyNotify: break;
			case SelectionClear:	
				if ((scr = XWindow2screen (ctx, e.xselectionclear.window)) != NULL)
				{
					if (e.xselectionclear.window == scr->selection_window 
						&& e.xselectionclear.selection == scr->_XA_AFTERSHOW_S)
					{ /* we can give up selection if time of the event
					   * after time of us accuring the selection  and WE DON'T HAVE ANY 
					   * ACTIVE CONNECTIONS OF THAT SCREEN! */
						if (e.xselectionclear.time > scr->selection_time 
							&& scr->windows_num <= 0)
						{
							ShutdownXScreen (ctx, scr->screen);
							if (!ctx->gui.x.valid && !ctx->gui.win32.valid)
									Shutdown (ctx);
							continue;
						}
					}
				}
				break;
		} 
	}
#endif	
}

void 
AcceptConnection (AfterShowContext *ctx)
{
	int           fd;
	unsigned int  len = sizeof (struct sockaddr_un);
	struct sockaddr_un name;

	fd = accept (ctx->socket_fd, (struct sockaddr *)&name, &len);

	if (fd < 0 && errno != EWOULDBLOCK)
        show_system_error("error accepting connection");

	/* set non-blocking I/O mode */
	if (fd >= 0)
	{
		if (fcntl (fd, F_SETFL, fcntl (fd, F_GETFL) | O_NONBLOCK) == -1)
		{
            show_system_error("unable to set non-blocking I/O for module socket");
			close (fd);
			fd = -1;
		}
	}
	if (fd >= 0) 
	{
		if (fd < ctx->fd_width)
		{
			ctx->clients[fd].fd = fd;
			ctx->clients[fd].xml_buf = safecalloc( 1, sizeof(ASXmlBuffer));
			show_progress( "accepted new connection with fd %d.", fd );
		}else
        {/* this should never happen, but just in case : */
            show_error("too many connections!");
			close (fd);
            fd = -1 ;
		}
    }
}

/* TODO : this should go into additional file xmlutils.c : */
void 
aftershow_add_tags_to_queue( xml_elem_t* tags, xml_elem_t **phead, xml_elem_t **ptail)
{
	if (*phead == NULL)
		*ptail = *phead = tags;
	else
		(*ptail)->next = tags;
	
	while ((*ptail)->next) *ptail = (*ptail)->next;
}

void 
HandleInput (AfterShowContext *ctx, int channel)
{
	AfterShowClient *client = &(ctx->clients[channel]);

#define READ_BUF_SIZE	4096	
	static char read_buf[READ_BUF_SIZE];
	int bytes_in = read (client->fd, &(read_buf[0]), READ_BUF_SIZE);

	if (bytes_in == 0 && errno != EINTR && errno != EAGAIN)
		ShutdownClient (ctx, channel);
	else
	{
		int i = 0;
		while (i < bytes_in) 
		{
			struct ASXmlBuffer *xb = client->xml_buf;
			while (xb->state >= 0)
			{
				int spooled_count = spool_xml_tag (xb, &read_buf[i], bytes_in - i);
				if (spooled_count <= 0)
					++i;
				else
					i += spooled_count;

				if( xb->state == ASXML_Start && xb->tags_count > 0 && xb->level == 0) 
				{
					xml_elem_t* doc;

					add_xml_buffer_chars( xb, "", 1 );
					LOCAL_DEBUG_OUT("buffer: [%s]", xb->buffer );

					if ((doc = xml_parse_doc(xb->buffer, NULL)) != NULL)
						aftershow_add_tags_to_queue (doc, &(client->xml_input_head), &(client->xml_input_tail));
					reset_xml_buffer( xb );
				}
			}
		}		   
	}
}

void 
HandleOutput (AfterShowContext *ctx, int channel)
{
	AfterShowClient *client = &(ctx->clients[channel]);

}

void 
HandleXML (AfterShowContext *ctx, int channel)
{
	AfterShowClient *client = &(ctx->clients[channel]);

}

/*************************************************************************** 
 * final cleanup code : 
 **************************************************************************/
void 
ShutdownXScreen (AfterShowContext *ctx, int screen)
{
#ifndef X_DISPLAY_MISSING
	if (ctx->gui.x.valid && screen >= 0 && screen < ctx->gui.x.screens_num)
	{
		AfterShowXScreen *scr = &(ctx->gui.x.screens[screen]);
		if (scr->do_service)
		{
			/* close all windows */
			/* unset event masks */
			/* close selection window */
			show_progress( "giving up AfterShow selection on screen %d. This screen is no longer serviced!", screen );
			XSetSelectionOwner( ctx->gui.x.dpy, scr->_XA_AFTERSHOW_S, None, scr->selection_time);
			XDestroyWindow (ctx->gui.x.dpy, scr->selection_window);
			scr->selection_window = None;

			/* destroy the visual */
			destroy_asvisual (scr->asv, False);
			scr->asv = NULL;
			
			/* mark screen as closed */
			scr->do_service = False;
			ctx->gui.x.serviced_screens_num--;
			if (ctx->gui.x.serviced_screens_num <= 0)
			{
				ctx->gui.x.screens_num = 0;
				ctx->gui.x.fd = -1;
				XCloseDisplay (ctx->gui.x.dpy);
				ctx->gui.x.dpy = NULL;
				ctx->gui.x.valid = False;
			}
		}
	}
#endif
}

void 
ShutdownWin32Screen (AfterShowContext *ctx, int screen)
{

}

void 
ShutdownClient (AfterShowContext *ctx, int channel)
{

}


void 
Shutdown (AfterShowContext *ctx)
{
	int i;
	for (i = 0 ; i < ctx->gui.x.screens_num && ctx->gui.x.valid; ++i)
		ShutdownXScreen (ctx, i);

	for (i = 0 ; i < ctx->gui.win32.screens_num && ctx->gui.win32.valid; ++i)
		ShutdownWin32Screen (ctx, i);
}


/* ********************************************************************************/
/* The end !!!! 																 */
/* ********************************************************************************/

