#ifndef ASLIST_HEADER_FILE_INCLUDED
#define ASLIST_HEADER_FILE_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ASBiDirElem
{
	struct ASBiDirElem *next, *prev ;
	void *data ;
}ASBiDirElem;

typedef void (*destroy_list_data_handler)( void *data );
typedef Bool (*iter_list_data_handler)(void *data, void *aux_data); /* returns:
                                                                     *   True  - continue iteration
                                                                     *   False - abort iteration
                                                                     */

typedef int (*compare_data_handler)(void *data1, void *data2);

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

void iterate_asbidirlist( ASBiDirList *l, iter_list_data_handler iter_func, void *aux_data,
                          void *start_from, Bool reverse);

void bubblesort_asbidirlist( ASBiDirList *l, compare_data_handler compare_func );

void *append_bidirelem( ASBiDirList *l, void *data );
void *prepend_bidirelem( ASBiDirList *l, void *data );
void *insert_bidirelem_after( ASBiDirList *l, void *data, ASBiDirElem *after );
void *insert_bidirelem_before( ASBiDirList *l, void *data, ASBiDirElem *before );
/* moves element to the beginning of the list :*/
void  pop_bidirelem( ASBiDirList *l, ASBiDirElem *elem );
/* destroy element and its data : */
void  destroy_bidirelem( ASBiDirList *l, ASBiDirElem *elem );
Bool  discard_bidirelem( ASBiDirList *l, void *data );
/* returns data of the first element in list, and removes this element from the list */
void *extract_first_bidirelem( ASBiDirList *l );

void  flush_asbidirlist_memory_pool();

#ifdef __cplusplus
}
#endif

#endif /* #ifndef ASVECTOR_HEADER_FILE_INCLUDED */
