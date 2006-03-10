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

#define LOCAL_DEBUG

#include "../../configure.h"

#include "asinternals.h"

#include <signal.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/stat.h>						   /* for chmod() */
#include <sys/types.h>
#include <sys/un.h>							   /* for struct sockaddr_un */

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

#include <X11/keysym.h>

#include "../../libAfterStep/desktop_category.h"
#include "../../libAfterStep/module.h"
#include "../../libAfterStep/wmprops.h"

static DECL_VECTOR(send_data_type, module_output_buffer);

static void        DeleteQueueBuff (module_t *module);
static void       AddToQueue      (module_t *module, send_data_type *ptr, int size, int done);

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
	if( access( tmpdir, W_OK ) != 0 )
	    if( (tmpdir = getenv( "HOME" )) == NULL )
		return False ;

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
KillModule (module_t *module, Bool dont_free_memory)
{
LOCAL_DEBUG_OUT( "module %p ", module );
LOCAL_DEBUG_OUT( "module name \"%s\"", module->name );
    if (module->fd > 0)
        close (module->fd);

	if( !dont_free_memory )
	{
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
	}else
	{
		module->output_queue = NULL ;
		module->name = NULL ;
		module->ibuf.text = NULL ;
		module->ibuf.func = NULL ;
	}

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
CheckCmdTextSize (module_t *module, CARD32 * size, CARD32 * curr_len, char **text)
{
	/* max command length is 1024 */
	if (*size > 1024)
	{
        show_error("command from module '%s' is too big (%d)", module->name, *size);
		*size = 1024;
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
    int           res = 1;
    Bool          invalid_func = False;
    register module_ibuf_t *ibuf ;

    ibuf = &(module->ibuf);

    /* read a window id */
    res = ReadModuleInput (module, &offset, sizeof (ibuf->window), &(ibuf->window));
	if (res > 0)
	{
        module->active = 1;
        res = ReadModuleInput (module, &offset, sizeof (ibuf->size), &(ibuf->size));
	}

LOCAL_DEBUG_OUT("res(%d)->window(0x%lX)->size(%ld)",res, ibuf->window, ibuf->size);
	if (res > 0)
	{
		if (ibuf->size > 0)					   /* Protocol 1 */
		{
LOCAL_DEBUG_OUT("Incoming message in proto 1%s","");
            CheckCmdTextSize (module, &(ibuf->size), &(ibuf->len), &(ibuf->text));
            res = ReadModuleInput (module, &offset, ibuf->size, ibuf->text);

            if (res > 0)
			{
				/* null-terminate the command line */
				ibuf->text[ibuf->size] = '\0';
                ibuf->func = String2Func (ibuf->text, ibuf->func, False);
                invalid_func = (ibuf->func== NULL);
            }
		} else								   /* Protocol 2 */
		{
            /* for module->afterstep communications - 32 bit values are always used : */
            CARD32        curr_len;
            CARD32        tmp32, tmp32_val[2] = {0,0}, tmp32_unit[2] = {0,0} ;
			register FunctionData *pfunc = ibuf->func;

LOCAL_DEBUG_OUT("Incoming message in proto 2%s","");
            if (pfunc == NULL)
			{
				pfunc = (FunctionData *) safecalloc (1, sizeof (FunctionData));
				init_func_data (pfunc);
				ibuf->func = pfunc;
			}

            res = ReadModuleInput (module, &offset, sizeof (CARD32), &tmp32);
            pfunc->func = tmp32 ;

			if (res > 0)
			{
				if (!IsValidFunc (pfunc->func))
				{
					res = 0;
					ibuf->done = 0;
                    invalid_func = True;
				} else
                    res = ReadModuleInput (module, &offset, sizeof (ibuf->name_size), &(ibuf->name_size));
			}
			if (res > 0 && ibuf->name_size > 0)
			{
				curr_len = (pfunc->name) ? strlen (pfunc->name) + 1 : 0;
                CheckCmdTextSize (module, &(ibuf->name_size), &curr_len, &(pfunc->name));
                res = ReadModuleInput (module, &offset, ibuf->name_size, pfunc->name);
				pfunc->name[ibuf->name_size] = '\0';
			}
            LOCAL_DEBUG_OUT( "name_size = %ld, pfunc->name = %p", ibuf->name_size, pfunc->name );
            if (res > 0)
                res = ReadModuleInput (module, &offset, sizeof (ibuf->text_size), &(ibuf->text_size));

			if (res > 0 && ibuf->text_size > 0)
			{
				curr_len = (pfunc->text) ? strlen (pfunc->text) + 1 : 0;
                CheckCmdTextSize (module, &(ibuf->text_size), &curr_len, &(pfunc->text));
                res = ReadModuleInput (module, &offset, ibuf->text_size, pfunc->text);
				pfunc->text[ibuf->text_size] = '\0';
			}
            LOCAL_DEBUG_OUT( "text_size = %ld, pfunc->text = %p", ibuf->text_size, pfunc->text );
            if (res > 0)
			{	
                res = ReadModuleInput (module, &offset, sizeof (tmp32_val), &(tmp32_val[0]));
				if (res > 0)
				{	
    	        	pfunc->func_val[0] = tmp32_val[0] ;
					pfunc->func_val[1] = tmp32_val[1] ;
				}
			}


			if (res > 0)
			{	
                res = ReadModuleInput (module, &offset, sizeof (tmp32_unit), &(tmp32_unit[0]));
				if (res > 0)
				{	
					pfunc->unit_val[0] = tmp32_unit[0] ;
					pfunc->unit_val[1] = tmp32_unit[1] ;
				}
            }

			if (res > 0 && IsValidFunc (pfunc->func))
                invalid_func = False;
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
        KillModule (module, False);
    else if (res > 0)
    {
        ibuf->done = 0;                        /* done reading command */
        if( invalid_func )
            res = -1 ;
    }
LOCAL_DEBUG_OUT( "result(%d)", res );
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
LOCAL_DEBUG_OUT( "deleting buffer %p sent to module %p - next %p ", a, module, a->next );
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
            LOCAL_DEBUG_OUT( "wrote %d bytes into the module %p pipe", bytes_written, module );
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
                KillModule (module, False);
                return -1;
            }
        }
        DeleteQueueBuff (module);
	}
    return 1;
}

void
FlushAllQueues()
{
	fd_set        out_fdset;
	int           retval = -1;
	struct timeval tv;
	struct timeval *t = NULL;

	do
	{
		int           max_fd = -1;
        register int i = MIN(MODULES_NUM,Module_npipes) ;
        register module_t *list = MODULES_LIST ;

		FD_ZERO (&out_fdset);

		while( --i >= 0 )
		{
			if (list[i].fd >= 0 )
			{

				int res = 0;
				if( list[i].output_queue && (retval < 0 || FD_ISSET (list[i].fd, &out_fdset)))
					FlushQueue (&(list[i]));
				if( res >= 0 && list[i].output_queue != NULL)
				{
					FD_SET (list[i].fd, &out_fdset);
					if (max_fd < list[i].fd)
						max_fd = list[i].fd;
				}
			}
		}

		if( max_fd < 0 )
			return ;/* no more output left */

		tv.tv_sec = 0 ;
		tv.tv_usec = 15000 ;
		t = &tv ;
	    retval = PORTABLE_SELECT(min (max_fd + 1, fd_width),NULL,&out_fdset,NULL,t);
		if (retval <= 0)
			return ;
	}while(1);
}




#include <sys/errno.h>
static inline int
PositiveWrite (unsigned int channel, send_data_type *ptr, int size)
{
    module_t *module = &(MODULES_LIST[channel]);
    register CARD32 mask = ptr[1] ;

    LOCAL_DEBUG_OUT( "module(%p)->active(%d)->module_mask(0x%lX)->mask(0x%lX)", module, module->active, module->mask, mask );
    if (module->active < 0 || !get_flags(module->mask, mask))
		return -1;

	AddToQueue (module, ptr, size, 0);
    if (get_flags(module->lock_on_send_mask, mask) && !is_server_grabbed() )
	{
        int           res ;
		int wait_count = 0 ; 
        do
        {
            if( (res = FlushQueue (module)) >= 0 )
			{
				sleep_a_millisec(10);/* give it some time to react */	  
                res = HandleModuleInput (module);
            }
			if ( res > 0  ) /* need to run command */
            {
				LOCAL_DEBUG_OUT( "replay received while waiting for UNLOCK, func = %ld, F_UNLOCK = %d", module->ibuf.func->func, F_UNLOCK );
                if( module->ibuf.func->func == F_UNLOCK )
                    return size;
                RunCommand (module->ibuf.func, channel, module->ibuf.window);
            }
			sleep_a_millisec(10);/* give it some time to react */
			++wait_count ;
			/* module has no more then 20 seconds to unlock us */
        }while( res >= 0 && wait_count < 2000 );
	}
	return size;
}

static send_data_type *
make_msg_header( send_data_type msg_type, send_data_type size )
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
    send_data_type size_to_send ;

    size_to_send = b[2];
    if( size_to_send > 0 && Modules )
	{
		/* lets make sure that we will not overrun the buffer : */
        realloc_vector(&module_output_buffer, size_to_send );
        b = VECTOR_HEAD(send_data_type,module_output_buffer);
LOCAL_DEBUG_OUT( "sending %ld words to module # %d of %d", size_to_send, channel, MODULES_NUM );
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
ShutdownModules(Bool dont_free_memory)
{
    if (Modules != NULL)
    {
        register int i = MODULES_NUM;
        register module_t *list = MODULES_LIST ;
LOCAL_DEBUG_OUT( "pid(%d),total modules %d. list starts at %p", getpid(), MODULES_NUM, MODULES_LIST );
        while(--i >= 0 )
            KillModule ( &(list[i]), dont_free_memory );
		if( !dont_free_memory )
		{
LOCAL_DEBUG_OUT( "pid(%d),destroy_asvector", getpid() );
        	destroy_asvector(&Modules);
LOCAL_DEBUG_OUT( "pid(%d),free_vector", getpid() );
			free_vector( &module_output_buffer );
LOCAL_DEBUG_OUT( "pid(%d),modules are down", getpid() );
		}
        Modules = NULL;
    }
}

void
SetupModules(void)
{
    if( Modules )
        ShutdownModules(False);

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
    if ((cmd = findIconFile (module, Environment->module_path, X_OK)) == NULL)
	{
        show_error("no such module %s in path %s\n", module, Environment->module_path);
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
LOCAL_DEBUG_OUT( "module %d has %s input and %s output", channel, has_input?"":"no", has_output?"":"no" );
        if( has_input )
        {
            res = HandleModuleInput( module );
            if( res < 0 )
                has_output = 0 ;
            else if ( res > 0 ) /* need to run command */
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


int FindModuleByName (char *name)
{
	int module = -1;
	if( Modules ) 
	{	
    	wild_reg_exp  *wrexp = compile_wild_reg_exp( name );

    	if ( wrexp != NULL )
    	{
        	register int i = MODULES_NUM;
        	register module_t *list = MODULES_LIST ;

        	while( --i >= 0 )
            	if (list[i].fd > 0)
            	{
                	LOCAL_DEBUG_OUT( "checking if module %d \"%s\" matches regexp \"%s\"", i, list[i].name, name);
                	if (match_wild_reg_exp( list[i].name, wrexp ) == 0 )
					{	
                    	module = i ; 
						break;
					}
            	}
        	destroy_wild_reg_exp( wrexp );
		}
    }
	return module;
}

char *
GetModuleCmdLineByName(char *name)
{
	int module = FindModuleByName (name);
	if( module >= 0 ) 
	{	
		register module_t *list = MODULES_LIST ;	
		if( list[module].cmd_line ) 
			return mystrdup( list[module].cmd_line ) ;
		else
			return mystrdup( list[module].name );
	}
	return NULL ;
}

void
KillModuleByName (char *name)
{
	int module = FindModuleByName (name);
	if( module >= 0 ) 
	{	
		register module_t *list = MODULES_LIST ;	
        KillModule (&(list[module]), False);
	}
}

void
KillAllModulesByName (char *name)
{
	int module ;
	while( (module = FindModuleByName (name)) >= 0 )
	{	
		register module_t *list = MODULES_LIST ;	
        KillModule (&(list[module]), False);
	}
}

void
SendPacket ( int channel, send_data_type msg_type, send_data_type num_datum, ...)
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
SendConfig (int module, send_data_type event_type, ASWindow * t)
{
    send_signed_data_type 	frame_x = 0, frame_y = 0, frame_width = 0, frame_height = 0;
    send_ID_type 			icon_title_w = None, icon_pixmap_w = None ;
    send_signed_data_type 	icon_x = 0, icon_y = 0, icon_width = 0, icon_height = 0 ;
	union {
		ASWindow *asw ;
		send_data_type id ;
	}asw_id;

    if( t->frame_canvas )
    {
        frame_x = t->frame_canvas->root_x ;
        frame_y = t->frame_canvas->root_y ;
        if( !ASWIN_GET_FLAGS(t, AS_Sticky) )
        {
            frame_x += t->status->viewport_x ;
            frame_y += t->status->viewport_y ;
        }
        frame_width = t->frame_canvas->width+t->frame_canvas->bw*2 ;
        frame_height = t->frame_canvas->height+t->frame_canvas->bw*2 ;
    }

    if( t->icon_canvas )
        icon_pixmap_w = t->icon_canvas->w ;
    if( t->icon_title_canvas && t->icon_title_canvas != t->icon_canvas )
        icon_title_w = t->icon_title_canvas->w ;

    if (ASWIN_GET_FLAGS(t,AS_Iconic))
    {
        ASCanvas *ic = t->icon_canvas?t->icon_canvas:t->icon_title_canvas ;
        if( ic != NULL )
        {
            icon_x = ic->root_x ;
            icon_y = ic->root_y ;
            icon_width = ic->width+ic->bw*2 ;
            icon_height = ic->height+ic->bw*2;
        }
    }

	asw_id.asw = t ;
    SendPacket (module, event_type, 26,
                t->w,                 t->frame,              asw_id.id,
                frame_x,              frame_y,               frame_width,         frame_height,
                ASWIN_DESK(t),        t->status->flags,      t->hints->flags,     t->hints->client_icon_flags,
                t->hints->base_width, t->hints->base_height, t->hints->width_inc, t->hints->height_inc,
                t->hints->min_width,  t->hints->min_height,  t->hints->max_width,  t->hints->max_height,
                t->hints->gravity,    icon_title_w,          icon_pixmap_w,
                icon_x,               icon_y,                icon_width,           icon_height );
}

void
SendString ( int channel, send_data_type msg_type,
             Window w, Window frame, ASWindow *asw_ptr,
			 char *string, send_data_type encoding )
{
    send_data_type 			data[3];
    send_signed_data_type 	len = 0;
	union {
		ASWindow *asw ;
		send_data_type id ;
	}asw_id;

    if (string == NULL)
		return;
    len = strlen(string);

    flush_vector( &module_output_buffer );
    append_vector( &module_output_buffer,
                    make_msg_header(msg_type,3+1+(len>>2)+1+MSG_HEADER_SIZE),
                    MSG_HEADER_SIZE );
    data[0] = w;
    data[1] = frame;
	asw_id.asw = asw_ptr ;
	data[2] = asw_id.id;
    append_vector( &module_output_buffer, &(data[0]), 3);
    append_vector( &module_output_buffer, &encoding, 1);
    serialize_string( string, &module_output_buffer );
    SendBuffer( channel );
}

void
SendVector (int channel, send_data_type msg_type, ASVector *vector)
{
    if (vector == NULL )
		return;

    flush_vector( &module_output_buffer );
    append_vector( &module_output_buffer,
                    make_msg_header(msg_type,VECTOR_USED(*vector)),
                    MSG_HEADER_SIZE );
    append_vector( &module_output_buffer, VECTOR_HEAD(send_data_type,*vector), VECTOR_USED(*vector));

    SendBuffer( channel );
}

void
SendStackingOrder (int channel, send_data_type msg_type, send_data_type desk, ASVector *ids)
{
    send_data_type data[2];

    if (ids == NULL )
		return;

    flush_vector( &module_output_buffer );
    append_vector( &module_output_buffer,
                    make_msg_header(msg_type,VECTOR_USED(*ids)+2),
                    MSG_HEADER_SIZE );
    data[0] = desk ;
    data[1] = VECTOR_USED(*ids);
    append_vector( &module_output_buffer, &(data[0]), 2);
    append_vector( &module_output_buffer, VECTOR_HEAD(send_data_type,*ids), VECTOR_USED(*ids));

    SendBuffer( channel );
}

/* this will run command received from module */
void
RunCommand (FunctionData * fdata, unsigned int channel, Window w)
{
	ASWindow     *tmp_win;
    int           toret = 0;
    module_t     *module;

    LOCAL_DEBUG_CALLER_OUT( "fdata(%p)->func(%ld,MOD_FS=%d)->channel(%d)->w(%lX)->Modules(%p)", fdata, fdata->func, F_MODULE_FUNC_START, channel, w, Modules );
/*fprintf( stderr,"Function parsed: [%s] [%s] [%d] [%d] [%c]\n",fdata.name,fdata.text,fdata.func_val[0], fdata.func_val[1] );
 */
    if (Modules == NULL || fdata == NULL || channel >= MODULES_NUM )
        return;
    if (!IsValidFunc(fdata->func))
        return;
    module = &(MODULES_LIST[channel] );
    switch (fdata->func)
	{
	 case F_SET_MASK:
         module->mask = fdata->func_val[0];
		 module->lock_on_send_mask = fdata->func_val[1];
		 break;
	 case F_SET_NAME:
        set_string( &(module->name), fdata->name );
        fdata->name = NULL;
		if( fdata->text ) 
		{	
	        set_string( &(module->cmd_line), fdata->text );
    	    fdata->text = NULL;
		}
        break;
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
                 broadcast_config (M_CONFIGURE_WINDOW, tmp_win);
			 break;
		 }
	 default:
        {
            ASEvent event;
			Bool defered = False ;
            memset( &event, 0x00, sizeof(ASEvent) );
            event.w = w;
            if ((event.client = window2ASWindow(w)) == NULL )
            {
                event.w = None;
                event.x.xbutton.x_root = 0;
                event.x.xbutton.y_root = 0;
            } else
            {
                event.x.xbutton.x_root = event.client->frame_canvas->root_x+1;
                event.x.xbutton.y_root = event.client->frame_canvas->root_y+1;
            }
			
            event.x.xany.type = ButtonRelease;
            event.x.xbutton.button = 1;
            event.x.xbutton.x = 0;
            event.x.xbutton.y = 0;
            event.x.xbutton.subwindow = None;
            event.context = C_FRAME;
            event.scr = ASDefaultScr ;
            event.event_time = Scr.last_Timestamp ;
            /* there must be no deffering on module commands ! */

			defered = ( w != None || !IsWindowFunc (fdata->func) );
            ExecuteFunctionExt (fdata, &event, channel, defered);
        }
    }
    free_func_data (fdata);
}

/*******************************************************************************/
/* usefull functions to simplify life in other places :                        */
/*******************************************************************************/
void
broadcast_focus_change( ASWindow *asw, Bool focused )
{
	union {
		ASWindow *asw ;
		send_data_type id ;
	}asw_id;

	asw_id.asw = asw ;
	if( asw )
        SendPacket(-1, M_FOCUS_CHANGE, 4, asw->w, asw->frame, asw_id.id, (send_data_type)focused);
}

void
broadcast_window_name( ASWindow *asw )
{
    if( asw )
	{	
        SendString( -1, M_WINDOW_NAME, asw->w, asw->frame,
                    asw, ASWIN_NAME(asw), get_hint_name_encoding(asw->hints, 0));
        SendString( -1, M_WINDOW_NAME_MATCHED, asw->w, asw->frame,
                    asw, asw->hints->matched_name0, asw->hints->matched_name0_encoding );
	}
}

void
broadcast_icon_name( ASWindow *asw )
{
    if( asw )
	{
        SendString( -1, M_ICON_NAME, asw->w, asw->frame,
                    asw, ASWIN_ICON_NAME(asw), get_hint_name_encoding(asw->hints, asw->hints->icon_name_idx));
	}
}

void
broadcast_res_names( ASWindow *asw )
{
    if( asw )
    {
        SendString( -1, M_RES_CLASS, asw->w, asw->frame,
                    asw, asw->hints->res_class, get_hint_name_encoding(asw->hints, asw->hints->res_class_idx));
        SendString( -1, M_RES_NAME, asw->w, asw->frame,
                    asw, asw->hints->res_name, get_hint_name_encoding(asw->hints, asw->hints->res_name_idx));
    }
}

void
broadcast_status_change( int message, ASWindow *asw )
{
	union {
		ASWindow *asw ;
		send_data_type id ;
	}asw_id;

	asw_id.asw = asw ;

    if( message == M_MAP )
        SendPacket( -1, M_MAP, 3, asw->w, asw->frame, asw_id.id);
}

void
broadcast_config (send_data_type event_type, ASWindow * t)
{
    SendConfig( -1, event_type, t );
}

/********************************************************************************/
/* module list menus regeneration :                                             */
/********************************************************************************/

static inline void
module_t2func_data( FunctionCode func, module_t *module, FunctionData *fdata, char *scut ) 
{
	fdata->func = func;
    fdata->name = mystrdup(module->name);
    fdata->text = mystrdup(module->name);
	if (++(*scut) == ('9' + 1))
		(*scut) = 'A';		/* Next shortcut key */
    fdata->hotkey = (*scut);
}	   

MenuData *
make_module_menu( FunctionCode func, const char *title, int sort_order )
{
    MenuData *md ;
    MenuDataItem *mdi ;
    FunctionData  fdata ;
    char          scut = '0';                  /* Current short cut key */
	module_t *modules ;
	int i, max_i ;

	if ( Modules == NULL ) 
        return NULL;
    
	if( (md = create_menu_data ("@#%module_menu%#@")) == NULL )
        return NULL;

	modules = MODULES_LIST ;
	max_i = MODULES_NUM ;

    memset(&fdata, 0x00, sizeof(FunctionData));
    fdata.func = F_TITLE ;
    fdata.name = mystrdup(title);
    add_menu_fdata_item( md, &fdata, NULL, NULL );

	if( sort_order == ASO_Alpha ) 
	{
		FunctionData **menuitems = safecalloc(sizeof(FunctionData *), max_i);
        
		for( i = 0 ; i < max_i ; ++i) 
		{
			menuitems[i] = safecalloc(1, sizeof(FunctionData));
			module_t2func_data( func, &(modules[i]), menuitems[i], &scut ); 
        }
		qsort(menuitems, i, sizeof(FunctionData *), compare_func_data_name);
		for( i = 0 ; i < max_i ; ++i) 
		{
			ASDesktopEntry *de ;
			char *minipixmap = NULL ; 
			de = fetch_desktop_entry( AfterStepCategories, menuitems[i]->name );
			if( de && de->fulliconname ) 
				minipixmap = de->fulliconname ;

			if( (mdi = add_menu_fdata_item( md, menuitems[i], minipixmap, NULL)) != NULL ) 
				set_flags( mdi->flags, MD_ScaleMinipixmapDown );
			safefree( menuitems[i] ); /* scrubba-dub-dub */
		}
		safefree(menuitems);
    }else /* if( sort_order == ASO_Circulation || sort_order == ASO_Stacking ) */
	{
		for( i = 0 ; i < max_i ; ++i) 
        {
			ASDesktopEntry *de ;
			char *minipixmap = NULL ; 
            module_t2func_data( func, &(modules[i]), &fdata, &scut ); 
			de = fetch_desktop_entry( AfterStepCategories, fdata.name );
			if( de && de->fulliconname ) 
				minipixmap = de->fulliconname ;
            if( (mdi = add_menu_fdata_item( md, &fdata, minipixmap, NULL )) != NULL ) 
				set_flags( mdi->flags, MD_ScaleMinipixmapDown );
        }
    }
    return md;
}


