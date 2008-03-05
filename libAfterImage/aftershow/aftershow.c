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

#define LOCAL_DEBUG
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
		return EXIT_FAILURE;

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
	aftershow_connect_x_gui (ctx);
#endif
  /* TODO: add win32 code */

	if (!ctx->gui.x.valid && !ctx->gui.win32.valid)
		return False;

	ctx->imman = create_generic_imageman(NULL);
	ctx->fontman = 
#ifndef X_DISPLAY_MISSING
		create_generic_fontman(ctx->gui.x.dpy, NULL);
#else
		create_generic_fontman(NULL,NULL);
#endif		
	return True;
}

Bool CheckInstance (AfterShowContext *ctx)
{
#ifndef X_DISPLAY_MISSING
	XEvent event;
	int i = ctx->gui.x.screens_num;

	/* must count down to get correct first_screen */	
	while ( --i >= 0 )
	{
		char        tmp[64];
        CARD32      data = 0xAAAAAAAA;
		Window      old_selection_owner = None;
		int 		tick_count;
		AfterShowXScreen *scr = &(ctx->gui.x.screens[i]);
		
		/* intern selection atom */		
		sprintf (tmp, "_AFTERSHOW_S%d", i);
		scr->selection_window = XCreateSimpleWindow (ctx->gui.x.dpy, scr->root.w, 0, 0, 5, 5, 0, 0, None);
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
				ctx->gui.x.first_screen = i;
			}else
			{
				if (scr->selection_window)
				{
					XDestroyWindow (ctx->gui.x.dpy, scr->selection_window);
					scr->selection_window = None;
				}
			}
		}
	}
#endif

	return (ctx->gui.x.serviced_screens_num>0);
}

Bool SetupComms (AfterShowContext *ctx)
{
	char *path = getenv("TMPDIR");
	if (path == NULL)
		path = "/tmp";

	if( access( path, W_OK ) != 0 )
	    if( (path = getenv( "HOME" )) == NULL )
			return False ;

    ctx->socket_name = safemalloc (strlen(path) + 1 + 18 + 32 + 32 + 1);
	sprintf (ctx->socket_name, "%s/aftershow-connect.%d.%d", path, getuid(), ctx->gui.x.first_screen);
	show_progress ("Socket name set to \"%s\".", ctx->socket_name);
	
	ctx->min_fd = ctx->socket_fd = socket_listen (ctx->socket_name);

#ifndef X_DISPLAY_MISSING
	if (ctx->gui.x.valid)
	{
		int i;
		if (ctx->gui.x.fd > ctx->socket_fd)
			ctx->min_fd = ctx->gui.x.fd;
		/* publish socket name as X property */
		for ( i = 0; i < ctx->gui.x.screens_num; ++i)
			if (ctx->gui.x.screens[i].do_service)
				aftershow_set_string_property (	ctx, 
												ctx->gui.x.screens[i].root.w, 
												XInternAtom (ctx->gui.x.dpy, XA_AFTERSHOW_SOCKET_NAME, False), 
												ctx->socket_name);
	}
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
		for ( i = ctx->fd_width; --i >= 0; )
			if (clients[i].fd > 0)
			{
				LOCAL_DEBUG_OUT("setting in_fdset for client %d", clients[i].fd);

				FD_SET(clients[i].fd, &in_fdset);
				if (clients[i].fd > max_fd)
					max_fd = clients[i].fd;
				if (clients[i].xml_output_buf->used > 0 || clients[i].xml_output_head)
					FD_SET(clients[i].fd, &out_fdset);
			}

	    retval = PORTABLE_SELECT(min (max_fd + 1, ctx->fd_width),&in_fdset,&out_fdset,NULL,t);
		LOCAL_DEBUG_OUT("SELECT RETURNED %d", retval);

		if (retval > 0)
		{	
			if (FD_ISSET (ctx->socket_fd, &in_fdset))
				AcceptConnection (ctx);
			
#ifndef X_DISPLAY_MISSING
			if (FD_ISSET (ctx->gui.x.fd, &in_fdset))
				HandleXEvents (ctx);
#endif
			
			for (i = min(max_fd,ctx->fd_width); i >= 0; --i)
				if (i != ctx->socket_fd)
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
	AfterShowXWindow *window;
	while (events_count > 0 && ctx->gui.x.valid)
	{
		XEvent e;	
		XNextEvent (ctx->gui.x.dpy, &e);
		--events_count;
		switch (e.type)
		{
			case Expose :
				if ((window = aftershow_XWindowID2XWindow (ctx, e.xexpose.window)) != NULL)
				{
					/* TODO: aggregate Expose events to a bigger area ! */
					aftershow_ExposeXWindowArea (ctx, window, e.xexpose.x, e.xexpose.y,
													e.xexpose.width + e.xexpose.x, 
													e.xexpose.height + e.xexpose.y);
				}
				break;
			case PropertyNotify: break;
			case SelectionClear:	
				if ((scr = aftershow_XWindowID2XScreen (ctx, e.xselectionclear.window)) != NULL)
				{
					if (e.xselectionclear.window == scr->selection_window 
						&& e.xselectionclear.selection == scr->_XA_AFTERSHOW_S)
					{ /* we can give up selection if time of the event
					   * after time of us accuring the selection  and WE DON'T HAVE ANY 
					   * ACTIVE CONNECTIONS OF THAT SCREEN! */
						if (e.xselectionclear.time > scr->selection_time 
							&& scr->windows->items_num <= 0)
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
			ctx->clients[fd].xml_input_buf = safecalloc( 1, sizeof(ASXmlBuffer));
			ctx->clients[fd].xml_output_buf = safecalloc( 1, sizeof(ASXmlBuffer));
			show_progress( "accepted new connection with fd %d.", fd );
		}else
        {/* this should never happen, but just in case : */
            show_error("too many connections!");
			close (fd);
            fd = -1 ;
		}
    }
}

void 
HandleInput (AfterShowContext *ctx, int channel)
{
	AfterShowClient *client = &(ctx->clients[channel]);

#define READ_BUF_SIZE	4096	
	static char read_buf[READ_BUF_SIZE];
	int bytes_in;
	
	errno = 0;
	bytes_in = read (client->fd, &(read_buf[0]), READ_BUF_SIZE);

	LOCAL_DEBUG_OUT ("%d bytes read from client %d, errno = %d", bytes_in, client->fd, errno);

	if (bytes_in == 0)
	{
		if (errno != EINTR && errno != EAGAIN)
			ShutdownClient (ctx, channel);
	}else
	{
		int i = 0;
		while (i < bytes_in) 
		{
			struct ASXmlBuffer *xb = client->xml_input_buf;
			LOCAL_DEBUG_OUT ("i = %d, state = %d", i, xb->state);

			while (xb->state >= 0 && i < bytes_in)
			{
				int spooled_count = spool_xml_tag (xb, &read_buf[i], bytes_in - i);
				LOCAL_DEBUG_OUT ("i = %d, spooled_count = %d\n", i, spooled_count);
				if (spooled_count <= 0)
					++i;
				else
					i += spooled_count;

				if( xb->state == ASXML_Start && xb->tags_count > 0 && xb->level == 0) 
				{
					xml_elem_t* doc;

					add_xml_buffer_chars( xb, "", 1 );
					LOCAL_DEBUG_OUT("buffer: [%s]", xb->buffer );

					if ((doc = aftershow_parse_xml_doc(xb->buffer)) != NULL)
						aftershow_add_tags_to_queue (doc, &(client->xml_input_head), &(client->xml_input_tail));
					reset_xml_buffer( xb );
				}
			}
			if (xb->state < 0)
			{
				xml_elem_t* err_xml = format_xml_buffer_state (xb);
				aftershow_add_tags_to_queue (err_xml, &(client->xml_output_head), &(client->xml_output_tail));
				reset_xml_buffer (xb);
			}
		}		   
	}
}

void 
HandleOutput (AfterShowContext *ctx, int channel)
{
	AfterShowClient *client = &(ctx->clients[channel]);
	ASXmlBuffer *xb = client->xml_output_buf;
	if (client->fd <= 0 || xb == NULL)
		return;
	while (client->xml_output_head || xb->used > 0)
	{
		xml_elem_t *tmp = client->xml_output_head;
		if (xb->used > xb->current)
		{
			int bytes_out;

			errno = 0;
			bytes_out = write (client->fd, &(xb->buffer[xb->current]), xb->used - xb->current);
			if (bytes_out == 0)
			{
				if (errno != EINTR && errno != EAGAIN)
					ShutdownClient (ctx, channel);
				return;
			}
			xb->current += bytes_out;
			if (xb->used > xb->current)
				return; /* can't write anymore - need to select to wait for data to be processed! */

			reset_xml_buffer (xb);
		}
		if (tmp)
		{
			xml_tags2xml_buffer( tmp, xb, 1/* one tag at a time */, 
								 -1/* no formatting with identing */ );
			client->xml_output_head = tmp->next;
			if (tmp->next == NULL || tmp == client->xml_output_tail)	
				client->xml_output_tail = NULL;

			tmp->next = NULL;
			xml_elem_delete (NULL, tmp);		
		}
	}
	
}

/* The XML has to be in the following format : 
 
 <window id="window_id" [default=0|1] [screen="screen_no"] [geometry="geom_string] [parent="parent_window_id"]>
   	possibly 0 or more child <window> tags 
	<layer id="layer_id" x="x" y="y" width="width" height="height">
		single image or composite tag
	</layer>
   	possibly 0 or more overlaping <Layer> tags 
 </window>
 
 If encompasing <window> tag is missing - default client's window is assumed.
 If encompasing <layer> tag is missing - first layer of the window is assumed.
 

*/

typedef struct AfterShowTagParams
{
	enum {AfterShowTag_Unknown = 0, AfterShowTag_Window, AfterShowTag_Layer, AfterShowTag_Image} type;
	char *id;
	int x, y, width, height;
	char *window_id;
	char *layer_id;
}AfterShowTagParams;

typedef struct AfterShowTagContext
{
	AfterShowClient 	*client
	
	AfterShowMagicPtr 	*window;
	AfterShowMagicPtr 	*layer;
}AfterShowTagContext;

void ParseTagParams (AfterShowContext *ctx, int channel, xml_elem_t *tag, AfterShowTagParams *params);
xml_elem_t *HandleWindowTag (AfterShowContext *ctx, AfterShowTagContext *tag_ctx, xml_elem_t *window_tag, xml_elem_t *child_tag);
xml_elem_t *HandleLayerTag (AfterShowContext *ctx, int channel, xml_elem_t *tag, AfterShowTagParams *params);
xml_elem_t *HandleImageTag (AfterShowContext *ctx, int channel, xml_elem_t *tag, AfterShowTagParams *params);

void 
HandleXML (AfterShowContext *ctx, int channel)
{
	AfterShowClient *client = &(ctx->clients[channel]);

	/* the fun part ! */
	while (client->xml_input_head)
	{
		xml_elem_t *container, *tag;
		xml_elem_t *result = NULL;

		container = tag = client->xml_output_head;
		/* remove tag from the input queue */

		client->xml_input_head = tag->next;
		if (tag->next == NULL || tag == client->xml_input_tail)	
			client->xml_input_tail = NULL;
		tag->next = NULL;

		/* now lets handle the tag ! */
		while (tag && tag->tag_id == XML_CONTAINER_ID)	tag = tag->child;
		
		if (tag)
		{
			AfterShowTagContext tag_ctx ; 
			memset( &tag_ctx, 0x00, sizeof(tag_ctx));
			tag_ctx.client = client;
			
			if (tag->tag_id != AfterShow_window_ID)
				result = HandleWindowTag( ctx, &tag_ctx, NULL, tag);
			else
				result = HandleWindowTag( ctx, &tag_ctx, tag, tag->child);
		
			if (result != NULL && client->fd > 0)
				aftershow_add_tags_to_queue (result, &(client->xml_output_head), &(client->xml_output_tail));
		}
		
		/* delete the tag */
		xml_elem_delete (NULL, container);
	}

}

ASMagic *client_id2object(AfterShowContext *ctx, int channel, const char *id)
{
	/* TODO : implement object search by name */
	return NULL;
}

void ParseTagParams (AfterShowContext *ctx, int channel, xml_elem_t *tag, AfterShowTagParams *params)
{
	xml_elem_t *tmp, *parm = xml_parse_parm(tag->parm, NULL);
	memset (params, 0x00, sizeof(AfterShowTagParams));
	if (parm)
	{
		char *x_str = NULL, *y_str = NULL, *width_str = NULL, *height_str = NULL;
		char *ref_id = NULL;
		int ref_width = 0, ref_height = 0;
		AfterShowMagicPtr ref_obj;
		
		if (strcmp(tag->tag, "window") == 0)
			params->type = AfterShowTag_Window;
		else if (strcmp(tag->tag, "layer") == 0)
			params->type = AfterShowTag_Layer;
		else
			params->type = AfterShowTag_Image;
						
		for (tmp = parm ; tmp ; tmp = tmp->next)
		{
			if (!strcmp(tmp->tag, "x")) 			x_str = tmp->parm;
			else if (!strcmp(tmp->tag, "y")) 		y_str = tmp->parm;
			else if (!strcmp(tmp->tag, "width")) 	width_str = tmp->parm;
			else if (!strcmp(tmp->tag, "height")) 	height_str = tmp->parm;
			else if (!strcmp(tmp->tag, "refid")) 	ref_obj.magic = client_id2object(ctx, channel, tmp->parm);
			else if (!strcmp(tmp->tag, "wid")) 		params->window_id = strdup(tmp->parm);
			else if (!strcmp(tmp->tag, "lid")) 		params->layer_id = strdup(tmp->parm);
			else if (!strcmp(tmp->tag, "id")) 		params->id = strdup(tmp->parm);
		}
		
		if (ref_obj.magic)
		{
			if (ref_obj.magic->magic == MAGIC_ASIMAGE)
			{
				ref_width = ref_obj.asim->width;
				ref_height = ref_obj.asim->height;
			}else if (ref_obj.magic->magic == MAGIC_AFTERSHOW_X_WINDOW)
			{
				ref_width = ref_obj.xwindow->width;
				ref_height = ref_obj.xwindow->height;
			}else if (ref_obj.magic->magic == MAGIC_AFTERSHOW_X_LAYER)
			{
				ref_width = ref_obj.xlayer->width;
				ref_height = ref_obj.xlayer->height;
			}else
			{
				
			}
		}
		if (x_str)	params->x = parse_math (x_str, NULL, ref_width);
		if (y_str)	params->y = parse_math (y_str, NULL, ref_width);
		if (width_str)	params->width = parse_math (width_str, NULL, ref_width);
		if (height_str)	params->height = parse_math (height_str, NULL, ref_width);
		
		xml_elem_delete(NULL, parm);
	}
}

xml_elem_t *
xml_elem_t *HandleWindowTag (AfterShowContext *ctx, AfterShowTagContext *tag_ctx, xml_elem_t *window_tag, xml_elem_t *child_tag);
{
	xml_elem_t *result = NULL;
	if (window_tag == NULL)
	{/* find or create the default window for the client */
		
	}else
	{
	
	}
	
	return result;
}

xml_elem_t *
HandleLayerTag (AfterShowContext *ctx, int channel, xml_elem_t *tag, AfterShowTagParams *params)
{
	xml_elem_t *result = NULL;
	AfterShowClient *client = &(ctx->clients[channel]);

	return result;
}

static inline AfterShowXScreen *GetWindowScreen (AfterShowXWindow *window) { return window->screen; };
static inline AfterShowClient *GetClient (AfterShowContext *ctx, int channel)
{
	if (ctx->clients[channel].fd == 0)
		return NULL;
	return &(ctx->clients[channel]);
}

AfterShowXWindow *
GetWindowForClient(AfterShowContext *ctx, AfterShowClient *client, 
					const char *id, int default_width, int default_height)
{
#ifndef X_DISPLAY_MISSING
	AfterShowXWindow *window = NULL;
	
	if (client->default_window.magic)
		if (client->default_window.magic->magic == MAGIC_AFTERSHOW_X_WINDOW)
			window = client->default_window.xwindow;
	if (id)
	{
		/* try and fetch window from the list by name */
		
	}
	if (window == NULL)
	{
		AfterShowXScreen *scr = &(ctx->gui.x.screens[client->default_screen]);
		window = aftershow_create_x_window (ctx, &(scr->root), default_width, default_height);
		XMapWindow( ctx->gui.x.dpy, window->w);
		
		if (id)
		{
			/* store window for future reference under the id */
		}
		if (client->default_window.xwindow == NULL)
			client->default_window.xwindow = window;
	}
	return window;
#else	
	return NULL;
#endif
}

AfterShowXLayer *
GetLayerForClientWindow(AfterShowContext *ctx, AfterShowClient *client, 
                        AfterShowXWindow *window, const char *id)
{
#ifndef X_DISPLAY_MISSING
	AfterShowXLayer *layer = NULL;
	
	if (client->default_layer.magic)
		if (client->default_layer.magic->magic == MAGIC_AFTERSHOW_X_LAYER)
			layer = client->default_layer.xlayer;
			
	if (id)
	{
		/* try and fetch layer from the list by name */
		
	}
	if (layer == NULL)
	{
		layer = aftershow_create_x_layer (ctx, window);
		if (id)
		{
			/* store layer for future reference under the id */
		}
		if (client->default_layer.xlayer == NULL)
			client->default_layer.xlayer = layer;
	}
	
	return layer;
#else	
	return NULL;
#endif
}


void RenderImage (AfterShowContext *ctx, AfterShowClient *client, ASImage *im, AfterShowTagParams *params)
{
#ifndef X_DISPLAY_MISSING
	AfterShowXWindow *window = GetWindowForClient(ctx, client, params->window_id, im->width, im->height);
	if (window != NULL)
	{
		AfterShowXLayer *layer = GetLayerForClientWindow(ctx, client, window, params->layer_id);
		if (layer != NULL)
		{
			aftershow_ASImage2XLayer (ctx, window, layer, im, params->x, params->y);
			/* if layer is the only one or synchronous rendering - blend it onto the window */
			XClearArea( ctx->gui.x.dpy, window->w, 0, 0, 0, 0, True);
		}
	}
#endif
}

xml_elem_t *
HandleImageTag (AfterShowContext *ctx, int channel, xml_elem_t *tag, AfterShowTagParams *params)
{
	AfterShowClient *client = GetClient(ctx, channel);
	xml_elem_t *result = NULL;
	ASImage *im = NULL;

	if (client == NULL)
		return NULL;
	
	im = compose_asimage_xml_from_doc(asv, client->imman, client->fontman, tag, ASFLAGS_EVERYTHING, False, None, NULL, -1, -1);

	if( im ) 
	{
		/* Display the image. */
		RenderImage (ctx, client, im, params);

		safe_asimage_destroy(im);
	}					
	/* printf("<success tag_count=%d/>\n", xb.tags_count ); */

	return result;
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
	AfterShowClient *client = &(ctx->clients[channel]);

	if (client->fd)
	{
		show_progress( "closing connection to client with fd %d.", client->fd );
		close (client->fd);
	}

	if (client->xml_input_buf)
	{
		free_xml_buffer_resources (client->xml_input_buf);
		free (client->xml_input_buf);
	}

	if (client->xml_output_buf)
	{
		free_xml_buffer_resources (client->xml_output_buf);
		free (client->xml_output_buf);
	}
	
	memset( client, 0x00, sizeof(AfterShowClient));		
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

