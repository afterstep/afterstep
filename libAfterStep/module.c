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
#include "myicon.h"

#define  ASSocketWriteInt32(sb,d,i)  socket_buffered_write( (sb), (d), (i)*sizeof(CARD32))
#define  ASSocketWriteInt16(sb,d,i)  socket_buffered_write( (sb), (d), (i)*sizeof(CARD16))

void
as_socket_write_string (ASSocketBuffer *sb, const char *string)
{
	if (sb && sb->fd >= 0)
	{
		CARD32        len = 0;

		if (string != NULL)
			len = strlen ((char*)string);
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
    if( as_module_out_buffer.fd >= 0 ) 
	{	
		CARD32 w32 = w ;
    	ASSocketWriteInt32 ( &as_module_out_buffer, &w32, 1 );
    	ASSocketWriteInt32 ( &as_module_out_buffer, &bytes, 1 );
	}
}

static inline void
send_module_msg_tail ()
{
    if( as_module_out_buffer.fd >= 0 ) 
	{	
		CARD32           cont = F_FUNCTIONS_NUM;

    	ASSocketWriteInt32 ( &as_module_out_buffer, &cont, 1 );
		socket_write_flush ( &as_module_out_buffer );
	}
}

static inline void
send_module_msg_raw ( void *data, size_t bytes )
{
    if( as_module_out_buffer.fd >= 0 ) 
	{	
		socket_buffered_write(&as_module_out_buffer, data, bytes);
	}
}

static inline void
send_module_msg_function (CARD32 func,
						  const char *name, const char *text, const send_signed_data_type *func_val, const send_signed_data_type *unit_val)
{
    if( as_module_out_buffer.fd >= 0 ) 
	{	
		CARD32        spare_func_val[2] = { 0, 0 };
		CARD32        spare_unit_val[2] = { 0, 0 };

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
}

/***********************************************************************
 *  High level function for message delivery to AfterStep :
 ***********************************************************************/
void
SendInfo ( char *message, send_ID_type window)
{
    if( as_module_out_buffer.fd >= 0 ) 
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
}

/* SendCommand - send a preparsed AfterStep command : */
void
SendCommand( FunctionData * pfunc, send_ID_type window)
{
LOCAL_DEBUG_OUT( "sending command %p to the astep", pfunc );
	if (pfunc != NULL && as_module_out_buffer.fd >= 0 ) 
	{
		send_module_msg_header(window, 0);
        send_module_msg_function(pfunc->func, pfunc->name, pfunc->text, pfunc->func_val, pfunc->unit_val);
		send_module_msg_tail ();
	}
}

void
SendTextCommand ( int func, const char *name, const char *text, send_ID_type window)
{
	send_signed_data_type          dummy_val[2] = { 0, 0 };

	if (IsValidFunc (func) && as_module_out_buffer.fd >= 0 ) 
	{
		send_module_msg_header(window, 0);
		send_module_msg_function(func, (char*)name, (char*)text, dummy_val, dummy_val);
		send_module_msg_tail ();
	}
}

void
SendNumCommand ( int func, const char *name, const send_signed_data_type *func_val, const send_signed_data_type *unit_val, send_ID_type window)
{
	if (IsValidFunc (func) && as_module_out_buffer.fd >= 0)
	{
		send_module_msg_header(window, 0);
		send_module_msg_function(func, (char*)name, NULL, func_val, unit_val);
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
ReadASPacket (int fd, send_data_type *header, send_data_type **body)
{
	int           count, count2;
	size_t        bytes_to_read;
	int           bytes_in = 0;
	char         *cbody;

	if( fd < 0 )
		return -1;

	bytes_to_read = 3 * sizeof (send_data_type);
	cbody = (char *)header;
	do
	{
		count = read (fd, &cbody[bytes_in], bytes_to_read);
		if (count == 0 ||					   /* dead pipe (EOF) */
			(count < 0 && errno != EINTR))	   /* not a signal interuption */
		{
			ASDeadPipe (1);
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
		bytes_to_read = (header[2] - 3) * sizeof (send_data_type);
		if ((*body = (send_data_type *)safemalloc (bytes_to_read)) == NULL)	/* not enough memory */
			return 0;

		cbody = (char *)(*body);
		bytes_in = 0;

		while (bytes_to_read > 0)
		{
			count2 = read (fd, &cbody[bytes_in], bytes_to_read);
			if (count2 == 0 ||				   /* dead pipe (EOF) */
				(count2 < 0 && errno != EINTR))	/* not a signal interuption */
			{
				ASDeadPipe (1);
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

	if( fd < 0 ) 
		return NULL;

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
		msg = (ASMessage *) safecalloc (1, sizeof (ASMessage));
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
module_wait_pipes_input ( void (*as_msg_handler) (send_data_type type, send_data_type *body) )
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


int
ConnectAfterStep (send_data_type message_mask, send_data_type lock_on_send_mask)
{
    char *temp;
    int   fd;

	/* connect to AfterStep */
	/* Dead pipe == AS died */
	signal (SIGPIPE, ASDeadPipe);
    fd = ASDefaultScr->wmprops?socket_connect_client(ASDefaultScr->wmprops->as_socket_filename):-1;

    set_module_in_fd( fd );
    set_module_out_fd( fd );

    if (fd < 0)
	{
        show_error("unable to establish connection to AfterStep");
	}else
	{	
		int arg_len = 0 ; 
		int i ; 
		char *ptr ;
		char *exec_name = strrchr (MyArgs.saved_argv[0], '/');
		int exec_name_len ; 
		send_signed_data_type masks[2] ; 

		masks[0] = message_mask ;
		masks[1] = lock_on_send_mask ;

		if( exec_name == NULL ) 
			exec_name = MyArgs.saved_argv[0] ;
		else
			++exec_name ;

		exec_name_len = strlen(exec_name);
		arg_len = exec_name_len ;
		for( i = 1 ; i < MyArgs.saved_argc ; ++i )
			arg_len += 1+1+strlen( MyArgs.saved_argv[i] )+1 ;	


		/* assuming that unsigned long will be limited to 32 chars : */
		temp = safemalloc (arg_len+1);
    	strcpy(temp, exec_name);
		ptr = temp + exec_name_len; 
		for( i = 1 ; i < MyArgs.saved_argc ; ++i )
		{
			Bool quote = False ; 
			if( MyArgs.saved_argv[i][0] != '-' ) 
			{
				int k = 0;
				int c = MyArgs.saved_argv[i][k] ; 
				while( isalnum(c) || c == '-' || c == '+' || c == '.' || c == '_' ) 
					c = MyArgs.saved_argv[i][++k] ; 
				quote = ( c != '\0' ) ;
			}	 
			sprintf( ptr, quote?" \"%s\"":" %s", MyArgs.saved_argv[i] );	
			while( *ptr ) ++ptr;
		}
		SendTextCommand ( F_SET_NAME, MyName, temp, None);
		free (temp);

		SendNumCommand ( F_SET_MASK, NULL, &masks[0], NULL, None);
		//sprintf (mask_mesg, "SET_MASK %lu %lu\n", (unsigned long)message_mask, (unsigned long) lock_on_send_mask);
    	//SendInfo ( mask_mesg, None);
	}
    /* don't really have to do this here, but anyway : */
    InitSession();
    return fd;
}

void 
SetAfterStepDisconnected()
{
    set_module_in_fd( -1 );
    set_module_out_fd( -1 );
}	 
/*************************************************************************/
/*************************************************************************/
	
void
LoadBaseConfig(void (*read_base_options_func) (const char *))
{
	if( read_base_options_func == NULL ) 
		return ;

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

        if( (const_configfile = get_session_file (Session, 0, F_CHANGE_THEME, False) ) != NULL )
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

/*************************************************************************/
/* some ASTBarProps utilities to be used by modules : 
 *************************************************************************/
Bool 
button_from_astbar_props( struct ASTBarProps *tbar_props, struct button_t *button, 
						  int context, Atom kind, Atom kind_pressed )
{
	int found = 0;
	
	if( button == NULL ) 
		return False;
	
	free_button_resources( button ) ;
	memset( button, 0x00, sizeof(MyButton));

	if( tbar_props != NULL )
	{	
		int i ;	 
		for( i = 0 ; i < tbar_props->buttons_num ; ++i ) 
		{
			MyIcon *icon = NULL ;
			if( tbar_props->buttons[i].kind == kind ) 
				icon = &(button->unpressed);
			else if( tbar_props->buttons[i].kind == kind_pressed ) 	
				icon = &(button->pressed);	 
			if( icon != NULL ) 
			{
				icon_from_pixmaps( icon, tbar_props->buttons[i].pmap, 
										 tbar_props->buttons[i].mask, 
										 tbar_props->buttons[i].alpha );
				/* these does not belong to us !!!*/
				icon->pix = None ;
				icon->mask = None ;
				icon->alpha = None ;
				if (icon->image)
					found++;
			}
		}	 
		button->width = max( button->unpressed.width, button->width );
		button->height = max( button->unpressed.height, button->height );
		if( button->height > 0 && button->width > 0 )
			button->context = context ; 
	}
	
	return (found > 0 && button->height > 0 && button->width > 0 );
}

void
destroy_astbar_props( struct ASTBarProps **props ) 
{
	if( props ) 
		if( *props )
		{	
			if( (*props)->buttons ) 
				free((*props)->buttons);
			free( *props );
			*props = NULL ;
		}
}


