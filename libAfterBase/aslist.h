#ifndef ASLIST_HEADER_FILE_INCLUDED
#define ASLIST_HEADER_FILE_INCLUDED

typedef struct ASBiDirElem
{
	struct ASBiDirElem *next, *prev ;
	void *data ;
}ASBiDirElem;

typedef void (*destroy_list_data_handler)( void *data );

typedef struct ASBiDirList
{
	size_t count ;
	destroy_list_data_handler destroy_func ;
	ASBiDirElem *head, *tail ;
}ASBiDirList;

#define LIST_START(l)  		((l)->head)
#define LIST_END(l)    		((l)->tail)
#define LISTELEM_DATA(e)    ((e)->data)
#define LISTELEM_NEXT(e)    ((e)->next)
#define LISTELEM_PREV(e)    ((e)->prev)
#define LIST_GOTO_NEXT(e)    ((e)=(e)->next)
#define LIST_GOTO_PREV(e)    ((e)=(e)->prev)


ASBiDirList *create_asbidirlist(destroy_list_data_handler destroy_func);
void purge_asbidirlist( register ASBiDirList *l );
void destroy_asbidirlist( ASBiDirList **pl );
void *append_bidirelem( ASBiDirList *l, void *data );
void *prepend_bidirelem( ASBiDirList *l, void *data );
void *insert_bidirelem_after( ASBiDirList *l, void *data, ASBiDirElem *after );
void *insert_bidirelem_before( ASBiDirList *l, void *data, ASBiDirElem *before );
void  pop_bidirelem( ASBiDirList *l, ASBiDirElem *elem );
void  destroy_bidirelem( ASBiDirList *l, ASBiDirElem *elem );
void  discard_bidirelem( ASBiDirList *l, void *data );


#endif /* #ifndef ASVECTOR_HEADER_FILE_INCLUDED */
