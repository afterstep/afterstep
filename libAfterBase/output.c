/****************************************************************************
 *
 * Copyright (c) 1999 Sasha Vasko <sashav@sprintmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 ****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <signal.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "config.h"
#include "astypes.h"
#include "output.h"
#include "ashash.h"
#include "parse.h"

char         *ApplicationName = NULL;                    /* name are we known by */


static stream_func  as_default_stream_func = (stream_func)fprintf;
static void        *as_default_stream_stream = NULL ;
static unsigned int as_output_threshold = OUTPUT_DEFAULT_THRESHOLD ;
static unsigned int as_output_prev_level = OUTPUT_DEFAULT_THRESHOLD ;
static unsigned int as_output_curr_level = OUTPUT_DEFAULT_THRESHOLD ;

void
set_application_name (char *argv0)
{
	char         *temp = strrchr (argv0, '/');

	/* Save our program name - for error messages */
	ApplicationName = temp ? temp + 1 : argv0;
}

const char *
get_application_name()
{
	return ApplicationName;
}

unsigned int
set_output_threshold( unsigned int threshold )
{
  unsigned int old = as_output_threshold;
    as_output_threshold = threshold ;
    if( threshold > 0 )
        as_default_stream_stream = stderr ;
    return old;
}

unsigned int
get_output_threshold()
{
  return as_output_threshold ;
}


unsigned int
set_output_level( unsigned int level )
{
  unsigned int old = as_output_curr_level;
    if( as_output_curr_level != level && level  > OUTPUT_LEVEL_INVALID)
    {
        as_output_prev_level = as_output_curr_level ;
        as_output_curr_level = level ;
    }
    return old;
}

void
restore_output_level()
{
    as_output_curr_level = as_output_prev_level;
}

Bool
is_output_level_under_threshold( unsigned int level )
{
    if( level == OUTPUT_LEVEL_INVALID )
        return (as_output_curr_level <= as_output_threshold);
    else
        return (level <= as_output_threshold);
}

Bool
show_error( const char *error_format, ...)
{
    if( OUTPUT_LEVEL_ERROR <= as_output_threshold)
    {
        va_list ap;
        fprintf (stderr, "%s ERROR: ", ApplicationName );
        va_start (ap, error_format);
        vfprintf (stderr, error_format, ap);
        va_end (ap);
        fprintf (stderr, "\n" );
        return True;
    }
    return False;
}

Bool
show_system_error( const char *error_format, ...)
{
    if( OUTPUT_LEVEL_ERROR <= as_output_threshold)
    {
        va_list ap;
        fprintf (stderr, "%s SYSTEM ERROR: ", ApplicationName );
        va_start (ap, error_format);
        vfprintf (stderr, error_format, ap);
        va_end (ap);
        fprintf (stderr, ".\n\tSystem error text" );
        perror("");
        return True;
    }
    return False;
}

Bool
show_warning( const char *warning_format, ...)
{
    if( OUTPUT_LEVEL_WARNING <= as_output_threshold)
    {
        va_list ap;
        fprintf (stderr, "%s warning: ", ApplicationName );
        va_start (ap, warning_format);
        vfprintf (stderr, warning_format, ap);
        va_end (ap);
        fprintf (stderr, "\n" );
        return True;
    }
    return False;
}

Bool
show_progress( const char *msg_format, ...)
{
    if( OUTPUT_LEVEL_PROGRESS <= as_output_threshold)
    {
        va_list ap;
        fprintf (stderr, "%s : ", ApplicationName );
        va_start (ap, msg_format);
        vfprintf (stderr, msg_format, ap);
        va_end (ap);
        fprintf (stderr, "\n" );
        return True;
    }
    return False;
}

Bool
show_debug( const char *file, const char *func, int line, const char *msg_format, ...)
{
    if( OUTPUT_LEVEL_DEBUG <= as_output_threshold)
    {
        va_list ap;
        fprintf (stderr, "%s debug msg: %s:%s():%d: ", ApplicationName, file, func, line );
        va_start (ap, msg_format);
        vfprintf (stderr, msg_format, ap);
        va_end (ap);
        fprintf (stderr, "\n" );
        return True;
    }
    return False;
}


inline void
nonGNUC_debugout_stub( const char *format, ...)
{}

void
nonGNUC_debugout( const char *format, ...)
{
    va_list ap;
    fprintf (stderr, "%s: ", ApplicationName );
    va_start (ap, format);
    vfprintf (stderr, format, ap);
    va_end (ap);
    fprintf (stderr, "\n" );
}

Bool
pre_print_check( stream_func *pfunc, void** pstream, void *data, const char *msg )
{

    if( *pfunc == NULL )
    {
        if( as_output_curr_level <= as_output_threshold )
            *pfunc = as_default_stream_func ;

        if( *pfunc == NULL )
            return False ;
    }

    if( *pstream == NULL )
        *pstream = as_default_stream_stream ;

    if( data == NULL && msg != NULL)
        (*pfunc)( *pstream, "ERROR=\"%s\"\n", msg);

    return (data != NULL);
}


