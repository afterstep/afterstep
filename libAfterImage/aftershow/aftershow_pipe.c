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

#define LOCAL_DEBUG

#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#include <errno.h>
#include <sys/file.h>
#include <sys/socket.h>

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
	int cmd_start;
	/* 
	 * 1) Connect to windowing system
	 * 2) Check for another instance running for the same user
	 * 3) Create UD Socket
	 * 4) Handle Events
	 */
	set_application_name(argv[0]);
	set_output_threshold(5);  /* be real quiet by default ! */

	for (cmd_start = 1 ; cmd_start < argc; cmd_start++)
		if (strcmp(argv[cmd_start], "-c") ==0)
			break;
			
	if (!InitContext(&context, argc, argv))
		return EXIT_FAILURE;

	if (!ConnectGUI(&context))
		return EXIT_FAILURE;

	if (!SetupComms(&context))
	{
		show_error( "AfterShow daemon is not available. Exiting!");
		return EXIT_FAILURE;
	}

	if (cmd_start < argc-1)
	{
		dup2 (context.socket_fd, 0);
		dup2 (context.socket_fd, 1);
		// do exec without fork here
	}else
	{	/* we have to manually redirect io */
		HandleEvents(&context);
	}
	close (context.socket_fd);		

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
												ctx->gui.x.screens[0].root.wcv, 
												XInternAtom (ctx->gui.x.dpy, XA_AFTERSHOW_SOCKET_NAME, False));
	} 
#endif
	if (ctx->socket_name == NULL)
		return False;
		
	show_progress ("Socket name set to \"%s\".", ctx->socket_name);
	
	ctx->min_fd = ctx->socket_fd = socket_connect_client (ctx->socket_name);
	fcntl (ctx->socket_fd, F_SETFL, fcntl (ctx->socket_fd, F_GETFL) | O_NONBLOCK);

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
#define IN_BUF_SIZE		16192
#define OUT_BUF_SIZE	1024
	char input_buf[IN_BUF_SIZE];
	int input_size = 0;
	Bool input_eof = False;
	
	while (ctx->gui.x.valid || ctx->gui.win32.valid)
	{
		fd_set  in_fdset, out_fdset;
		int     retval;
		int i;
		FD_ZERO (&in_fdset);
		FD_ZERO (&out_fdset);

		LOCAL_DEBUG_OUT( "input_size = %d", input_size);
		if (input_size > 0)
		{
			FD_SET(ctx->socket_fd, &out_fdset);
		}else
		{
			FD_SET(0, &in_fdset);
		}
			
		FD_SET(ctx->socket_fd, &in_fdset);

		LOCAL_DEBUG_OUT( "waiting pipes%s", "");
	    retval = PORTABLE_SELECT(ctx->socket_fd+1,&in_fdset,&out_fdset,NULL,NULL);
		LOCAL_DEBUG_OUT( "result = %d", retval);

		if (retval > 0)
		{	
			if (FD_ISSET (ctx->socket_fd, &in_fdset))
			{
				char *buf[OUT_BUF_SIZE];
				int bytes_in;
				errno = 0;
				while ((bytes_in = read( ctx->socket_fd, &buf[0], OUT_BUF_SIZE)) > 0)
					write( 1, &(buf[0]), bytes_in);
				if (bytes_in > 0)
					write( 1, "\n", 1);
				if (bytes_in == 0 && errno != EINTR && errno != EAGAIN)
				{
					show_progress( "AfterShow Server has closed the connection. Exiting.");
					return;
				}
				LOCAL_DEBUG_OUT( "bytes_in = %d, errno = %d", bytes_in, errno);				
			}
			if (FD_ISSET (ctx->socket_fd, &out_fdset))
			{
				/* write input buffer out to server */
				int bytes_out;

				errno = 0;
				bytes_out = write (ctx->socket_fd, &(input_buf[0]), input_size);
				LOCAL_DEBUG_OUT("%d bytes sent.", bytes_out);

				if (bytes_out == 0 && errno != EINTR && errno != EAGAIN)
				{
					show_progress( "AfterShow Server has closed the connection. Exiting.");
					return;
				}
				if (bytes_out > 0)
				{
					for (i = 0; i < input_size-bytes_out; ++i)
						input_buf[i] = input_buf[i+bytes_out];
					input_size -= bytes_out;	
					if (input_size == 0 && input_eof)
						return;
				}
			}
			if (FD_ISSET (0, &in_fdset))
			{
				/* fill input buffer from stdin */
				input_size = read( 0, &(input_buf[0]), IN_BUF_SIZE);
				if (input_size == 0) /* EOF */
					return;
				input_eof = (input_buf[input_size-1] == EOF);
				LOCAL_DEBUG_OUT("%d bytes read. eof = %d", input_size, input_eof);
			}
		}

	}
}

/*********************************************************************************
 * The end !!!! 																 
 ********************************************************************************/

