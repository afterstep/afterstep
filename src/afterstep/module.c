/*
 * Copyright (c) 1999 Ethan Fischer <allanon@crystaltokyo.com>
 * Copyright (C) 1998 Guylhem Aznar
 * Copyright (C) 1993 Robert Nation
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

/***********************************************************************
 *
 * code for launching afterstep modules.
 *
 ***********************************************************************/

#include "../../configure.h"

#include <signal.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/stat.h>						   /* for chmod() */
#include <sys/types.h>
#include <sys/un.h>							   /* for struct sockaddr_un */

#include <X11/keysym.h>

#include "../../include/asapp.h"
#include "../../include/afterstep.h"
#include "../../include/decor.h"
#include "../../include/screen.h"
#include "../../include/module.h"
#include "../../include/wmprops.h"
#include "../../include/event.h"
#include "asinternals.h"

typedef CARD32 send_data_type;
static DECL_VECTOR(send_data_type, module_output_buffer);

static void       DeleteQueueBuff (module_t *module);
static void       AddToQueue      (module_t *module, unsigned long *ptr, int size, int done);

int           module_listen (const char *socket_name);


/* create a named UNIX socket, and start watching for connections */
Bool
module_setup_socket ()
{
	char         *tmp;
#ifdef __CYGWIN__
	{										   /* there are problems under Windows as home dir could be anywhere  -
											    * so we just use /tmp since there will only be 1 user anyways :) */
        tmp = safemalloc (4 + 9 + 32 + 1);
		/*sprintf (tmp, "/tmp/connect.%s", display_string); */
        sprintf (tmp, "/tmp/as-connect.%ld", Scr.screen);
		fprintf (stderr, "using %s for intermodule communications.\n", tmp);
	}
#else
	{
        char         *display = XDisplayString (dpy);
        char *tmpdir = getenv("TMPDIR");
        static char *default_tmp_dir = "/tmp" ;
        if( tmpdir != NULL )
            if( CheckDir( tmpdir ) < 0 )
                tmpdir = NULL ;

        if( tmpdir == NULL )
            tmpdir = default_tmp_dir ;
        tmp = safemalloc (strlen(tmpdir)+11+32 + strlen (display) + 1);
        sprintf (tmp, "%s/afterstep-%d.%s", tmpdir, getuid(), display);
		LOCAL_DEBUG_OUT("using socket \"%s\" for intermodule communications", tmp);
	}
#endif
    set_as_module_socket( Scr.wmprops, tmp );
    Module_fd = socket_listen (tmp);
	free (tmp);

	XSync (dpy, 0);

    return (Module_fd >= 0);
}

void
KillModule (module_t *module)
{
LOCAL_DEBUG_OUT( "module %p ", module );
LOCAL_DEBUG_OUT( "module name \"%s\"", module->name );
    if (module->fd > 0)
        close (module->fd);

    while (module->output_queue != NULL)
        DeleteQueueBuff (module);
    if (module->name != NULL)
        free (module->name);
    if (module->ibuf.text != NULL)
        free (module->ibuf.text);

    if (module->ibuf.func != NULL)
	{
        free_func_data (module->ibuf.func);
        free (module->ibuf.func);
	}

    memset( module, 0x00, sizeof(module_t) );
    module->fd = -1;
    module->active = -1;
}


/*
 * ReadModuleInput Does actuall read from the module pipe.
 * returns :
 *  0, -1    - error or not enough data
 *  1        - SUCCESS
 */



static int
ReadModuleInput (module_t *module, size_t * offset, size_t size, void *ptr)
{
    size_t        done_this = module->ibuf.done - *offset;
	int           n = size;

	if (done_this >= 0 && done_this < size)
	{
		ptr += done_this;
        n = read (module->fd, ptr, size - done_this);
		if (n > 0)
		{
            module->ibuf.done += n;
            if (module->ibuf.done < *offset + size)
				return 0;					   /* No more data available */
		} else
			return (n == -1 && (errno == EINTR || errno == EAGAIN)) ? 0 : -1;
	}
	if (n > 0)
		*offset += n;
	return 1;								   /* Success */
}

static void
CheckCmdTextSize (module_t *module, size_t * size, size_t * curr_len, char **text)
{
	/* max command length is 255 */
	if (*size > 255)
	{
        show_error("command from module '%s' is too big (%d)", module->name, *size);
		*size = 255;
	}

	/* need to be able to read in command */
	if (*curr_len < *size + 1)
	{
		*curr_len = *size + 1;
		*text = realloc (*text, *curr_len);
	}
}

/*
 * Higher level protocol handler
 *
 * Two possible protocols :
 * 1. Text command line :
 * <window>< 0<size<256 >< size bytes of text >
 * < continuation_indicator == F_FUNCTIONS_NUM >
 *
 * 2. Preprocessed AS Function data :
 * <window>< size==0 ><function_code>
 *  < 0<name_size<256 >[< name_size bytes of function name >|<nothing if size == 0>]
 *  < 0<text_size<256 >[< size bytes of text >|< nothing if size == 0>]
 *  < 2*sizeof(long) of func_val[] >< 2*sizeof(long) of unit_val >
 * < continuation_indicator == F_FUNCTIONS_NUM >
 *
 * Returns :
 *  -2    - bad module
 *  0, -1 - error or not enough data
 *  > 0   - command execution result
 */

int
HandleModuleInput (module_t *module)
{
	size_t        offset = 0;
	int           res = 1, invalid_func = -1;
    register module_ibuf_t *ibuf ;

    ibuf = &(module->ibuf);

    /* read a window id */
    res = ReadModuleInput (module, &offset, sizeof (Window), &(ibuf->window));
	if (res > 0)
	{
        module->active = 1;
        res = ReadModuleInput (module, &offset, sizeof (ibuf->size), &(ibuf->size));
	}

	if (res > 0)
	{
		if (ibuf->size > 0)					   /* Protocol 1 */
		{
            CheckCmdTextSize (module, &(ibuf->size), &(ibuf->len), &(ibuf->text));
            res = ReadModuleInput (module, &offset, ibuf->size, ibuf->text);

			if (res > 0)
			{
				/* null-terminate the command line */
				ibuf->text[ibuf->size] = '\0';
                invalid_func = (ibuf->func=String2Func (ibuf->text, ibuf->func, False))!= NULL?1:-1;
			}
		} else								   /* Protocol 2 */
		{
			size_t        curr_len;
			register FunctionData *pfunc = ibuf->func;

			if (pfunc == NULL)
			{
				pfunc = (FunctionData *) safemalloc (sizeof (FunctionData));
				init_func_data (pfunc);
				ibuf->func = pfunc;
			}

            res = ReadModuleInput (module, &offset, sizeof (pfunc->func), &(pfunc->func));

			if (res > 0)
			{
				if (!IsValidFunc (pfunc->func))
				{
					res = 0;
					ibuf->done = 0;
                    invalid_func = -1;
				} else
                    res = ReadModuleInput (module, &offset, sizeof (int), &(ibuf->name_size));
			}
			if (res > 0 && ibuf->name_size > 0)
			{
				curr_len = (pfunc->name) ? strlen (pfunc->name) + 1 : 0;
                CheckCmdTextSize (module, &(ibuf->name_size), &curr_len, &(pfunc->name));
                res = ReadModuleInput (module, &offset, ibuf->name_size, pfunc->name);
				pfunc->name[ibuf->name_size] = '\0';
			}
            if (res > 0)
                res = ReadModuleInput (module, &offset, sizeof (int), &(ibuf->text_size));

			if (res > 0 && ibuf->text_size > 0)
			{
				curr_len = (pfunc->text) ? strlen (pfunc->text) + 1 : 0;
                CheckCmdTextSize (module, &(ibuf->text_size), &curr_len, &(pfunc->text));
                res = ReadModuleInput (module, &offset, ibuf->text_size, pfunc->text);
				pfunc->text[ibuf->text_size] = '\0';
			}
			if (res > 0)
                res = ReadModuleInput (module, &offset, sizeof (long) * 2, pfunc->func_val);

			if (res > 0)
                res = ReadModuleInput (module, &offset, sizeof (long) * 2, pfunc->unit_val);

			if (res > 0 && IsValidFunc (pfunc->func))
				invalid_func = 0;
		}
	}

	/* get continue command */
	if (res > 0)
	{
        res = ReadModuleInput (module, &offset, sizeof (ibuf->cont), &(ibuf->cont));
		if (res > 0)
		{
			if (ibuf->cont != F_FUNCTIONS_NUM)
				if (ibuf->cont != 1)
					res = -1;
		} else
			ibuf->cont = 0;
	}

	if (res < 0)
        KillModule (module);
    else if (res > 0)
        ibuf->done = 0;                        /* done reading command */

    PRINT_MEM_STATS(NULL);
	return res;
}

static void
AddToQueue (module_t *module, send_data_type *ptr, int size, int done)
{
    register struct queue_buff_struct *new_elem, **tail;

    new_elem = safecalloc (1, sizeof (struct queue_buff_struct));

    new_elem->size = size;
    new_elem->done = done;
    new_elem->data = safemalloc (size);
    memcpy (new_elem->data, ptr, size);
LOCAL_DEBUG_OUT( "que_buff %p: size = %d, done = %d, data = %p", new_elem, size, done, new_elem->data );
    for( tail = &(module->output_queue) ; *tail ; tail = &((*tail)->next) );
    *tail = new_elem;
}

static void
DeleteQueueBuff (module_t *module)
{
    register struct queue_buff_struct *a = module->output_queue;
    if ( a )
    {
        module->output_queue = a->next;
        free (a->data);
        free (a);
    }
}

int
FlushQueue (module_t *module)
{
	extern int    errno;
    int           fd;
    register struct queue_buff_struct *curr;

    if ( module->active <= 0 )
        return -1;
    if( module->output_queue == NULL )
        return 1;

    fd = module->fd ;
    while( (curr = module->output_queue) != NULL)
	{
        register unsigned char *dptr = curr->data;
        int bytes_written = 0;

        do
        {
            if( (bytes_written = write (fd, &dptr[curr->done], curr->size - curr->done)) > 0 )
                curr->done += bytes_written ;
        }while( curr->done < curr->size && bytes_written > 0);

		/* the write returns EWOULDBLOCK or EAGAIN if the pipe is full.
		 * (This is non-blocking I/O). SunOS returns EWOULDBLOCK, OSF/1
		 * returns EAGAIN under these conditions. Hopefully other OSes
		 * return one of these values too. Solaris 2 doesn't seem to have
		 * a man page for write(2) (!) */
        if (bytes_written < 0 )
        {
            if(errno == EWOULDBLOCK || errno == EAGAIN || errno == EINTR)
            {
               return 0;
            }else
            {
                KillModule (module);
                return -1;
            }
        }
        DeleteQueueBuff (module);
	}
    return 1;
}




#include <sys/errno.h>
static inline int
PositiveWrite (unsigned int channel, send_data_type *ptr, int size)
{
    module_t *module = &(MODULES_LIST[channel]);
	register unsigned long mask = 0x01<<ptr[1] ;

    if (module->active < 0 || !get_flags(module->mask, mask))
		return -1;

	AddToQueue (module, ptr, size, 0);
    if (get_flags(module->mask, M_LOCKONSEND))
	{
        int           res ;
        do
        {
            if( (res = FlushQueue (module)) >= 0 )
                res = HandleModuleInput (module);
            if ( res > 0  ) /* need to run command */
            {
                if( module->ibuf.func->func == F_UNLOCK )
                    return size;
                RunCommand (module->ibuf.func, channel, module->ibuf.window);
            }
        }while( res >= 0 );
	}
	return size;
}

static send_data_type *
make_msg_header( unsigned long msg_type, unsigned long size )
{
    static send_data_type msg_header[MSG_HEADER_SIZE];

    msg_header[0] = START_FLAG ;
    msg_header[1] = msg_type ;
    msg_header[2] = size + MSG_HEADER_SIZE ;
    return &(msg_header[0]);
}

void
SendBuffer( int channel )
{
    send_data_type *b = VECTOR_HEAD(send_data_type,module_output_buffer);
    unsigned long size_to_send ;

    size_to_send = b[2];
    if( size_to_send > 0 && Modules )
	{
		/* lets make sure that we will not overrun the buffer : */
        realloc_vector(&module_output_buffer, size_to_send );
        b = VECTOR_HEAD(send_data_type,module_output_buffer);
LOCAL_DEBUG_OUT( "sending %d words to module # %d of %d", size_to_send, channel, MODULES_NUM );
        if( channel >= 0 && channel < MODULES_NUM)
            PositiveWrite (channel, b, size_to_send*sizeof(send_data_type) );
	    else
  		{
            register int i = MODULES_NUM;
            while ( --i >= 0 )
                PositiveWrite (i, b, size_to_send*sizeof(send_data_type));
	    }
	}
}



/********************************************************************************/
/* public interfaces :                                                          */
/********************************************************************************/
int
AcceptModuleConnection (int socket_fd)
{
	int           fd;
	int           len = sizeof (struct sockaddr_un);
	struct sockaddr_un name;

	fd = accept (socket_fd, (struct sockaddr *)&name, &len);

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

	/* mark as close-on-exec so other programs won't inherit the socket */
	if (fd >= 0)
	{
		if (fcntl (fd, F_SETFD, 1) == -1)
		{
            show_system_error("unable to set close-on-exec for module socket");
			close (fd);
			fd = -1;
		}
	}

	if (fd >= 0 && Modules )
	{
        int channel = -1;

        /* Look for an available pipe slot */
        if( MODULES_NUM < Module_npipes-1 )
        {
            module_t   new_module ;
			memset( &new_module, 0x00, sizeof(module_t) );
            /* add pipe to afterstep's active pipe list */
            new_module.name = mystrdup ("unknown module");
            new_module.fd = fd;
            new_module.active = 0;
            new_module.mask = MAX_MASK;
            new_module.output_queue = NULL;
            /* adding new module to the end of the list */
			LOCAL_DEBUG_OUT( "adding new module:  total modules %d. list starts at %p", MODULES_NUM, MODULES_LIST );
            channel = vector_insert_elem( Modules, &new_module, 1, NULL, False );
			LOCAL_DEBUG_OUT( "added module # %d : total modules %d. list starts at %p", channel, MODULES_NUM, MODULES_LIST );

        }
        if( channel < 0 )
        {
            show_error("too many modules!");
			close (fd);
            fd = -1 ;
		}
    }

	return fd;
}

void
ShutdownModules()
{
    if (Modules != NULL)
    {
        register int i = MODULES_NUM;
        register module_t *list = MODULES_LIST ;
LOCAL_DEBUG_OUT( "total modules %d. list starts at %p", MODULES_NUM, MODULES_LIST );
        while(--i >= 0 )
            KillModule ( &(list[i]) );
        destroy_asvector(&Modules);
		free_vector( &module_output_buffer );
        Modules = NULL;
    }
}

void
SetupModules(void)
{
    if( Modules )
        ShutdownModules();

    Module_npipes = get_fd_width ();
	LOCAL_DEBUG_OUT( "max Module pipes = %d", Module_npipes );
    Modules = create_asvector(sizeof(module_t));
    module_setup_socket ();
}

void
ExecModule (char *action, Window win, int context)
{
	int           val;
    char         *module, *cmd, *args ;

    if (action == NULL || Modules == NULL )
		return;
    args = parse_filename( action, &module );
    if( module == NULL )
        return ;
    if ((cmd = findIconFile (module, ModulePath, X_OK)) == NULL)
	{
        show_error("no such module %s in path %s\n", module, ModulePath);
        free( module );
        return;
	}
    free( module );

    args = stripcpy( args );
    val = spawn_child( cmd, -1, Scr.screen, win, context, True, True, args, NULL );

    if( args )
        free (args);
    free( cmd );
}


void
HandleModuleInOut(unsigned int channel, Bool has_input, Bool has_output)
{
    int res  = 0;

    if( Modules && channel < MODULES_NUM )
    {
        register module_t *module = &(MODULES_LIST[channel]);
        if( has_input )
        {
            res = HandleModuleInput( module );
            if( res < 0 )
                has_output = 0 ;
            else if (res > 0 && IsValidFunc (module->ibuf.func->func) ) /* need to run command */
                RunCommand (module->ibuf.func, channel, module->ibuf.window);
        }
        if( has_output )
            FlushQueue( module );
	}
}

void
DeadPipe (int nonsense)
{
	signal (SIGPIPE, DeadPipe);
}


void
KillModuleByName (char *name)
{
    wild_reg_exp  *wrexp = compile_wild_reg_exp( name );

    if ( wrexp != NULL && Modules)
    {
        register int i = MODULES_NUM;
        register module_t *list = MODULES_LIST ;

        while( --i >= 0 )
            if (list[i].fd > 0)
            {
                if (match_wild_reg_exp( list[i].name, wrexp ) == 0 )
                    KillModule (&(list[i]));
            }
        destroy_wild_reg_exp( wrexp );
    }
	return;
}

void
SendPacket ( int channel, unsigned long msg_type, unsigned long num_datum, ...)
{
	va_list       ap;
    register send_data_type *body;
    int i ;

    flush_vector( &module_output_buffer );
    append_vector( &module_output_buffer, make_msg_header(msg_type,num_datum), MSG_HEADER_SIZE );

    append_vector( &module_output_buffer, NULL, num_datum);
    body = VECTOR_TAIL(send_data_type,module_output_buffer) ;

    va_start (ap, num_datum);
	for (i = 0; i < num_datum; i++)
        *(body++) = va_arg (ap, send_data_type);

    VECTOR_USED(module_output_buffer) += num_datum;
	va_end (ap);

    SendBuffer( channel );
}

void
SendString (int channel, unsigned long msg_type, unsigned long id, unsigned long tag, char *string )
{
    unsigned long data[3];
    int           len = 0;

    if (string == NULL)
		return;
    len = strlen(string);

    flush_vector( &module_output_buffer );
    append_vector( &module_output_buffer,
                    make_msg_header(msg_type,2+1+(len>>2)+1+MSG_HEADER_SIZE),
                    MSG_HEADER_SIZE );
    data[0] = id;
    data[1] = tag;
    append_vector( &module_output_buffer, &(data[0]), 2);
    serialize_string( string, &module_output_buffer );
    SendBuffer( channel );
}

void
SendVector (int channel, unsigned long msg_type, ASVector *vector)
{
    if (vector == NULL )
		return;

    flush_vector( &module_output_buffer );
    append_vector( &module_output_buffer,
                    make_msg_header(msg_type,VECTOR_USED(*vector)),
                    MSG_HEADER_SIZE );
    append_vector( &module_output_buffer, VECTOR_HEAD(CARD32,*vector), VECTOR_USED(*vector));

    SendBuffer( channel );
}





/* this will run command received from module */
void
RunCommand (FunctionData * fdata, unsigned int channel, Window w)
{
	ASWindow     *tmp_win;
    int           toret = 0;
	extern module_t *Module;

/*fprintf( stderr,"Function parsed: [%s] [%s] [%d] [%d] [%c]\n",fdata.name,fdata.text,fdata.func_val[0], fdata.func_val[1] );
 */
    if (Modules == NULL || fdata == NULL || channel >= MODULES_NUM )
        return;
    if (!IsValidFunc(fdata->func))
        return;

    switch (fdata->func)
	{
	 case F_SET_MASK:
         Module[channel].mask = fdata->func_val[0];
		 break;
	 case F_SET_NAME:
        {
            module_t *module = &(MODULES_LIST[channel] );
            if (module->name != NULL)
                free (module->name);
            module->name = fdata->text;
            fdata->text = NULL;
            break;
        }
	 case F_UNLOCK:
		 toret = 66;
		 break;
	 case F_SET_FLAGS:
		 {
			 int           xorflag;
			 Bool          update = False;

             if ((tmp_win = window2ASWindow(w)) == NULL)
				 break;
             xorflag = tmp_win->hints->flags ^ fdata->func_val[0];
			 /*if (xorflag & STICKY)
			    Stick (tmp_win); */
			 if (xorflag & AS_SkipWinList)
			 {
				 tmp_win->hints->flags ^= AS_SkipWinList;
				 update_windowList ();
				 update = True;
			 }
			 if (xorflag & AS_AvoidCover)
			 {
				 tmp_win->hints->flags ^= AS_AvoidCover;
				 update = True;
			 }
			 if (xorflag & AS_Transient)
			 {
				 tmp_win->hints->flags ^= AS_Transient;
				 update = True;
			 }
			 if (xorflag & AS_DontCirculate)
			 {
				 tmp_win->hints->flags ^= AS_DontCirculate;
				 update = True;
			 }
			 if (update)
				 BroadcastConfig (M_CONFIGURE_WINDOW, tmp_win);
			 break;
		 }
	 default:
        {
            ASEvent event;
            event.w = w;
            if ((event.client = window2ASWindow(w)) == NULL )
            {
                event.w = None;
                event.x.xbutton.x_root = 0;
                event.x.xbutton.y_root = 0;
            } else
            {
                event.x.xbutton.x_root = event.client->frame_canvas->root_x;
                event.x.xbutton.y_root = event.client->frame_canvas->root_y;
            }
            event.x.xany.type = ButtonRelease;
            event.x.xbutton.button = 1;
            event.x.xbutton.x = 0;
            event.x.xbutton.y = 0;
            event.x.xbutton.subwindow = None;
            event.context = C_FRAME;
            ExecuteFunction (fdata, &event, channel);
        }
    }
    free_func_data (fdata);
}

/*******************************************************************************/
/* usefull functions to simplify life in other places :                        */
/*******************************************************************************/
void
broadcast_focus_change( ASWindow *focused )
{
    if( focused == NULL )
        SendPacket(-1, M_FOCUS_CHANGE, 3, 0L, 0L, 0L);
    else
        SendPacket(-1, M_FOCUS_CHANGE, 3, focused->w, focused->frame, (unsigned long)focused);
}

void
broadcast_window_name( ASWindow *asw )
{
    if( asw )
        SendName( -1, M_WINDOW_NAME, asw->w, asw->frame,
                      (unsigned long)asw, ASWIN_NAME(asw));
}

void
broadcast_icon_name( ASWindow *asw )
{
    if( asw )
        SendName( -1, M_ICON_NAME, asw->w, asw->frame,
                    (unsigned long)asw, ASWIN_ICON_NAME(asw));
}

void
broadcast_res_names( ASWindow *asw )
{
    if( asw )
    {
        SendName( -1, M_RES_CLASS, asw->w, asw->frame,
                    (unsigned long)asw, asw->hints->res_class);
        SendName( -1, M_RES_NAME, asw->w, asw->frame,
                    (unsigned long)asw, asw->hints->res_name);
    }
}

void
broadcast_status_change( int message, ASWindow *asw )
{
    if( message == M_DEICONIFY || message == M_ICONIFY )
    {
        ASRectangle geom = {INVALID_POSITION, INVALID_POSITION, 0, 0} ;
        get_icon_root_geometry( asw, &geom );
        SendPacket( -1, message, 7, asw->w, asw->frame, (unsigned long)asw,
                    geom.x, geom.y, geom.width, geom.height);
    }else if( message == M_MAP )
        SendPacket( -1, M_MAP, 3, asw->w, asw->frame, (unsigned long)asw);
}





/**************************************************************************/
/* old stuff : */
#if 0

int           module_fd;
int           npipes;

extern char  *global_base_file;

module_t     *Module = NULL;

AFTER_INLINE int PositiveWrite (int module, unsigned long *ptr, int size);
void          DeleteQueueBuff (int module);
void          AddToQueue (int module, unsigned long *ptr, int size, int done);

extern ASDirs as_dirs;

int
module_setup_socket (Window w, const char *display_string)
{
	char         *tmp;

#ifdef __CYGWIN__
	{										   /* there are problems under Windows as home dir could be anywhere  -
											    * so we just use /tmp since there will only be 1 user anyways :) */
		tmp = safemalloc (4 + 9 + strlen (display_string) + 1);
		/*sprintf (tmp, "/tmp/connect.%s", display_string); */
		sprintf (tmp, "/tmp/as-connect.%ld", Scr.screen);
		fprintf (stderr, "using %s for intermodule communications.\n", tmp);
	}
#else
	{
		char         *tmpdir = getenv ("TMPDIR");
		static char  *default_tmp_dir = "/tmp";

		if (tmpdir != NULL)
			if (CheckDir (tmpdir) < 0)
				tmpdir = NULL;

		if (tmpdir == NULL)
			tmpdir = default_tmp_dir;
		tmp = safemalloc (strlen (tmpdir) + 11 + 32 + strlen (display_string) + 1);
		sprintf (tmp, "%s/afterstep-%d.%s", tmpdir, getuid (), display_string);
	}
#endif

	XChangeProperty (dpy, w, _AS_MODULE_SOCKET, XA_STRING, 8,
					 PropModeReplace, (unsigned char *)tmp, strlen (tmp));
	module_fd = module_listen (tmp);
	free (tmp);

	XSync (dpy, 0);

	return (module_fd >= 0);
}

/* create a named UNIX socket, and start watching for connections */
int
module_listen (const char *socket_name)
{
	int           fd;

	/* create an unnamed socket */
	if ((fd = socket (AF_UNIX, SOCK_STREAM, 0)) < 0)
	{
		fprintf (stderr, "%s: unable to create UNIX socket: ", MyName);
		perror ("");
	}

	/* remove the socket file */
	if (fd >= 0)
	{
		if (unlink (socket_name) == -1 && errno != ENOENT)
		{
			fprintf (stderr, "%s: unable to delete file '%s': ", MyName, socket_name);
			perror ("");
			close (fd);
			fd = -1;
		}
	}

	/* bind the socket to a name */
	if (fd >= 0)
	{
		struct sockaddr_un name;

		name.sun_family = AF_UNIX;
		strcpy (name.sun_path, socket_name);

		if (bind (fd, (struct sockaddr *)&name, sizeof (struct sockaddr_un)) == -1)
		{
			fprintf (stderr, "%s: unable to bind socket to name '%s': ", MyName, socket_name);
			perror ("");
			close (fd);
			fd = -1;
		}
	}

	/* set file permissions to 0700 (read/write/execute by owner only) */
	if (fd >= 0)
	{
		if (chmod (socket_name, 0700) == -1)
		{
			fprintf (stderr, "%s: unable to set socket permissions to 0700: ", MyName);
			perror ("");
			close (fd);
			fd = -1;
		}
	}

	/* start listening for incoming connections */
	if (fd >= 0)
	{
		if (listen (fd, 254) == -1)
		{
			fprintf (stderr, "%s: unable to listen on socket: ", MyName);
			perror ("");
			close (fd);
			fd = -1;
		}
	}

	/* set non-blocking I/O mode */
	if (fd >= 0)
	{
		if (fcntl (fd, F_SETFL, fcntl (fd, F_GETFL) | O_NONBLOCK) == -1)
		{
			fprintf (stderr, "%s: unable to set non-blocking I/O: ", MyName);
			perror ("");
			close (fd);
			fd = -1;
		}
	}

	/* mark as close-on-exec so other programs won't inherit the socket */
	if (fd >= 0)
	{
		if (fcntl (fd, F_SETFD, 1) == -1)
		{
			fprintf (stderr, "%s: unable to set close-on-exec for socket: ", MyName);
			perror ("");
			close (fd);
			fd = -1;
		}
	}

	return fd;
}

int
module_accept (int socket_fd)
{
	int           fd;
	int           len = sizeof (struct sockaddr_un);
	struct sockaddr_un name;

	fd = accept (socket_fd, (struct sockaddr *)&name, &len);
	if (fd < 0 && errno != EWOULDBLOCK)
	{
		fprintf (stderr, "%s: error accepting connection: ", MyName);
		perror ("");
	}
	/* set non-blocking I/O mode */
	if (fd >= 0)
	{
		if (fcntl (fd, F_SETFL, fcntl (fd, F_GETFL) | O_NONBLOCK) == -1)
		{
			fprintf (stderr, "%s: unable to set non-blocking I/O for module socket: ", MyName);
			perror ("");
			close (fd);
			fd = -1;
		}
	}
	/* mark as close-on-exec so other programs won't inherit the socket */
	if (fd >= 0)
	{
		if (fcntl (fd, F_SETFD, 1) == -1)
		{
			fprintf (stderr, "%s: unable to set close-on-exec for module socket: ", MyName);
			perror ("");
			close (fd);
			fd = -1;
		}
	}
	if (fd >= 0)
	{
		int           i;

		/* Look for an available pipe slot */
		i = 0;
		while ((i < npipes) && (Module[i].fd >= 0))
			i++;
		if (i >= npipes)
		{
			fprintf (stderr, "%s: too many modules!\n", MyName);
			close (fd);
			return -1;
		}
		/* add pipe to afterstep's active pipe list */
		Module[i].name = mystrdup ("unknown module");
		Module[i].fd = fd;
		Module[i].active = 0;
		Module[i].mask = MAX_MASK;
		Module[i].output_queue = NULL;
	}
	return fd;
}

void
module_init (int free_resources)
{
	if (free_resources)
	{
		ClosePipes ();
		if (Module != NULL)
			free (Module);
	}

	Module = NULL;
}

void
module_setup (void)
{
	int           i;

	npipes = GetFdWidth ();

	Module = safemalloc (sizeof (module_t) * npipes);

	for (i = 0; i < npipes; i++)
	{
		Module[i].fd = -1;
		Module[i].active = -1;
		Module[i].mask = MAX_MASK;
		Module[i].output_queue = NULL;
		Module[i].name = NULL;
		Module[i].ibuf.len = 0;
		Module[i].ibuf.done = 0;
		Module[i].ibuf.text = NULL;
	}
}

void
ClosePipes (void)
{
	int           i;

	for (i = 0; i < npipes; i++)
		KillModule (i, 0);
}

void
executeModule (char *action, FILE * fd, char **win, int *context)
{
	int           val;
	char          command[256];
	char         *cptr;
	char         *aptr = NULL;
	char         *end;
	char         *arg0;
	extern char  *ModulePath;

	if (action == NULL)
		return;

	sprintf (command, "%.255s", action);
	command[255] = '\0';

	for (cptr = command; isspace (*cptr); cptr++);

	for (end = cptr; !isspace (*end) && *end != '\0'; end++);

	if (*end != '\0')
	{
		for (*end++ = '\0'; isspace (*end); end++);
		if (*end != '\0')
			aptr = end;
	}

	arg0 = findIconFile (cptr, ModulePath, X_OK);
	if (arg0 == NULL)
	{
		fprintf (stderr, "%s: no such module %s in path %s\n", MyName, cptr, ModulePath);
		return;
	}

	val = fork ();
	if (val == 0)
	{
		/* this is the child */
		extern Bool   shall_override_config_file;
		int           i = 0;
		char         *args[10];
		char          arg1[40];
		char          arg2[40];

		args[i++] = arg0;

		args[i++] = "--window";
		sprintf (arg1, "%lx", (unsigned long)win);
		args[i++] = arg1;

		args[i++] = "--context";
		sprintf (arg2, "%lx", (unsigned long)context);
		args[i++] = arg2;

		if (shall_override_config_file)
		{
			args[i++] = "-f";
			args[i++] = global_base_file;
		}

		if (aptr != NULL)
			args[i++] = aptr;

		args[i++] = NULL;

		execvp (arg0, args);
		fprintf (stderr, "%s: execution of module '%s' failed: ", MyName, arg0);
		perror ("");
		exit (1);
	}

	free (arg0);
}

int
HandleModuleInput (int channel)
{
	Window        w;
	char         *text;
	int           len, done;
	void         *ptr;

	/* read a window id */
	done = Module[channel].ibuf.done;
	len = sizeof (Window);
	ptr = &Module[channel].ibuf.window;
	if (done < len)
	{
		int           n = read (Module[channel].fd, ptr + done, len - done);

		if (n == 0 || (n == -1 && errno != EINTR && errno != EAGAIN))
		{
			KillModule (channel, 101 + n);
			return -1;
		}
		if (n == -1)
			return 0;
		Module[channel].ibuf.done += n;
		if (done + n < len)
			return 0;
	}

	/* the pipe is active */
	Module[channel].active = 1;

	/* read an afterstep bultin command line */
	done = Module[channel].ibuf.done - sizeof (Window);
	len = sizeof (int);
	ptr = &Module[channel].ibuf.size;
	if (done < len)
	{
		int           n = read (Module[channel].fd, ptr + done, len - done);

		if (n == 0 || (n == -1 && errno != EINTR && errno != EAGAIN))
		{
			KillModule (channel, 103 + n);
			return -1;
		}
		if (n == -1)
			return 0;
		Module[channel].ibuf.done += n;
		if (done + n < len)
			return 0;
	}

	/* max command length is 255 */
	if (Module[channel].ibuf.size > 255)
	{
		fprintf (stderr, "%s: command from module '%s' is too big (%d)\n", MyName,
				 Module[channel].name, Module[channel].ibuf.size);
		Module[channel].ibuf.size = 255;
	}

	/* need to be able to read in command */
	if (Module[channel].ibuf.len < Module[channel].ibuf.size + 1)
	{
		Module[channel].ibuf.len = Module[channel].ibuf.size + 1;
		Module[channel].ibuf.text = realloc (Module[channel].ibuf.text, Module[channel].ibuf.len);
	}

	/* read an afterstep builtin command line */
	done = Module[channel].ibuf.done - sizeof (Window) - sizeof (int);
	len = Module[channel].ibuf.size;
	ptr = Module[channel].ibuf.text;
	if (done < len)
	{
		int           n = read (Module[channel].fd, ptr + done, len - done);

		if (n == 0 || (n == -1 && errno != EINTR && errno != EAGAIN))
		{
			KillModule (channel, 105 + n);
			return -1;
		}
		if (n == -1)
			return 0;
		Module[channel].ibuf.done += n;
		if (done + n < len)
			return 0;
	}

	/* get continue command */
	done = Module[channel].ibuf.done - sizeof (Window) - sizeof (int) - Module[channel].ibuf.size;
	len = sizeof (int);
	ptr = &Module[channel].ibuf.cont;
	if (done < len)
	{
		int           n = read (Module[channel].fd, ptr + done, len - done);

		if (n == 0 || (n == -1 && errno != EINTR && errno != EAGAIN))
		{
			KillModule (channel, 107 + n);
			return -1;
		}
		if (n == -1)
			return 0;
		Module[channel].ibuf.done += n;
		if (done + n < len)
			return 0;
	}

	/* check continue to make sure we're still talking properly */
	if (Module[channel].ibuf.cont != 1)
		KillModule (channel, 104);

	/* done reading command */
	Module[channel].ibuf.done = 0;

	/* set window id */
	w = Module[channel].ibuf.window;

	/* null-terminate the command line */
	text = Module[channel].ibuf.text;
	text[Module[channel].ibuf.size] = '\0';

	if (strlen (text))
		return RunCommand (text, channel, w);

	return 0;
}

void
DeadPipe (int nonsense)
{
	signal (SIGPIPE, DeadPipe);
}


void
KillModule (int channel, int place)
{
	if (Module[channel].fd > 0)
		close (Module[channel].fd);

	Module[channel].fd = -1;
	Module[channel].active = -1;

	while (Module[channel].output_queue != NULL)
		DeleteQueueBuff (channel);
	if (Module[channel].name != NULL)
	{
		free (Module[channel].name);
		Module[channel].name = NULL;
	}
	Module[channel].ibuf.len = 0;
	Module[channel].ibuf.done = 0;
	if (Module[channel].ibuf.text != NULL)
	{
		free (Module[channel].ibuf.text);
		Module[channel].ibuf.text = NULL;
	}
}

void
KillModuleByName (char *name)
{
	int           i;

	if (name == NULL)
		return;

	for (i = 0; i < npipes; i++)
	{
		if (Module[i].fd > 0)
		{
			if ((Module[i].name != NULL) && (strcmp (name, Module[i].name) == 0))
				KillModule (i, 20);
		}
	}
	return;
}

void
SendPacket (int module, unsigned long event_type, unsigned long num_datum, ...)
{
	int           i;
	va_list       ap;
	unsigned long *body;

	body = safemalloc ((3 + num_datum) * sizeof (unsigned long));

	body[0] = START_FLAG;
	body[1] = event_type;
	body[2] = 3 + num_datum;

	va_start (ap, num_datum);
	for (i = 0; i < num_datum; i++)
		body[3 + i] = va_arg (ap, unsigned long);

	va_end (ap);

    if( module >= 0 )
        PositiveWrite (module, body, body[2] * sizeof (unsigned long));
    else
        for (i = 0; i < npipes; i++)
            PositiveWrite (i, body, body[2] * sizeof (unsigned long));

	free (body);
}

void
SendConfig (int module, unsigned long event_type, ASWindow * t)
{
    int frame_x = 0, frame_y = 0, frame_width = 0, frame_height = 0;
    Window icon_title_w = None, icon_pixmap_w = None ;

    if( t->frame_canvas )
    {
        frame_x = t->frame_canvas->root_x ;
        frame_y = t->frame_canvas->root_y ;
        frame_width = t->frame_canvas->width ;
        frame_height = t->frame_canvas->height ;
    }

    if( t->icon_canvas )
        icon_pixmap_w = t->icon_canvas->w ;
    if( t->icon_title_canvas && t->icon_title_canvas != t->icon_canvas )
        icon_title_w = t->icon_title_canvas->w ;


    SendPacket (module, event_type, 24, t->w, t->frame, (unsigned long)t,
                frame_x, frame_y, frame_width, frame_height,
                ASWIN_DESK(t), t->status->flags, t->hints->flags, 0,
				t->hints->base_width, t->hints->base_height, t->hints->width_inc,
				t->hints->height_inc, t->hints->min_width, t->hints->min_height,
                t->hints->max_width,  t->hints->max_height,
                icon_title_w,         icon_pixmap_w, t->hints->gravity, 0xFFFFFFFF, 0x00000000);
}


void
BroadcastConfig (unsigned long event_type, ASWindow * t)
{
    SendConfig( -1, event_type, t );
}


void
SendName (int module, unsigned long event_type,
		  unsigned long data1, unsigned long data2, unsigned long data3, char *name)
{
    int           l, i;
	unsigned long *body;

	if (name == NULL)
		return;
	l = strlen (name) / (sizeof (unsigned long)) + 7;
	body = (unsigned long *)safemalloc (l * sizeof (unsigned long));

	body[0] = START_FLAG;
	body[1] = event_type;
	body[2] = l;

	body[3] = data1;
	body[4] = data2;
	body[5] = data3;
	strcpy ((char *)&body[6], name);

    if( module <0 )
        for (i = 0; i < npipes; i++)
            PositiveWrite (i, (unsigned long *)body, l * sizeof (unsigned long));
    else
        PositiveWrite (module, (unsigned long *)body, l * sizeof (unsigned long));

	free (body);
}

/* Low level messaging queue handling :                                        */
/*******************************************************************************/

#include <sys/errno.h>
AFTER_INLINE int
PositiveWrite (int module, unsigned long *ptr, int size)
{
	if ((Module[module].active < 0) || (!((Module[module].mask) & ptr[1])))
		return -1;

	AddToQueue (module, ptr, size, 0);

	if (Module[module].mask & M_LOCKONSEND)
	{
		int           e;

		FlushQueue (module);

		do
		{
			e = HandleModuleInput (module);
		}
		while (e != 66 && e != -1);
	}

	return size;
}

void
AddToQueue (int module, unsigned long *ptr, int size, int done)
{
	struct queue_buff_struct *c, *e;
	unsigned long *d;

	c = (struct queue_buff_struct *)safemalloc (sizeof (struct queue_buff_struct));
	c->next = NULL;
	c->size = size;
	c->done = done;
	d = (unsigned long *)safemalloc (size);
	c->data = d;
	memcpy (d, ptr, size);

	e = Module[module].output_queue;
	if (e == NULL)
	{
		Module[module].output_queue = c;
		return;
	}
	while (e->next != NULL)
		e = e->next;
	e->next = c;
}

void
DeleteQueueBuff (int module)
{
	struct queue_buff_struct *a;

	if (Module[module].output_queue == NULL)
		return;
	a = Module[module].output_queue;
	Module[module].output_queue = a->next;
	free (a->data);
	free (a);
	return;
}

void
FlushQueue (int module)
{
	char         *dptr;
	struct queue_buff_struct *d;
	int           a;
	extern int    errno;

	if ((Module[module].active <= 0) || (Module[module].output_queue == NULL))
		return;

	while (Module[module].output_queue != NULL)
	{
		int           fd = Module[module].fd;

		d = Module[module].output_queue;
		dptr = (char *)d->data;
		for (a = 0; d->done < d->size; d->done += a)
		{
			a = write (fd, &dptr[d->done], d->size - d->done);
			if (a < 0)
				break;
		}
		/* the write returns EWOULDBLOCK or EAGAIN if the pipe is full.
		 * (This is non-blocking I/O). SunOS returns EWOULDBLOCK, OSF/1
		 * returns EAGAIN under these conditions. Hopefully other OSes
		 * return one of these values too. Solaris 2 doesn't seem to have
		 * a man page for write(2) (!) */
		if (a < 0 && (errno == EWOULDBLOCK || errno == EAGAIN || errno == EINTR))
			return;
		else if (a < 0)
		{
			KillModule (module, 123);
			return;
		}
		DeleteQueueBuff (module);
	}
}

#endif
