#ifndef ASSTORAGE_H_HEADER_INCLUDED
#define ASSTORAGE_H_HEADER_INCLUDED


/* 
 *	there could be up to 16 arrays of 1024 pointers to slots each in Storage Block
 *	There could be 2^18 StorageBlocks in ASStorage
 */
#define AS_STORAGE_SLOTS_BATCH		1024  /* so that batch of pointer occupies 1 page  */ 
#define AS_STORAGE_SLOTS_BATCH_CNT	16
#define AS_STORAGE_Index2Batch(i)   (((i)>>10)&0x0F)
#define AS_STORAGE_Index2BatchIdx(i)   ((i)&0x03FF)
#define AS_STORAGE_SLOT_ID_BITS		14  /* 32*512 == 2^14 */ 
#define AS_STORAGE_MAX_SLOTS_CNT	(0x01<<AS_STORAGE_SLOT_ID_BITS)

#define AS_STORAGE_BLOCK_ID_BITS	(32-AS_STORAGE_SLOT_ID_BITS)
#define AS_STORAGE_MAX_BLOCK_CNT   	(0x01<<AS_STORAGE_BLOCK_ID_BITS)
#define AS_STORAGE_DEF_BLOCK_SIZE	(1024*256)  /* 256 Kb */  


#define ASStorageSlot_SIZE 16 /* 16 bytes */

typedef struct ASStorageSlot
{
/* Pointer to ASStorageSlot is the pointer to used memory beginning - ASStorageSlot_SIZE 
 * thus we need not to store it separately 
 */

#define ASStorage_CompressionType	(0x0F<<0) /* allow for 16 compression schemes */
#define ASStorage_Used				(0x01<<4)
#define ASStorage_NotTileable		(0x01<<5)

	CARD16  flags ;
	CARD16  ref_count ;
	CARD32  size ;
	CARD32  uncompressed_size ;
	CARD16  index ;  /* reverse mapping of slot address into index in array */
	/* slots may be placed in array pointing into different areas of the memory 
	 * block, since we will need to implement some sort of garbadge collection and 
	 * defragmentation mechanism - we need to be able to process them in orderly 
	 * fashion. 
	 * So finally : 
	 * 1) slot's index does not specify where in the memory slot 
	 * is located, it is only used to address slot from outside.
	 * 2) Using slots memory address and its size we can go through the chain of slots
	 * and perform all the maintenance tasks  as long as we have reverse mapping 
	 * of addresses into indexes.
	 * 
	 */
	CARD16 reserved ;          /* to make us have size rounded by 16 bytes margin */
}ASStorageSlot;


typedef struct ASStorageBlock
{
#define ASStorage_MonoliticBlock		(0x01<<0) /* block consists of a single batch of storage */
 	CARD32  flags ;
	int 	size ;

	int   	total_free;
	ASStorageSlot  *start;
	/* array of pointers to slots is allocated separately, so that we can reallocate it 
	   in case we have lots of small slots */
	ASStorageSlot **slots[AS_STORAGE_SLOTS_BATCH_CNT];
	int first_free, last_used ;

}ASStorageBlock;


typedef struct ASStorage
{
	int default_block_size ;


	ASStorageBlock **blocks ;
	int 			blocks_count;

}ASStorage;


typedef CARD32 ASStorageID ;


ASStorageID store_data(ASStorage *storage, CARD8 *data, int size, ASFlagType flags);
/* data will be fetched fromthe slot identified by id and placed into buffer. 
 * Data will be fetched from offset  and will count buf_size bytes if buf_size is greater then
 * available data - data will be tiled to accomodate this size, unless NotTileable is set */
int  fetch_data(ASStorage *storage, ASStorageID id, CARD8 *buffer, int offset, int buf_size);
/* slot identified by id will be marked as unused */
void forget_data(ASStorage *storage, ASStorageID id);

/* data will be copied starting with src_offset, and counting dst_size bytes.
 * it will be stored in slot identifyed by optional dst_id, or in the new slot,
 * if dst_id is 0. 
 */
ASStorageID copy_data(  ASStorage *storage, ASStorageID src_id, 
						ASStorageID dst_id, int src_offset, int dst_size, ASFlagType dst_flags);

/* returns new ID without copying data. Data will be stored as copy-on-right. 
 * Reference count of the data will be increased. If optional dst_id is specified - 
 * its data will be erased, and it will point to the data of src_id: 
 */				
ASStorageID dup_data(ASStorage *storage, ASStorageID src_id, ASStorageID dst_id);


#endif
