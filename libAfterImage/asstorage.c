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

#define LOCAL_DEBUG
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

#define StorageID2BlockIdx(id)    	(((((CARD32)(id))>>14)&0x0003FFFF)-1)
#define StorageID2SlotIdx(id)    	 ((((CARD32)(id))&0x00003FFF)-1)

static inline ASStorageID 
make_asstorage_id( int block_id, int slot_id )
{
	ASStorageID id = 0 ;
	if( block_id > 0 && block_id < (0x01<<18)&& slot_id > 0 && slot_id < (0x01<<14)) 
		id = ((CARD32)block_id<<14)|(CARD32)slot_id ;
	return id;
}

static CARD8* 
compress_stored_data( ASStorage *storage, CARD8 *data, int size, ASFlagType flags, int *compressed_size )
{
	/* TODO: just a stub for now - need to implement compression */
	if( compressed_size ) 
		*compressed_size = size ;
	return data;
}

static CARD8 *
decompress_stored_data( ASStorage *storage, CARD8 *data, int size, int uncompressed_size, 
						ASFlagType flags )
{
	/* TODO - just a stub for now */
	return data;
}

static ASStorageBlock *
create_asstorage_block( int useable_size )
{
	int allocate_size = (((sizeof(ASStorageBlock)+sizeof(ASStorageSlot) + useable_size)/4096)+1)*4096 ;
#ifndef DEBUG_ALLOCS
	void *ptr = malloc(allocate_size);
#else
	void *ptr = guarded_malloc(allocate_size);
#endif
	if( ptr == NULL ) 
		return NULL;
	ASStorageBlock *block = ptr ;
	memset( block, 0x00, sizeof(ASStorageBlock));
	block->size = allocate_size - sizeof(ASStorageBlock) ;
	block->total_free = block->size - ASStorageSlot_SIZE ;
	
#ifndef DEBUG_ALLOCS
	block->slots = calloc( AS_STORAGE_SLOTS_BATCH, sizeof(ASStorageSlot*));
#else
	block->slots = guarded_calloc( AS_STORAGE_SLOTS_BATCH, sizeof(ASStorageSlot*));
#endif	  
	block->slots_count = AS_STORAGE_SLOTS_BATCH ;
	if( block->slots == NULL ) 
	{	
		free( ptr ); 
		return NULL;
	}
	block->start = (ASStorageSlot*)(ptr+sizeof(ASStorageBlock));
	block->end = (ASStorageSlot*)(ptr+(allocate_size-ASStorageSlot_SIZE));
	block->slots[0] = block->start ;
	block->slots[0]->flags = 0 ;  /* slot of the free memory */ 
	block->slots[0]->ref_count = 0 ;
	block->slots[0]->size = block->size ;
	block->slots[0]->uncompressed_size = block->size ;
	block->last_used = 0;
	block->first_free = 0 ;
	
	LOCAL_DEBUG_OUT("Storage block created : block ptr = %p, slots ptr = %p", block, block->slots );
	
	return block;
}

static void
destroy_asstorage_block( ASStorageBlock *block )
{
	LOCAL_DEBUG_OUT( "freeing block %p, size = %d", block, block->size );
#ifndef DEBUG_ALLOCS
	free( block->slots );
	free( block );	  
#else	
	guarded_free( block->slots );
	guarded_free( block );
#endif

}

static int
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
				block->last_used < AS_STORAGE_MAX_SLOTS_CNT )
				return i+1;
		}else if( new_block < 0 ) 
			new_block = i ;
	}		
	/* no available blocks found - need to allocate a new block */
	if( new_block  < 0 ) 
	{
		i = new_block = storage->blocks_count ;
		storage->blocks_count += 16 ;
#ifndef DEBUG_ALLOCS
		storage->blocks = realloc( storage->blocks, storage->blocks_count*sizeof(ASStorageBlock*));
#else
		storage->blocks = guarded_realloc( storage->blocks, storage->blocks_count*sizeof(ASStorageBlock*));
#endif		   
		while( ++i < storage->blocks_count )
			storage->blocks[i] = NULL ;
	}	 
	storage->blocks[new_block] = create_asstorage_block( max(storage->default_block_size, compressed_size) );		
	if( storage->blocks[new_block] == NULL )  /* memory allocation failed ! */ 
		new_block = -1 ;
	return new_block+1;
}

static inline void
destroy_storage_slot( ASStorageBlock *block, int index )
{
	ASStorageSlot **slots = block->slots ;
	int i = index;
	
	slots[i] = NULL ; 
	if( block->last_used == index ) 
	{	
		while( --i > 0 ) 
			if( slots[i] != NULL ) 
				break;
		block->last_used = i<0?0:i;	 
	}
}

static inline void 
join_storage_slots( ASStorageBlock *block, ASStorageSlot *from_slot, ASStorageSlot *to_slot )
{
	ASStorageSlot *s ;
	do
	{
		s = AS_STORAGE_GetNextSlot(from_slot);
		from_slot->size += ASStorageSlot_FULL_SIZE(s) ;	
		destroy_storage_slot( block, s->index );
	}while( s < to_slot );	
}

static inline void
defragment_storage_block( ASStorageBlock *block )
{
	ASStorageSlot *first, *curr, *prev ;
	int first_free = -1 ;
	first = block->start ; 
	while( first < block->end ) 
	{	
		LOCAL_DEBUG_OUT("first = %p", first );
		if( first->flags == 0 ) 
		{	
			if( first_free < -1 )
				first_free = first->index;
			prev = NULL ;
			curr = first ;
			while( (curr = AS_STORAGE_GetNextSlot(curr)) < block->end )
			{
				LOCAL_DEBUG_OUT("curr = %p", curr );
				if( curr->flags != 0 ) 
					break;	 
				prev = curr ;
			}	 
			if( prev != NULL ) 
				join_storage_slots( block, first, prev );				   
	   	}
		first = AS_STORAGE_GetNextSlot(first);		   
	}
	block->first_free = first_free ;
	/* Second pass. Here we want to move all the taken slots down, 
	 * and all the free slots up. */

}

static ASStorageSlot *
select_storage_slot( ASStorageBlock *block, int size )
{
	int i = block->first_free, last_used = block->last_used ;
	ASStorageSlot **slots = block->slots ;
	
	while( i <= last_used )
	{
		ASStorageSlot *slot = slots[i] ;
		LOCAL_DEBUG_OUT( "last_used = %d, slots[%d] = %p", last_used, i, slot );
		if( slot != 0 )
		{
			int size_to_match = size ;
			while( slot->flags == 0 )
			{
				if( ASStorageSlot_USABLE_SIZE(slot) >= size )
					return slot;
				if( ASStorageSlot_USABLE_SIZE(slot) >= size_to_match )
				{
					join_storage_slots( block, slots[i], slot );
					return slots[i];
				}	
				size_to_match -= ASStorageSlot_FULL_SIZE(slot);
				slot = AS_STORAGE_GetNextSlot(slot);		
				/* make sure we has not exceeded boundaries of the block */									   
				if( slot >=  block->end )
					break;
			}			
		}
		++i ;
	}
		
	/* no free slots of sufficient size - need to do defragmentation */
	defragment_storage_block( block );
	i = block->first_free;
    if( block->slots[i] == NULL || block->slots[i]->size < size ) 
		return NULL;
	return block->slots[i];		   
}

static inline Bool
split_storage_slot( ASStorageBlock *block, ASStorageSlot *slot, int to_size )
{
	int old_size = ASStorageSlot_USABLE_SIZE(slot) ;
	ASStorageSlot *new_slot ;

	slot->size = to_size ; 
	new_slot = AS_STORAGE_GetNextSlot(slot);
	if( new_slot >=  block->end )
		return True;

	new_slot->flags = 0 ;
	new_slot->ref_count = 0 ;
	new_slot->size = old_size - ASStorageSlot_USABLE_SIZE(slot) - ASStorageSlot_SIZE ;											   
	new_slot->uncompressed_size = 0 ;
	
	new_slot->index = 0 ;
	/* now we need to find where this slot's pointer we should store */		   
	if( block->unused_count < block->slots_count/10 && block->last_used < block->slots_count-1 )
	{	
		new_slot->index = ++(block->last_used) ;
	}else
	{
		register int i, max_i = block->slots_count ;
		register ASStorageSlot **slots = block->slots ;
		for( i = 0 ; i < max_i ; ++i ) 
			if( slots[i] == NULL ) 
				break;
		if( i >= max_i ) 
		{
			if( block->slots_count + AS_STORAGE_SLOTS_BATCH > AS_STORAGE_MAX_SLOTS_CNT )
				return False;
			else
			{
				i = block->slots_count ;
				block->last_used = i ;
				block->slots_count += AS_STORAGE_SLOTS_BATCH ; 
#ifndef DEBUG_ALLOCS
				block->slots = realloc( block->slots, block->slots_count*sizeof(ASStorageSlot*));
#else
				block->slots = guarded_realloc( block->slots, block->slots_count*sizeof(ASStorageSlot*));
#endif
				memset( &(block->slots[i]),	0x00, AS_STORAGE_SLOTS_BATCH*sizeof(ASStorageSlot*) );
			}	 
		}
 		new_slot->index = i ;		
		if( i < block->last_used )
			--(block->unused_count);
	}	
	block->slots[new_slot->index] = new_slot ;
	return True;
}

static int
store_data_in_block( ASStorageBlock *block, CARD8 *data, int size, int compressed_size, ASFlagType flags )
{
	ASStorageSlot *slot ;
	CARD8 *dst ;
	slot = select_storage_slot( block, compressed_size );
	LOCAL_DEBUG_OUT( "selected slot %p for size %d (compressed %d) and flags %lX", slot, size, compressed_size, flags );
	if( slot == NULL ) 
		return 0;
	if( ASStorageSlot_USABLE_SIZE(slot) > compressed_size+ASStorageSlot_SIZE ) 
		if( !split_storage_slot( block, slot, compressed_size ) ) 
			return 0;

	block->total_free -= ASStorageSlot_FULL_SIZE(slot);
	
	dst = &(slot->data[0]);
	memcpy( dst, data, compressed_size );
	slot->flags = (flags | ASStorage_Used) ;
	slot->ref_count = 1;
	slot->size = compressed_size ;
	slot->uncompressed_size = size ;

	if( slot->index == block->first_free ) 
	{
		int i = block->first_free ;
		while( ++i < block->last_used ) 
			if( block->slots[i] && block->slots[i]->flags == 0 ) 
				break;
		block->first_free = i ;
	}
		  
	return slot->index+1 ;
}


static inline ASStorageBlock *
find_storage_block( ASStorage *storage, ASStorageID id )
{	
	int block_idx = StorageID2BlockIdx(id);
	if( block_idx >= 0 && block_idx < storage->blocks_count )  
		return storage->blocks[block_idx];
	return NULL ;
}

static ASStorageSlot *
find_storage_slot( ASStorageBlock *block, ASStorageID id )
{	
	if( block != NULL ) 
	{
		int slot_idx = StorageID2SlotIdx(id);
		if( slot_idx >= 0 && slot_idx < block->slots_count ) 
		{
			if( block->slots[slot_idx] && block->slots[slot_idx]->flags != 0 )
				return block->slots[slot_idx];
		}
	}	
	return NULL ;
}

static inline void 
free_storage_slot( ASStorageBlock *block, ASStorageSlot *slot)
{
	slot->flags = 0 ;
	block->total_free += ASStorageSlot_USABLE_SIZE(slot) ;
}	 

static Bool 
is_block_empty( ASStorageBlock *block)
{
	int i = block->last_used+1;
	ASStorageSlot **slots = block->slots ;
	while( --i >= 0 ) 
	{
		if( slots[i] )
			if( slots[i]->flags != 0 ) 
				return False;	 
	}	
	return True;	
}	 

static void 
free_storage_block( ASStorage *storage, int block_idx  )
{
	ASStorageBlock *block = storage->blocks[block_idx] ;
	storage->blocks[block_idx] = NULL ;
	destroy_asstorage_block( block );
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

	LOCAL_DEBUG_CALLER_OUT( "data = %p, size = %d, flags = %lX", data, size, flags );
	if( size <= 0 || data == NULL || storage == NULL ) 
		return 0;
		
	if( get_flags( flags, ASStorage_CompressionType ) )
		buffer = compress_stored_data( storage, data, size, flags, &compressed_size );
	block_id = select_storage_block( storage, compressed_size, flags );
	LOCAL_DEBUG_OUT( "selected block %d", block_id );
	if( block_id > 0 ) 
	{
		slot_id = store_data_in_block(  storage->blocks[block_id-1], 
										buffer, size, compressed_size, flags );

		if( slot_id > 0 )	
			id = make_asstorage_id( block_id, slot_id );
	}
	return id ;		
}



int  
fetch_data(ASStorage *storage, ASStorageID id, CARD8 *buffer, int offset, int buf_size)
{
	if( storage != NULL && id != 0 )
	{	
		ASStorageSlot *slot = find_storage_slot( find_storage_block( storage, id ), id );
		if( slot )
		{
			int uncomp_size = slot->uncompressed_size ;
			if( buffer && buf_size > 0 ) 
			{
				int bytes_in = 0;
				CARD8 *tmp = decompress_stored_data( storage, &(slot->data[0]), slot->size,
														uncomp_size, slot->flags );
				while( offset > uncomp_size ) offset -= uncomp_size ; 
				while( offset < 0 ) offset += uncomp_size ; 
				if( offset > 0 ) 
				{
					bytes_in = uncomp_size-offset ; 
					if( bytes_in  < buf_size ) 
						bytes_in = buf_size ;
					memcpy( buffer, tmp+offset, bytes_in ); 															
				}
				while( bytes_in  < buf_size ) 
				{
					int to_copy = buf_size - bytes_in ; 
					if( to_copy > uncomp_size ) 
						to_copy = uncomp_size ;
					memcpy( buffer+bytes_in, tmp, to_copy ); 															
					bytes_in += to_copy;
				}
			}
			return uncomp_size ;
		}
	}
	return 0 ;	
}

void 
forget_data(ASStorage *storage, ASStorageID id)
{
	if( storage != NULL && id != 0 ) 
	{
		ASStorageBlock *block = find_storage_block( storage, id );
 		ASStorageSlot  *slot  = find_storage_slot( block, id );				
		if( block && slot ) 
		{
			free_storage_slot(block, slot);
			if( is_block_empty(block) ) 
				free_storage_block( storage, StorageID2BlockIdx(id) );
		}	 
	}			  
}

/*************************************************************************/
/* test code */
/*************************************************************************/
#ifdef TEST_ASSTORAGE

#define STORAGE_TEST_KINDS	5
static int StorageTestKinds[STORAGE_TEST_KINDS][2] = 
{
	{1024, 2048 },
	{128*1024, 128 },
	{256*1024, 64 },
	{512*1024, 32 },
	{1024*1024, 16 }
};	 

CARD8 Buffer[1024*1024] ;

#define STORAGE_TEST_COUNT  16+32+64+128+2048	
typedef struct ASStorageTest {
	int size ;
	CARD8 *data;
	ASStorageID id ;
}ASStorageTest;
 
static ASStorageTest Tests[STORAGE_TEST_COUNT];

void
make_storage_test_data( ASStorageTest *test, int min_size, int max_size )
{
	int size = random()%max_size ;
	int i ;
	int seed = time(NULL);
	
	if( size <= min_size )
		size += min_size ;
 	
	test->size = size ; 	   
	test->data = malloc(size);
	
	for( i = 0 ; i < size ; ++i ) 
		test->data[i] = i+seed+test->data[i] ;
	test->id = 0 ;
}

Bool 
test_asstorage()
{
	ASStorage *storage ;
	ASStorageID id ;
	int i, kind, test_count;
	int min_size, max_size ;
	ASFlagType test_flags = 0 ;
	
	fprintf(stderr, "Testing storage creation ...");
	storage = create_asstorage();
#define TEST_EVAL(val)   do{ \
							if(!(val)){ fprintf(stderr, "failed\n"); return False;} \
							else fprintf(stderr, "success.\n");}while(0)
	TEST_EVAL( storage != NULL ); 
	
	fprintf(stderr, "Testing store_data for data %p size = %d, and flags 0x%lX...", NULL, 0,
			test_flags);
	id = store_data( storage, NULL, 0, test_flags );
	TEST_EVAL( id == 0 ); 

	kind = 0 ; 
	min_size = 1 ;
	max_size = StorageTestKinds[kind][0] ; 
	test_count = StorageTestKinds[kind][1] ;
	for( i = 0 ; i < STORAGE_TEST_COUNT ; ++i ) 
	{
		make_storage_test_data( &(Tests[i]), min_size, max_size );
		fprintf(stderr, "Testing store_data for data %p size = %d, and flags 0x%lX...", Tests[i].data, Tests[i].size,
				test_flags);
		Tests[i].id = store_data( storage, Tests[i].data, Tests[i].size, test_flags );
		TEST_EVAL( Tests[i].id != 0 ); 

		if( --test_count <= 0 )
		{
			if( ++kind >= STORAGE_TEST_KINDS ) 
				break;
			min_size = max_size ;
			max_size = StorageTestKinds[kind][0] ; 
			test_count = StorageTestKinds[kind][1] ;
		}		   
	}	 

	fgetc(stdin);
	for( i = 0 ; i < STORAGE_TEST_COUNT ; ++i ) 
	{
		int size ;
		int res ;
		fprintf(stderr, "Testing fetch_data for id %lX size = %d ...", Tests[i].id, Tests[i].size);
		size = fetch_data(storage, Tests[i].id, &(Buffer[0]), 0, Tests[i].size);
		TEST_EVAL( size == Tests[i].size ); 
		
		fprintf(stderr, "Testing fetched data integrity ...");
		res = memcmp( &(Buffer[0]), Tests[i].data, size );
		TEST_EVAL( res == 0 ); 
	}	 

	fgetc(stdin);
	for( i = 0 ; i < STORAGE_TEST_COUNT ; ++i ) 
	{
		int size ;
		int r = random();
		if( (r&0x01) == 0 ) 
			continue;
		fprintf(stderr, "Testing forget_data for id %lX size = %d ...\n", Tests[i].id, Tests[i].size);
		forget_data(storage, Tests[i].id);
		size = fetch_data(storage, Tests[i].id, &(Buffer[0]), 0, Tests[i].size);
		TEST_EVAL( size != Tests[i].size ); 
		Tests[i].id = 0;
		free( Tests[i].data );
		Tests[i].data = NULL ; 
		Tests[i].size = 0 ;
	}	 
	fgetc(stdin);
	kind = 0 ; 
	min_size = 1 ;
	max_size = StorageTestKinds[kind][0] ; 
	test_count = StorageTestKinds[kind][1] ;
	for( i = 0 ; i < STORAGE_TEST_COUNT ; ++i ) 
	{
		if( Tests[i].id == 0 ) 
		{	
			make_storage_test_data( &(Tests[i]), min_size, max_size );
			fprintf(stderr, "Testing store_data for data %p size = %d, and flags 0x%lX...\n", Tests[i].data, Tests[i].size,
					test_flags);
			Tests[i].id = store_data( storage, Tests[i].data, Tests[i].size, test_flags );
			TEST_EVAL( Tests[i].id != 0 ); 
		}
		if( --test_count <= 0 )
		{
			if( ++kind >= STORAGE_TEST_KINDS ) 
				break;
			min_size = max_size ;
			max_size = StorageTestKinds[kind][0] ; 
			test_count = StorageTestKinds[kind][1] ;
		}		   
	}	 
	
	fgetc(stdin);
	for( i = 0 ; i < STORAGE_TEST_COUNT ; ++i ) 
	{
		int size ;
		int res ;
		fprintf(stderr, "Testing fetch_data for id %lX size = %d ...", Tests[i].id, Tests[i].size);
		size = fetch_data(storage, Tests[i].id, &(Buffer[0]), 0, Tests[i].size);
		TEST_EVAL( size == Tests[i].size ); 
		
		fprintf(stderr, "Testing fetched data integrity ...");
		res = memcmp( &(Buffer[0]), Tests[i].data, size );
		TEST_EVAL( res == 0 ); 
	}	 

	fprintf(stderr, "Testing storage destruction ...");
	destroy_asstorage(&storage);
	TEST_EVAL( storage == NULL ); 
	
	return True ;
}

int main()
{
	set_output_threshold( 0 );
	return test_asstorage();
}
#endif

