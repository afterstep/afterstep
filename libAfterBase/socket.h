#ifndef AS_SOCKET_H_HEADER_INCLUDED
#define AS_SOCKET_H_HEADER_INCLUDED

#include "aslist.h"
struct ASBiDirList;

/* socket setup code :		 														   */
int socket_connect_client (const char *socket_name);
int socket_listen (const char *socket_name);

/* Network to local 32bit binary conversion : (Network is Big Endian) */

#ifdef WORDS_BIGENDIAN
#define as_ntohl(ui32)		(ui32)
#define as_hlton(ui32)		(ui32)
#define as_ntohl16(ui16)		(ui16)
#define as_hlton16(ui16)		(ui16)
#else
#define as_ntohl(ui32)		((((ui32)&0x000000FF)<<24)|(((ui32)&0x0000FF00)<<8)|(((ui32)&0x00FF0000)>>8)|(((ui32)&0xFF000000)>>24))
#define as_hlton(ui32)		as_ntohl(ui32)     /* conversion is symmetrical */
#define as_ntohl16(ui16)		((((ui16)&0x00FF)<<8)|(((ui16)&0xFF00)>>8))
#define as_hlton16(ui16)		as_ntohl(ui16)     /* conversion is symmetrical */
#endif

/* simple buffered write operations : 	*/
typedef struct ASSocketBuffer
{
	int fd ;
#define AS_SOCK_BUFFER_SIZE		2048           /* single page */
	int   bytes_in ;
	CARD8 buffer[AS_SOCK_BUFFER_SIZE] ;
}ASSocketBuffer;

void socket_buffered_write (ASSocketBuffer *sb, const void *data, int size);
void socket_write_int32 (ASSocketBuffer *sb, CARD32 *data, size_t items );
void socket_write_int16 (ASSocketBuffer *sb, CARD16 *data, size_t items );
void socket_write_string (ASSocketBuffer *sb, const char *string);
void socket_write_flush ( ASSocketBuffer *sb );

/* More complex signal safe and overflow safe socket write operations (FIFO): */
typedef struct ASFIFOPacket
{
	int 	ref_count ;          /* number of queues we are participating in */
	time_t  added_time ;         /* so we can implement per-packet timeouts */
	size_t  bytes_in, bytes_out ;
	CARD8  *buffer ;
}ASFIFOPacket;

typedef struct ASFIFOQueue
{
	/* output pipe : */
	int fd ;
	enum {
		ASFS_Empty = 0,
		ASFS_WritePending,
		ASFS_Closed
	} state ;
	/* actuall queue : */
	struct ASBiDirList  queue ;
}ASFIFOQueue;

ASFIFOPacket *form_fifo_packet( CARD8* buffer, size_t size );
void dereference_packet( ASFIFOPacket **ppacket );  /* destroys packet when its ref_count reaches 0 */

ASFIFOQueue *create_fifo( int fd );
size_t 		add_fifo_packet( ASFIFOQueue *fifo, ASFIFOPacket *packet );
size_t 		send_fifo( ASFIFOQueue *fifo );     /* attempts to send next packet */
void 		purge_fifo( ASFIFOQueue *fifo );
void 		destroy_fifo( ASFIFOQueue **pfifo);

/* signal safe socket read operations :   */

typedef struct ASProtocolItemSpec
{
#define AS_PROTOCOL_ITEM_NONE    -1
#define AS_PROTOCOL_ITEM_STRING   0
#define AS_PROTOCOL_ITEM_BYTE	  1
#define AS_PROTOCOL_ITEM_INT16	  2
#define AS_PROTOCOL_ITEM_INT32	  4
	int 	type ;                                 /* one of the above items */
	size_t  max_size ;
}ASProtocolItemSpec;

typedef struct ASProtocolSpec
{
	ASProtocolItemSpec *items ;             /* list terminated by element AS_PROTOCOL_ITEM_NONE */
	size_t 				items_num ;
	time_t              timeout ;
}ASProtocolSpec;

typedef struct ASProtocolItem
{
	size_t  size, size_bytes ;
	size_t  bytes_allocated, bytes_read;
	union {
		void   *memory;
		char   *string;
		CARD8  *byte  ;
		CARD16 *int16 ;
		CARD32 *int32 ;
	}d;
}ASProtocolItem;

typedef struct ASProtocolState
{
	struct ASProtocolSpec *specs;
	struct ASProtocolItem *items ;
	unsigned int 		   curr_item ;
	time_t                 last_read_time ;
	int 				   fd ;
}ASProtocolState;

/* returns : */
typedef enum {
	ASP_SocketError = -3,
	ASP_Timeout 	= -2,
	ASP_BadData 	= -1,
	ASP_WaitData 	= 0,
	ASP_Success 	= 1
}ASProtocolResult;

ASProtocolResult socket_read_proto_item( ASProtocolState *ps );
ASProtocolResult socket_read_proto( ASProtocolState *ps );
void socket_read_proto_reset( ASProtocolState *ps );
void *socket_read_steal_buffer( ASProtocolState *ps );

#endif                                         /* AS_SOCKET_H_HEADER_INCLUDED */

