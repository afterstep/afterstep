/*
 * Copyright (C) 1999 Sasha Vasko   <sasha at aftercode.net>
 * Copyright (C) 1999 Ethan Fischer <allanon@crystaltokyo.com>
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
#define LOCAL_DEBUG

#include "../configure.h"
#include "asapp.h"
#include <signal.h>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
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
#include "../libAfterImage/afterimage.h"
#include "screen.h"
#include "module.h"
#include "wmprops.h"
#include "session.h"
#include "colorscheme.h"

#define  ASSocketWriteInt32(sb,d,i)  socket_buffered_write( (sb), (d), (i)*sizeof(CARD32))
#define  ASSocketWriteInt16(sb,d,i)  socket_buffered_write( (sb), (d), (i)*sizeof(CARD16))

void
as_socket_write_string (ASSocketBuffer *sb, const char *string)
{
	if (sb && sb->fd >= 0)
	{
		CARD32        len = 0;

		if (string != NULL)
			len = strlen (string);
        ASSocketWriteInt32 (sb, &len, 1);
		if (len)
			socket_buffered_write (sb, string, len);
	}
}

#define ASSocketWriteString(sb,s) as_socket_write_string ((sb),(s))

/***********************************************************************
 * Sending data to the AfterStep :
 **********************************************************************/
static ASSocketBuffer  as_module_out_buffer = { -1, 0, {0}};

#define AS_MODULE_MSG_PROTO_PARTS   2
static ASProtocolItemSpec as_module_msg_parts[AS_MODULE_MSG_PROTO_PARTS] =
{
	{AS_PROTOCOL_ITEM_INT32, 3},               /* the header */
	{AS_PROTOCOL_ITEM_BYTE, 0/*we'll update it based on the header */}, /* the body */
};

static ASProtocolSpec as_module_msg_proto =
{
	&(as_module_msg_parts[0]),
	AS_MODULE_MSG_PROTO_PARTS,
	180 /* timeout in sec */
};

static ASProtocolItem  as_module_msg_items[AS_MODULE_MSG_PROTO_PARTS] ;
static ASProtocolState as_module_msg_state = {&as_module_msg_proto, &(as_module_msg_items[0]), 0, 0, 0};

void
set_module_out_fd( int fd )
{
	as_module_out_buffer.fd = fd ;
	as_module_out_buffer.bytes_in = 0 ;        /* sort of discarding buffer */
}

void
set_module_in_fd( int fd )
{
	as_module_msg_state.fd = fd ;
	socket_read_proto_reset( &as_module_msg_state );
}

int
get_module_out_fd()
{
	return as_module_out_buffer.fd;
}
int
get_module_in_fd()
{
	return as_module_msg_state.fd;
}


static inline void
send_module_msg_header (Window w, CARD32 bytes)
{
    CARD32 w32 = w ;
    ASSocketWriteInt32 ( &as_module_out_buffer, &w32, 1 );
    ASSocketWriteInt32 ( &as_module_out_buffer, &bytes, 1 );
}

static inline void
send_module_msg_tail ()
{
	CARD32           cont = F_FUNCTIONS_NUM;

    ASSocketWriteInt32 ( &as_module_out_buffer, &cont, 1 );
	socket_write_flush ( &as_module_out_buffer );
}

static inline void
send_module_msg_raw ( void *data, size_t bytes )
{
	socket_buffered_write(&as_module_out_buffer, data, bytes);
}

static inline void
send_module_msg_function (CARD32 func,
						  const char *name, const char *text, const long *func_val, const long *unit_val)
{
	CARD32        spare_func_val[2] = { 0, 0 };
	CARD32        spare_unit_val[2] = { 100, 100 };

    ASSocketWriteInt32 (&as_module_out_buffer, &func, 1 );
    ASSocketWriteString(&as_module_out_buffer , name);
    ASSocketWriteString(&as_module_out_buffer , text);
	if (func_val != NULL)
	{
		spare_func_val[0] = func_val[0] ;
		spare_func_val[1] = func_val[1] ;
	}
	if (unit_val != NULL)
	{
		spare_unit_val[0] = unit_val[0] ;
		spare_unit_val[1] = unit_val[1] ;
	}
    ASSocketWriteInt32 (&as_module_out_buffer, &(spare_func_val[0]), 2);
    ASSocketWriteInt32 (&as_module_out_buffer, &(spare_unit_val[0]), 2);
}

/***********************************************************************
 *  High level function for message delivery to AfterStep :
 ***********************************************************************/
#if 0   /* old version of SendInfo : */
void
SendInfo (int *fd, char *message, unsigned long window)
{
	size_t        w;
LOCAL_DEBUG_OUT( "message to afterstep:\"%s\"", message );
	if (message != NULL)
	{
		write (fd[0], &window, sizeof (unsigned long));
		w = strlen (message);
		write (fd[0], &w, sizeof (int));
		write (fd[0], message, w);

		/* keep going */
		w = 1;
		write (fd[0], &w, sizeof (int));
	}
}
#else
void
SendInfo ( char *message, unsigned long window)
{
	size_t        len;
LOCAL_DEBUG_OUT( "message to afterstep:\"%s\"", message );
    if (message != NULL)
	{
		if ((len = strlen (message)) > 0)
		{
			send_module_msg_header(window, len);
			send_module_msg_raw(message, len);
			send_module_msg_tail ();
		}
	}
}
#endif

/* SendCommand - send a preparsed AfterStep command : */
void
SendCommand( FunctionData * pfunc, unsigned long window)
{
LOCAL_DEBUG_OUT( "sending command %p to the astep", pfunc );
	if (pfunc != NULL)
	{
		send_module_msg_header(window, 0);
        send_module_msg_function(pfunc->func, pfunc->name, pfunc->text, pfunc->func_val, pfunc->unit_val);
		send_module_msg_tail ();
	}
}

void
SendTextCommand ( int func, const char *name, const char *text, unsigned long window)
{
	long          dummy_val[2] = { 0, 0 };

	if (IsValidFunc (func))
	{
		send_module_msg_header(window, 0);
		send_module_msg_function(func, name, text, dummy_val, dummy_val);
		send_module_msg_tail ();
	}
}

void
SendNumCommand ( int func, const char *name, const long *func_val, const long *unit_val, unsigned long window)
{
	if (IsValidFunc (func))
	{
		send_module_msg_header(window, 0);
		send_module_msg_function(func, name, NULL, func_val, unit_val);
		send_module_msg_tail ();
	}
}

/************************************************************************
 *
 * Reads a single packet of info from AfterStep. Prototype is:
 * unsigned long header[3];
 * unsigned long *body;
 * int fd[2];
 * void DeadPipe(int nonsense);
 *  (Called if the pipe is no longer open )
 *
 * ReadASPacket(fd[1],header, &body);
 *
 * Returns:
 *   > 0 everything is OK.
 *   = 0 invalid packet.
 *   < 0 pipe is dead. (Should never occur)
 *
 **************************************************************************/
int
ReadASPacket (int fd, unsigned long *header, unsigned long **body)
{
	int           count, count2;
	size_t        bytes_to_read;
	int           bytes_in = 0;
	char         *cbody;

	bytes_to_read = 3 * sizeof (unsigned long);
	cbody = (char *)header;
	do
	{
		count = read (fd, &cbody[bytes_in], bytes_to_read);
		if (count == 0 ||					   /* dead pipe (EOF) */
			(count < 0 && errno != EINTR))	   /* not a signal interuption */
		{
			DeadPipe (1);
			return -1;
		}
		if (count > 0)
		{
			bytes_to_read -= count;
			bytes_in += count;
		}
	}
	while (bytes_to_read > 0);

	if (header[0] == START_FLAG)
	{
		bytes_to_read = (header[2] - 3) * sizeof (unsigned long);
		if ((*body = (unsigned long *)safemalloc (bytes_to_read)) == NULL)	/* not enough memory */
			return 0;

		cbody = (char *)(*body);
		bytes_in = 0;

		while (bytes_to_read > 0)
		{
			count2 = read (fd, &cbody[bytes_in], bytes_to_read);
			if (count2 == 0 ||				   /* dead pipe (EOF) */
				(count2 < 0 && errno != EINTR))	/* not a signal interuption */
			{
				DeadPipe (1);
				return -1;
			}
			if (count2 > 0)
			{
				bytes_to_read -= count2;
				bytes_in += count2;
			}
		}
	} else
		count = 0;
	return count;
}


int           GetFdWidth (void);

ASMessage    *
CheckASMessageFine (int t_sec, int t_usec)
{
	fd_set        in_fdset;
	ASMessage    *msg = NULL;
	struct timeval tv;
    int           fd = get_module_in_fd();

	FD_ZERO (&in_fdset);
	FD_SET (fd, &in_fdset);
	tv.tv_sec = t_sec;
	tv.tv_usec = t_usec;
#ifdef __hpux
	while (select (fd + 1, (int *)&in_fdset, 0, 0, (t_sec < 0) ? NULL : &tv) == -1)
		if (errno != EINTR)
			break;
#else
	while (select (fd + 1, &in_fdset, 0, 0, (t_sec < 0) ? NULL : &tv) == -1)
		if (errno != EINTR)
			break;
#endif
	if (FD_ISSET (fd, &in_fdset))
	{
		msg = (ASMessage *) safemalloc (sizeof (ASMessage));
		if (ReadASPacket (fd, msg->header, &(msg->body)) <= 0)
		{
			free (msg);
			msg = NULL;
		}
	}

	return msg;
}

void
DestroyASMessage (ASMessage * msg)
{
	if (msg)
	{
		if (msg->body)
			free (msg->body);
		free (msg);
	}
}


void
module_wait_pipes_input ( void (*as_msg_handler) (unsigned long type, unsigned long *body) )
{
    fd_set        in_fdset, out_fdset;
	int           retval;
	struct timeval tv;
	struct timeval *t = NULL;
    int           max_fd = 0;
    ASMessage     msg;
    int as_fd = get_module_in_fd();

	FD_ZERO (&in_fdset);
	FD_ZERO (&out_fdset);

	FD_SET (x_fd, &in_fdset);
    max_fd = x_fd ;

    if (as_fd >= 0)
    {
        FD_SET (as_fd, &in_fdset);
        if (max_fd < as_fd)
            max_fd = as_fd;
    }

    if (timer_delay_till_next_alarm ((time_t *) & tv.tv_sec, (time_t *) & tv.tv_usec))
        t = &tv;

    retval = PORTABLE_SELECT(min (max_fd + 1, fd_width),&in_fdset,&out_fdset,NULL,t);

	if (retval > 0)
	{
        /* check for incoming module connections */
        if (as_fd >= 0)
            if (FD_ISSET (as_fd, &in_fdset))
                if (ReadASPacket (as_fd, msg.header, &(msg.body)) > 0)
                {
                    as_msg_handler (msg.header[1], msg.body);
                    free (msg.body);
                }
    }

	/* handle timeout events */
	timer_handle ();
}


void          DeadPipe (int nonsense);

int
ConnectAfterStep (unsigned long message_mask)
{
    char *temp;
    char  mask_mesg[32];
    int   fd;

	/* connect to AfterStep */
	/* Dead pipe == AS died */
	signal (SIGPIPE, DeadPipe);
    fd = Scr.wmprops?socket_connect_client(Scr.wmprops->as_socket_filename):-1;
    signal (SIGPIPE, DeadPipe);

    if (fd < 0)
	{
        show_error("unable to establish connection to AfterStep");
		exit (1);
	}

    set_module_in_fd( fd );
    set_module_out_fd( fd );

	/* assuming that unsigned long will be limited to 32 chars : */
	temp = safemalloc (9 + 1 +max(strlen (MyName),32) + 1 + 1);
    sprintf (temp, "SET_NAME \"%s\"", MyName);
    SendInfo ( temp, None);
	free (temp);

	sprintf (mask_mesg, "SET_MASK %lu\n", (unsigned long)message_mask);
    SendInfo ( mask_mesg, None);

    /* don't really have to do this here, but anyway : */
    InitSession();
    return fd;
}

void
LoadBaseConfig(void (*read_base_options_func) (const char *))
{
    if( Session == NULL )
    {
        show_error("Session has not been properly initialized. Exiting");
        exit(1);
    }

    if (Session->overriding_file == NULL )
	{
        char *configfile = make_session_file(Session, BASE_FILE, False/* no longer use #bpp in filenames */ );
        if( configfile != NULL )
        {
            read_base_options_func (configfile);
            show_progress("BASE configuration loaded from \"%s\" ...", configfile);
            free( configfile );
        }else
        {
            show_warning("BASE configuration file cannot be found");
        }
    }else
        read_base_options_func (Session->overriding_file);
}

void
LoadConfig (char *config_file_name, void (*read_options_func) (const char *))
{
    if( Session == NULL )
    {
        show_error("Session has not been properly initialized. Exiting");
        exit(1);
    }
    if (Session->overriding_file == NULL )
	{
        char *configfile ;
        const char *const_configfile;

        configfile = make_session_file(Session, config_file_name, False );
        if( configfile != NULL )
        {
            read_options_func(configfile);
            show_progress("configuration loaded from \"%s\" ...", configfile);
            free( configfile );
        }else
        {
            show_warning("configuration file \"%s\" cannot be found", config_file_name);
        }

        if( (const_configfile = get_session_file (Session, 0, F_CHANGE_THEME) ) != NULL )
        {
            read_options_func(const_configfile);
            show_progress("THEME configuration loaded from \"%s\" ...", const_configfile);
            if( (configfile = make_session_data_file  (Session, False, R_OK, THEME_OVERRIDE_FILE, NULL )) != NULL )
            {
                read_options_func(configfile);
                show_progress("THEME OVERRIDES configuration loaded from \"%s\" ...", configfile);
                free( configfile );
            }
        }
    }else
        read_options_func (Session->overriding_file);
}


