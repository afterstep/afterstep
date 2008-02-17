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

/*********************************************************************************
 * Utility functions to handle X stuff
 *********************************************************************************/

#ifdef _WIN32
#include "../win32/config.h"
#else
#include "../config.h"
#endif

#undef LOCAL_DEBUG

#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include "../afterbase.h"
#include "../afterimage.h"
#include "aftershow.h"

Bool InitContext (AfterShowContext *context, int argc, char **argv);
Bool ConnectGUI (AfterShowContext *context);
Bool SetupComms (AfterShowContext *context);
void HandleEvents (AfterShowContext *context);
void Shutdown (AfterShowContext *ctx);

void show_usage (Bool short_form)
{
	printf( "AfterShow Pipe - tool to redirect stdin and stdout to and from AfterShow - \n"
			"                 AfterImage XML processing and display daemon.\n"
			"Usage: aftershow_pipe [-h|--help] [-d|--display display_string] [-V level]\n"
			"  -h --help      	- display this message;\n"
			"  -d --display display_string - connect to X display specified by display_string;\n"
			"  -V level     	- set output verbosity level - 0 for no output;\n" );
}

int main (int argc, char **argv)
{
	AfterShowContext context;
	/* 
	 * 1) Connect to windowing system
	 * 2) Check for another instance running for the same user
	 * 3) Create UD Socket
	 * 4) Handle Events
	 */
	set_application_name(argv[0]);
	set_output_threshold(5);  /* be real quiet by default ! */
	
	if (!InitContext(&context, argc, argv))
		return EXIT_FAILURE;

	if (!ConnectGUI(&context))
		return EXIT_FAILURE;

	if (!SetupComms(&context))
	{
		show_error( "AfterShow daemon is not available. Exiting!");
		return EXIT_FAILURE;
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
	set_flags(ctx->flags, AfterShow_SingleScreen); /* single screen always ! */
	
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
						case 'V': 	if (i + 1 < argc)
										set_output_threshold(atoi(argv[++i]));
									break;
						case 'd': 	if (i + 1 < argc)
									{
										if (ctx->display) 
											free (ctx->display);
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
				}else if (strcmp(&(argv[i][2]), "display") == 0)
				{ 
					if (i + 1 < argc)
					{
						if (ctx->display) 
							free (ctx->display);
						ctx->display = strdup(argv[++i]);
					}				
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

	return 	(ctx->gui.x.valid || ctx->gui.win32.valid);
}

Bool SetupComms (AfterShowContext *ctx)
{
#ifndef X_DISPLAY_MISSING
 	if (ctx->gui.x.valid)
	{
		/* get socket name from X property */
	    ctx->socket_name = aftershow_read_string_property (ctx, 
												ctx->gui.x.screens[0].root, 
												XInternAtom (ctx->gui.x.dpy, XA_AFTERSHOW_SOCKET_NAME, False));
	} 
#endif
	if (ctx->socket_name == NULL)
		return False;
		
	show_progress ("Socket name set to \"%s\".", ctx->socket_name);
	
	ctx->min_fd = ctx->socket_fd = socket_connect_client (ctx->socket_name);

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

void 
HandleEvents (AfterShowContext *ctx)
{
	while (ctx->gui.x.valid || ctx->gui.win32.valid)
	{
		fd_set  in_fdset, out_fdset;
		int     retval;
		/* struct timeval tv; */
		struct timeval *t = NULL;
    	int           max_fd = ctx->min_fd;
		LOCAL_DEBUG_OUT( "waiting pipes%s", "");
		FD_ZERO (&in_fdset);
		FD_ZERO (&out_fdset);

		FD_SET(ctx->socket_fd, &in_fdset);

	    retval = PORTABLE_SELECT(min (max_fd + 1, ctx->fd_width),&in_fdset,&out_fdset,NULL,t);

		if (retval > 0)
		{	
		}

	}
}


/*********************************************************************************
 * The end !!!! 																 
 ********************************************************************************/

