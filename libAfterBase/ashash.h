#ifndef ASHASH_HEADER_FILE_INCLUDED
#define ASHASH_HEADER_FILE_INCLUDED
/* REALY USEFULL and UNIVERSAL hash table implementation */

typedef union {
    unsigned long  long_val ;
    char 	  *string_val;
}ASHashableValue ; 

typedef struct ASHashItem
{
    struct ASHashItem 	*next ;
    ASHashableValue    	 value ;
    void 		*data ; /* optional data structure related to this 
				   hashable value */
}ASHashItem ;

typedef unsigned int ASHashKey ;

typedef struct ASHashTable
{
    ASHashKey size ;
    ASHashItem *items ;
    ASHashKey (*hash_func)(ASHashableValue value, ASHashKey hash_size);
}ASHashTable;

void init_ashash( ASHashTable *hash, Bool freeresources );
ASHashTable *create_ashash( ASHashKey size, ASHashKey (*hash_func)(ASHashableValue value, ASHashKey hash_size));
void destroy_ashash( ASHashTable *hash );

Bool add_hash_item( ASHashTable *hash, ASHashableValue value, void *data );
Bool remove_hash_item( ASHashTable *hash, ASHashableValue value );
void *get_hash_item( ASHashTable *hash, ASHashableValue value );

/* here is the set of implemented hash functions : */
/* configuration options - case unsensitive and spaces are not alowed */
ASHashKey option_hash_value (ASHashableValue value, ASHashKey hash_size);
/* case unsensitive strings  - spaces and control chars are alowed */
ASHashKey casestring_hash_value (ASHashableValue value, ASHashKey hash_size);
/* case sensitive strings  - spaces and control chars are alowed */
ASHashKey string_hash_value (ASHashableValue value, ASHashKey hash_size);
/* basically any long value, but was written originally for colors hash */
ASHashKey color_hash_value(ASHashableValue value, ASHashKey hash_size);

#endif
