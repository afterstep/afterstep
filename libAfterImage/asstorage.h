#ifndef ASSTORAGE_H_HEADER_INCLUDED
#define ASSTORAGE_H_HEADER_INCLUDED


/* 
 *	Slot can hold upto (2^16)*4 == 256 KBytes ; 
 *	there could be up to 4 arrays of 512 pointers to slots each in Storage Block
 *	There could be 2^20 StorageBlocks in ASStorage
 */


#define ASStorageSlot_SIZE 8 /* 8 bytes */

typedef struct ASStorageSlot
{
/* Pointer to ASStorageSlot is the pointer to used memory beginning - ASStorageSlot_SIZE 
 * thus we need not to store it separately 
 */

#define ASStorage_CompressionType	(0x0F<<0) /* allow for 16 compression schemes */
#define ASStorage_Used				(0x01<<4)
#define ASStorage_NotTileable		(0x01<<4)

	CARD16  flags ;
	CARD16  data_size ;
	CARD16  ref_count ;
	CARD16  reserved ;
}ASStorageSlot;


/* Pointer to ASStorageBlock is the pointer to allocated memory beginning - sizeof(ASStorageBlock) 
 * thus we need not to store it separately 
 */

typedef struct ASStorageBlock
{
#define ASStorage_MonoliticBlock		(0x01<<0) /* block consists of a single batch of storage */
 	CARD32  flags ;
	int 	size ;

	int   	min_free, max_free, used;

	/* array of pointers to slots is allocated separately, so that we can reallocate it 
	   in case we have lots of small slots */
	ASStorageSlot **slots;
	int 	slots_count, first_free ;

}ASStorageBlock;


typedef struct ASStorage
{

	ASStorageBlock **blocks ;
	int 			blocks_count, first_free;

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
