/*
 * Copyright (C) 1999,2001 Sasha Vasko   <sasha at aftercode.net>
 * Copyright (C) 1999 Ethan Fischer <allanon@crystaltokyo.com>
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
 */

#include "config.h"

#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include <sys/file.h>
#include <sys/stat.h>						   /* for chmod() */


#include "astypes.h"
#include "safemalloc.h"
#include "aslist.h"
#include "output.h"
#include "socket.h"
#include "audit.h"

/***************************************************************************************/
/* socket setup code :		 														   */
/***************************************************************************************/

int
socket_connect_client (const char *socket_name)
{
	int           fd;

    if( socket_name == NULL )
        return -1;

	/* create an unnamed socket */
	if ((fd = socket (AF_UNIX, SOCK_STREAM, 0)) < 0)
	{
        show_error("unable to create UNIX socket: ");
		perror ("");
	}

	/* connect to the named socket */
	if (fd >= 0)
	{
		struct sockaddr_un name;

		name.sun_family = AF_UNIX;
		strncpy (name.sun_path, socket_name, sizeof(name.sun_path)-1 );
		name.sun_path[sizeof(name.sun_path)-1] = '\0' ;

		if (connect (fd, (struct sockaddr *)&name, sizeof (struct sockaddr_un)))
		{
            show_error ("unable to connect to socket '%s': ", name.sun_path);
			perror ("");
			close (fd);
			fd = -1;
		}
	}

	return fd;
}

int
socket_listen (const char *socket_name)
{
	int           fd;

	/* create an unnamed socket */
	if ((fd = socket (AF_UNIX, SOCK_STREAM, 0)) < 0)
        show_system_error("unable to create UNIX socket");

	/* remove the socket file */
	if (fd >= 0)
	{
		if (unlink (socket_name) == -1 && errno != ENOENT)
		{
            show_system_error("unable to delete file '%s'", socket_name);
			close (fd);
			fd = -1;
		}
	}

	/* bind the socket to a name */
	if (fd >= 0)
	{
		struct sockaddr_un name;

		name.sun_family = AF_UNIX;
		strncpy (name.sun_path, socket_name, sizeof(name.sun_path)-1 );
		name.sun_path[sizeof(name.sun_path)-1] = '\0' ;

		if (bind (fd, (struct sockaddr *)&name, sizeof (struct sockaddr_un)) == -1)
		{
            show_system_error("unable to bind socket to name '%s'", socket_name);
			close (fd);
			fd = -1;
		}
	}

	/* set file permissions to 0700 (read/write/execute by owner only) */
	if (fd >= 0)
	{
		if (chmod (socket_name, 0700) == -1)
		{
            show_system_error("unable to set socket permissions to 0700");
			close (fd);
			fd = -1;
		}
	}

	/* start listening for incoming connections */
	if (fd >= 0)
	{
		if (listen (fd, 254) == -1)
		{
            show_system_error("unable to listen on socket");
			close (fd);
			fd = -1;
		}
	}

	/* set non-blocking I/O mode */
	if (fd >= 0)
	{
		if (fcntl (fd, F_SETFL, fcntl (fd, F_GETFL) | O_NONBLOCK) == -1)
		{
            show_system_error("unable to set non-blocking I/O");
			close (fd);
			fd = -1;
		}
	}

	/* mark as close-on-exec so other programs won't inherit the socket */
	if (fd >= 0)
	{
		if (fcntl (fd, F_SETFD, 1) == -1)
		{
            show_system_error("unable to set close-on-exec for socket");
			close (fd);
			fd = -1;
		}
	}

	return fd;
}


/***************************************************************************************/
/* buffered write operations : 														   */
/***************************************************************************************/
void
socket_buffered_write (ASSocketBuffer *sb, const void *data, int size)
{

	if( sb == NULL || sb->fd < 0 )
		return;

	if (data == NULL || size == 0)
	{
		write (sb->fd, &(sb->buffer[0]), sb->bytes_in);
		sb->bytes_in = 0;
	} else if (size > AS_SOCK_BUFFER_SIZE - sb->bytes_in)
	{
		if (sb->bytes_in > 0)
		{
			write (sb->fd, &(sb->buffer[0]), sb->bytes_in);
			sb->bytes_in = 0;
		}
		write (sb->fd, data, size);
	} else
	{
		memcpy (&(sb->buffer[sb->bytes_in]), data, size);
		sb->bytes_in += size;
		if (sb->bytes_in == AS_SOCK_BUFFER_SIZE)
		{
			write (sb->fd, &(sb->buffer[0]), sb->bytes_in);
			sb->bytes_in = 0;
		}
	}
}

void
socket_write_flush ( ASSocketBuffer *sb )
{
	socket_buffered_write (sb, NULL, 0);			   /* flashing buffer */
}

/* we have to consider endiannes of the data while writing into the socket :
 * network format is Big Endian !
 */
void
socket_write_int32 (ASSocketBuffer *sb, CARD32 *data, size_t items )
{
#ifndef TRANSLATE_HOST_TO_NET
	socket_buffered_write( sb, data, items*sizeof(CARD32));
#else
	CARD32  buf ;
	register int i = 0;
	while( i < items )
	{
		buf = as_hlton(data[i]);
		socket_buffered_write( sb, &buf, sizeof(CARD32));
        ++i;
	}
#endif
}

void
socket_write_int16 (ASSocketBuffer *sb, CARD16 *data, size_t items )
{
#ifndef TRANSLATE_HOST_TO_NET
	socket_buffered_write( sb, data, items*sizeof(CARD16));
#else
	CARD16  buf ;
	register int i = 0;
	while( i < items )
	{
		buf = as_hlton16(data[i]);
		socket_buffered_write( sb, &buf, sizeof(CARD16));
        ++i;
	}
#endif
}

/* strings are transferred as <bytes_count><byte1, byte2, ...>
 * bytes_count = 0 means string is empty
 */
void
socket_write_string (ASSocketBuffer *sb, const char *string)
{
	if (sb && sb->fd >= 0)
	{
		CARD32        len = 0;

		if (string != NULL)
			len = strlen (string);
		socket_write_int32 (sb, &len, 1);
		if (len)
			socket_buffered_write (sb, string, len);
	}
}

/**********************************************************************/
/* More complex signal safe and overflow safe socket write operations (FIFO): */
/**********************************************************************/

ASFIFOPacket *
form_fifo_packet( CARD8* buffer, size_t size )
{
	if( size > 0 && buffer )
	{
		register ASFIFOPacket *p = safecalloc( 1, sizeof(ASFIFOPacket) );
		p->buffer = safemalloc( size );
		memcpy( p->buffer, buffer, size );
		p->bytes_in = size ;
		return p;
	}
	return NULL;
}

void
dereference_packet( ASFIFOPacket **ppacket )
{
	ASFIFOPacket *p = *ppacket ;
	if( p )
		if( --(p->ref_count) )
		{
			free( p->buffer );
			free( p );
			*ppacket = NULL ;
		}
}

void
remove_fifo_packet( void *data )
{
	ASFIFOPacket *p = data ;
	dereference_packet( &p );
}

ASFIFOQueue  *
create_fifo( int fd )
{
	ASFIFOQueue  *fifo = NULL ;
	if( fd >= 0 )
	{
		fifo = safecalloc( 1, sizeof(ASFIFOQueue) );
		fifo->fd = fd ;
		fifo->queue.destroy_func = remove_fifo_packet ;
	}
	return fifo;
}

inline void
purge_fifo_queue( ASFIFOQueue *fifo )
{
	if( fifo )
		purge_asbidirlist( &(fifo->queue) );
}

size_t
send_fifo_queue( ASFIFOQueue *fifo )
{
	size_t written = 0 ;
	ASBiDirElem *curr ;
	while( (curr = LIST_START(&(fifo->queue))) != NULL  )
	{


	}
	return written;
}

void
destroy_fifo_queue( ASFIFOQueue **pfifo)
{
	ASFIFOQueue *p = *pfifo;
	if( p )
	{
		purge_fifo_queue( p );
		free( p );
	}
}


/***************************************************************************************/
/* signal safe socket read operations : 											   */
/***************************************************************************************/

ASProtocolResult
socket_read_proto_item( ASProtocolState *ps )
{
	int curr = ps->curr_item ;
	int type = ps->specs->items[curr].type ;
	register ASProtocolItem *item = &(ps->items[curr]);

	if( item->size == 0 )
	{
		if( type > 0 )
		{
			item->size = ps->specs->items[curr].max_size ;
			item->size_bytes = item->size * type;
		}else
		{
			item->size = 1;
			item->size_bytes = 4 ;
		}
	}
	if( item->bytes_allocated < item->size_bytes )
	{
		item->d.memory = realloc( item->d.memory, item->size_bytes );
		item->bytes_allocated = item->size_bytes ;
	}
	while ( item->size_bytes > item->bytes_read )
	{
		time_t curr_time;
		int res = read( ps->fd, (CARD8*)(item->d.memory)+item->bytes_read, item->size_bytes - item->bytes_read );
		curr_time = time(NULL);
		ps->last_read_time = curr_time;
		if( res > 0 )
			item->bytes_read += res ;
		else if( errno != EINTR )
		{
			if( errno == EAGAIN )
			{
				if( ps->last_read_time > 0 && curr_time > ps->last_read_time )
					if( curr_time - ps->last_read_time > ps->specs->timeout )
						return ASP_Timeout ;
				return ASP_WaitData;
			}
			return ASP_SocketError ;
		}
	}
#ifdef TRANSLATE_HOST_TO_NET
	switch( item->size_bytes / item->size  )
	{
		case 2 :
			{
				int i ;
				for( i = 0 ; i < item->size ; i++ )
					item->d.int16[i] = as_ntohl16(item->d.int16[i]);
			}
			break ;
		case 4 :
			{
				int i ;
				for( i = 0 ; i < item->size ; i++ )
					item->d.int32[i] = as_ntohl(item->d.int32[i]);
			}
		    break ;
	}
#endif
	if( type == AS_PROTOCOL_ITEM_STRING && item->size == 1 && item->size_bytes == 4 )
	{
		if( (item->size = item->d.int32[0]) > ps->specs->items[curr].max_size )
			return ASP_BadData;
		item->size_bytes = item->size ;
		item->bytes_read = 0 ;
		return socket_read_proto_item( ps );
	}
	return ASP_Success;
}

ASProtocolResult
socket_read_proto( ASProtocolState *ps )
{
	if( ps == NULL || ps->fd < 0 )
		return ASP_SocketError;

	if( ps->items[ps->curr_item].size_bytes == ps->items[ps->curr_item].bytes_read )
	{
		if( ++ps->curr_item >= ps->specs->items_num  )
			ps->curr_item = 0 ;
		ps->items[ps->curr_item].size = 0 ;
		ps->items[ps->curr_item].size_bytes = 0 ;
		ps->items[ps->curr_item].bytes_read = 0 ;
	}

	return socket_read_proto_item( ps );
}

void
socket_read_proto_reset( ASProtocolState *ps )
{
	if( ps )
	{
		ps->curr_item = 0 ;
		ps->items[0].size = 0 ;
		ps->items[0].size_bytes = 0 ;
		ps->items[0].bytes_read = 0 ;
	}
}

void *
socket_read_steal_buffer( ASProtocolState *ps )
{
	void *ptr = NULL;
	if( ps )
	{
		register ASProtocolItem *item = &(ps->items[ps->curr_item]);
		if( item->bytes_allocated > 0 )
		{
			ptr = item->d.memory ;
			item->d.memory = NULL ;
			item->bytes_allocated = 0 ;
		}
	}
	return ptr;
}

