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

#ifdef _WIN32
# include "../win32/afterbase.h"
#else
# include "../afterbase.h"
#endif
#include "../afterimage.h"

/**********************************************************************************/
/* main portion of AfterShow daemon                                               */
/**********************************************************************************/
#ifndef STDIN_FILENO
# define STDIN_FILENO   0
# define STDOUT_FILENO  1
# define STDERR_FILENO  2
#endif

#if !defined (EACCESS) && defined(EAGAIN)
# define EACCESS EAGAIN
#endif

#ifndef EXIT_SUCCESS            /* missing from <stdlib.h> */
# define EXIT_SUCCESS           0       /* exit function success */
# define EXIT_FAILURE           1       /* exit function failure */
#endif


typedef struct AfterShowContext
{
#define AfterShow_DoFork	(0x01<<0)
	ASFlagType flags;
	
}AfterShowContext;

Bool InitContext (AfterShowContext *context, int argc, char **argv);
Bool ConnectGUI (AfterShowContext *context);
Bool CheckInstance (AfterShowContext *context);
Bool SetupComms (AfterShowContext *context);
void HandleEvents (AfterShowContext *context);

void show_usage (Bool short_form)
{
	

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
Bool InitContext (AfterShowContext *context, int argc, char **argv)
{
	int i;
	for (i = 1 ; i < argc ; ++i) 
	{
		if (argv[i][0] == '-')
		{
			if (argv[i][1] != '-') /* short option */
			{
				if (argv[i][2] == '\0')
					switch (argv[i][1])
					{
						case 'h': show_usage(False); return False;
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

Bool ConnectGUI (AfterShowContext *context)
{

	return True;

}

Bool CheckInstance (AfterShowContext *context)
{

	return True;

}

Bool SetupComms (AfterShowContext *context)
{

	return True;
}

void HandleEvents (AfterShowContext *context)
{

}

/* ********************************************************************************/
/* The end !!!! 																 */
/* ********************************************************************************/

