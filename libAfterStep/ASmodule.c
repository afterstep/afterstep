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
#include "../include/asapp.h"
#include <signal.h>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/time.h>

#include "../libAfterImage/afterimage.h"
#include "../include/screen.h"
#include "../include/module.h"
#include "../include/wmprops.h"
#include "../include/session.h"

char         *display_name = NULL;

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
    ASSocketWriteInt32 ( &as_module_out_buffer, &w, 1 );
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

/*************************************************************************/
/* establishing the connection :                                         */
/*************************************************************************/

int
module_connect (const char *socket_name)
{
	int           fd;

    if( socket_name == NULL )
        return -1;

	/* create an unnamed socket */
	if ((fd = socket (AF_UNIX, SOCK_STREAM, 0)) < 0)
	{
		fprintf (stderr, "%s: unable to create UNIX socket: ", MyName);
		perror ("");
	}

	/* connect to the named socket */
	if (fd >= 0)
	{
		struct sockaddr_un name;

		name.sun_family = AF_UNIX;
		strcpy (name.sun_path, socket_name);

		if (connect (fd, (struct sockaddr *)&name, sizeof (struct sockaddr_un)))
		{
			fprintf (stderr, "%s: unable to connect to socket '%s': ", MyName, name.sun_path);
			perror ("");
			close (fd);
			fd = -1;
		}
	}

	return fd;
}

int           GetFdWidth (void);

ASMessage    *
CheckASMessageFine (int fd, int t_sec, int t_usec)
{
	fd_set        in_fdset;
	ASMessage    *msg = NULL;
	struct timeval tv;

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
module_wait_pipes_input ( int x_fd, int as_fd, void (*as_msg_handler) (unsigned long type, unsigned long *body) )
{
    fd_set        in_fdset, out_fdset;
	int           retval;
	struct timeval tv;
	struct timeval *t = NULL;
    int           max_fd = 0;
    ASMessage     msg;

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

	temp = safemalloc (9 + strlen (MyName) + 1);
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
        char *configfile = make_session_file(Session, BASE_FILE, True );
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

/**********************************************************************/
/*              ModuleInfo functions                                  */
/**********************************************************************/
void
default_version_func (void)
{
	printf ("%s version %s\n", MyName, VERSION);
	exit (0);
}

void          (*custom_version_func) (void) = default_version_func;

int
ProcessModuleArgs (int argc, char **argv, char **global_config_file, unsigned long *app_window,
				   unsigned long *app_context, void (*custom_usage_func) (void))
{
	int           i;

	display_name = getenv ("DISPLAY");
	for (i = 1; i < argc && *argv[i] == '-'; i++)
	{
		if (!strcmp (argv[i], "-h") || !strcmp (argv[i], "--help"))
		{
			if (custom_usage_func)
				custom_usage_func ();
			else
				exit (0);
		} else if (!strcmp (argv[i], "-d") || !strcmp (argv[i], "--display"))
		{
			display_name = argv[++i];
		} else if (!strcmp (argv[i], "-v") || !strcmp (argv[i], "--version"))
		{
			if (custom_version_func)
				custom_version_func ();
			else
				exit (0);
		} else if (!strcmp (argv[i], "-w") || !strcmp (argv[i], "--window"))
		{
			i++;
			if (app_window)
				*app_window = strtol (argv[i], NULL, 16);
		} else if (!strcmp (argv[i], "-c") || !strcmp (argv[i], "--context"))
		{
			i++;
			if (app_context)
				*app_context = strtol (argv[i], NULL, 16);
		} else if (!strcmp (argv[i], "-f") && i + 1 < argc)
		{
			i++;
			if (global_config_file)
				*global_config_file = argv[i];
		} else
			return i;

	}
	return i;
}
