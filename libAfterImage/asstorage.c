/* This file contains code for memopry management for image data    */
/********************************************************************/
/* Copyright (c) 2004 Sasha Vasko <sasha at aftercode.net>          */
/********************************************************************/
/*
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
 *
 */

#undef LOCAL_DEBUG
#undef DO_CLOCKING

#ifdef _WIN32
#include "win32/config.h"
#else
#include "config.h"
#endif

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
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#include <memory.h>

#ifdef _WIN32
# include "win32/afterbase.h"
#else
# include "afterbase.h"
#endif

#include "asstorage.h"

/************************************************************************/
/* Private Functions : 													*/
/************************************************************************/
static inline ASStorageID 
make_asstorage_id( int block_id, int slot_id )
{
	ASStorageID id = 0 ;
	if( block_id > 0 && block_id < (0x01<<18)&& slot_id > 0 && slot_id < (0x01<<14)) 
		id = ((CARD32)block_id<<14)|(CARD32)slot_id ;
	return id;
}

CARD8* 
compress_stored_data( ASStorage *storage, CARD8 *data, int size, ASFlagType flags, int *compressed_size )
{
	/* TODO: just a stub for now - need to implement compression */
	if( compressed_size ) 
		*compressed_size = size ;
	return data;
}

ASStorageBlock *
create_asstorage_block( int useable_size )
{
	int allocate_size = (((sizeof(ASStorageBlock)+sizeof(ASStorageSlot) + useable_size)/4096)+1)*4096 ;
	void * ptr = malloc(allocate_size);
	if( ptr == NULL ) 
		return NULL;
	ASStorageBlock *block = ptr ;
	memset( block, 0x00, sizeof(ASStorageBlock));
	block->size = allocate_size - sizeof(ASStorageBlock) ;
	block->total_free = block->size ;
	block->slots[0] = calloc( AS_STORAGE_SLOTS_BATCH, sizeof(ASStorageSlot*));
	if( block->slots[0] == NULL ) 
	{	
		free( ptr ); 
		return NULL;
	}
	block->start = (ASStorageSlot*)(ptr+sizeof(ASStorageBlock));
	block->slots[0][0] = block->start ;
	block->slots[0][0]->flags = 0 ;  /* slot of the free memory */ 
	block->slots[0][0]->ref_count = 0 ;
	block->slots[0][0]->size = block->size ;
	block->slots[0][0]->uncompressed_size = block->size ;
	block->used_slots = 1 ;

	return block;
}

int
select_storage_block( ASStorage *storage, int compressed_size, ASFlagType flags )
{
	int i ;
	int new_block = -1 ; 
	compressed_size += sizeof(ASStorageSlot);
	for( i = 0 ; i < storage->blocks_count ; ++i ) 
	{
		ASStorageBlock *block = storage->blocks[i];
		if( block )
		{	
			if( block->total_free > compressed_size && 
				block->used_slots < AS_STORAGE_MAX_SLOTS_CNT )
				return i+1;
		}else if( new_block < 0 ) 
			new_block = i ;
	}		
	/* no available blocks found - need to allocate a new block */
	if( new_block  < 0 ) 
	{
		i = new_block = storage->blocks_count ;
		storage->blocks_count += 16 ;
		storage->blocks = realloc( storage->blocks, storage->blocks_count*sizeof(ASStorageBlock*));
		while( ++i < storage->blocks_count )
			storage->blocks[i] = NULL ;
	}	 
	storage->blocks[new_block] = create_asstorage_block( max(storage->default_block_size, compressed_size) );		
	if( storage->blocks[new_block] == NULL )  /* memory allocation failed ! */ 
		new_block = -1 ;
	return new_block+1;
}

#define AS_STORAGE_GetNextSlot(slot) ((ASStorageSlot*)(((void*)((slot)+1))+(slot)->size))

void 
join_storage_slots( ASStorageBlock *block, ASStorageSlot *from_slot, ASStorageSlot *to_slot )
{
	ASStorageSlot *s = AS_STORAGE_GetNextSlot(from_slot);
	
	do
	{
		from_slot->size += s->size ;	
	
	}while( s != to_slot );	
}


ASStorageSlot *
select_storage_slot( ASStorageBlock *block, int size )
{
	int i = block->first_free;
	int batch_no = AS_STORAGE_Index2Batch(i); 
	ASStorageSlot **batch = block->slots[batch_no] ;
	i = AS_STORAGE_Index2BatchIdx(i);
	while( batch != NULL )
	{
		while( i < AS_STORAGE_SLOTS_BATCH )
		{
			ASStorageSlot *slot = batch[i] ;
			if( slot != 0 )
			{
				int size_to_match = size ;
				while( slot->flags == 0 )
				{
					if( slot->size >= size )
						return slot;
					if( slot->size >= size_to_match )
					{
						join_storage_slots( block, batch[i]->index, slot->index );
						return batch[i];
					}	
					size_to_match -= slot->size ;
					slot = AS_STORAGE_GetNextSlot(slot);											
				}			
			}
			++i ;
		}
		
		if( ++batch_no < AS_STORAGE_SLOTS_BATCH_CNT ) 
			break ;
		batch = block->slots[batch_no];
		i = 0;
	}
	return NULL;		   
}

Bool
split_storage_slot( ASStorageBlock *block, ASStorageSlot *slot, int to_size )
{
	
	return True;
}

int
store_data_in_block( ASStorageBlock *block, CARD8 *data, int size, int compressed_size, ASFlagType flags )
{
	ASStorageSlot *slot ;
	CARD8 *dst ;
	slot = select_storage_slot( block, compressed_size );
	if( slot == NULL ) 
		return 0;
	if( slot->size > compressed_size ) 
		if( !split_storage_slot( block, slot, compressed_size ) ) 
			return 0;
	dst = (CARD8*)(slot+1);
	memcpy( dst, data, compressed_size );
	slot->flags = (flags | ASStorage_Used) ;
	slot->ref_count = 1;
	slot->size = compressed_size ;
	slot->uncompressed_size = size ;

	return slot->index+1 ;
}


/************************************************************************/
/* Public Functions : 													*/
/************************************************************************/
ASStorage *
create_asstorage()
{
	ASStorage *storage = safecalloc(1, sizeof(ASStorage));
	storage->default_block_size = AS_STORAGE_DEF_BLOCK_SIZE ;
	return storage ;
}

void 
destroy_asstorage(ASStorage **pstorage)
{
	ASStorage *storage = *pstorage ;
	
	if( storage->blocks != NULL || storage->blocks_count  > 0 )
	{
		/* TODO */
		return;
	}	
	
	*pstorage = NULL ;
}

ASStorageID 
store_data(ASStorage *storage, CARD8 *data, int size, ASFlagType flags)
{
	int id = 0 ;
	int block_id ;
	int slot_id ;
	int compressed_size = size ;
	CARD8 *buffer = data;
	if( size <= 0 || data == NULL || storage == NULL ) 
		return 0;
		
	if( get_flags( flags, ASStorage_CompressionType ) )
		buffer = compress_stored_data( storage, data, size, flags, &compressed_size );
	block_id = select_storage_block( storage, compressed_size, flags );
	if( block_id > 0 ) 
	{
		slot_id = store_data_in_block(  storage->blocks[block_id-1], 
										buffer, size, compressed_size, flags );
	
		id = make_asstorage_id( block_id, slot_id );
	}
	return id ;		
}


CARD8* make_storage_test_data( int size )
{
	CARD8 *data = malloc(size);
	int i ;
	int seed = time(NULL);
	for( i = 0 ; i < size ; ++i ) 
		data[i] = i+seed%(i+1)+data[i] ;
	return data ;
}

Bool 
test_asstorage()
{
	ASStorage *storage ;
	ASStorageID id ;
	CARD8 *test_data = NULL;
	int test_data_size = 0;
	ASFlagType test_flags = 0 ;
	
	printf("Testing storage creation ...");
	storage = create_asstorage();
#define TEST_EVAL(val)   do{ \
							if(!(val)){ printf("failed\n"); return False;} \
							else printf("success.\n");}while(0)
	TEST_EVAL( storage != NULL ); 
	
	printf("Testing store_data for data %p size = %d, and flags 0x%lX...", test_data, test_data_size,
			test_flags);
	id = store_data( storage, test_data, test_data_size, test_flags );
	TEST_EVAL( id == 0 ); 

	test_data_size = 1 ;
	test_data = make_storage_test_data( test_data_size );
	printf("Testing store_data for data %p size = %d, and flags 0x%lX...", test_data, test_data_size,
			test_flags);
	id = store_data( storage, test_data, test_data_size, test_flags );
	TEST_EVAL( id != 0 ); 
	free( test_data );

	printf("Testing storage destruction ...");
	destroy_asstorage(&storage);
	TEST_EVAL( storage == NULL ); 
	
	return True ;
}

#ifdef TEST_ASSTORAGE
int main()
{
	return test_asstorage();
}
#endif
