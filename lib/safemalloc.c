#include <stdlib.h>
#include <stdio.h>
#include "../configure.h"
#include "../include/aftersteplib.h"

#ifdef DEBUG_ALLOCS
#include <string.h>
#undef malloc
#undef safemalloc
#endif /* DEBUG_ALLOCS */

/* always undef free, as it will be both redefined with and without 
   DEBUG_ALLOC */
#undef free

/* safemalloctmp - short term memory  heap implementation */

#define BLOCK_SIZE	(1<<20)
#define MIN_BLOCK_SIZE	(1<<8)
#define BLOCKS_NUM	16

#define FREE_SIGNATURE  0xAAAAAAAA
#define USED_SIGNATURE  0x55555555

typedef struct _allocated_block
  {
    struct _allocated_block *prev;
    struct _allocated_block *next;
    unsigned long sig;
    size_t size;
  }
allocated_block;

typedef struct
  {
    allocated_block *first, *last;
    size_t max_free;
  }
short_term_block;

short_term_block short_term_storage[BLOCKS_NUM];
int blocks_used = 0;

void *malloc_tmp (size_t size);
void *malloc_tmp_first (size_t size);

void *(*malloc_func) (size_t size) = malloc_tmp_first;

void
init_tmp_heap ()
{
  register int i;
  for (i = 0; i < BLOCKS_NUM; i++)
    {
      short_term_storage[i].first = NULL;
      short_term_storage[i].last = NULL;
    }
}

short_term_block *
new_block ()
{
  register int i;
  short_term_block *block = short_term_storage;
  for (i = 0; i < BLOCKS_NUM; i++)
    if (short_term_storage[i].first == NULL)
      break;

  if (i >= BLOCKS_NUM)
    return NULL;
  block = &short_term_storage[i];
  block->last = block->first = (allocated_block *) malloc (BLOCK_SIZE);
#ifdef DEBUG_ALLOCS
  memset (block->last, 0xFF, BLOCK_SIZE);
#endif
  block->last->sig = FREE_SIGNATURE;
  block->max_free = block->last->size = BLOCK_SIZE - sizeof (allocated_block);
  block->first->next = block->last->prev = NULL;
  if (i >= blocks_used)
    blocks_used = i + 1;
#ifdef DEBUG_ALLOCS
  fprintf (stderr, "\nallocated temporary block # %d", i);
#endif
  return block;
}

void *
malloc_tmp (size_t size)
{
  short_term_block *block = short_term_storage;
  allocated_block *ptr;
  allocated_block *new_ptr;
  register int i;
  for (i = 0; i < blocks_used; i++)
    if (short_term_storage[i].first != NULL && short_term_storage[i].max_free >= size)
      break;
  if (i < blocks_used)
    block = &(short_term_storage[i]);
  else if ((block = new_block ()) == NULL)
    return NULL;

  for (ptr = block->last; ptr != NULL; ptr = ptr->prev)
    if (ptr->sig == FREE_SIGNATURE)
      if (ptr->size >= size)
	{			/* use this chunk */
	  if (ptr->size > size + sizeof (allocated_block) * 2)
	    {
	      new_ptr = ptr + (size / sizeof (allocated_block) + 1) + 1;
	      new_ptr->prev = ptr;
	      new_ptr->next = ptr->next;
	      if (ptr->next)
		ptr->next->prev = new_ptr;
	      else
		block->last = new_ptr;
	      ptr->next = new_ptr;
	      new_ptr->size = ptr->size - (((void *) new_ptr) - ((void *) ptr) + sizeof (allocated_block) * 2);
	      if (ptr->size >= block->max_free)
		block->max_free = new_ptr->size;
	      ptr->size -= new_ptr->size + sizeof (allocated_block);
	      new_ptr->sig = FREE_SIGNATURE;
	    }
	  else if (ptr->size >= block->max_free)
	    block->max_free = 0;

	  ptr->sig = USED_SIGNATURE;
	  break;
	}

  return (void *) (ptr + 1);
}

void *
malloc_tmp_first (size_t size)
{
  init_tmp_heap ();

  malloc_func = malloc_tmp;
  return malloc_tmp (size);
}

void
free_tmp (void *ptr)
{
  register int i;
  short_term_block *block = short_term_storage;
  allocated_block *owner = (allocated_block *) (ptr - sizeof (allocated_block));
  if (ptr == NULL)
    return;

  for (i = 0; i < blocks_used; i++, block++)
    if (block->first != NULL)
      if (ptr > (void *) (block->first) && ptr < (void *) (block->first) + BLOCK_SIZE)
	break;

  if (i >= blocks_used)
    free (ptr);
  else
    {
      if (owner->sig != USED_SIGNATURE)
	return;
      owner->sig = FREE_SIGNATURE;

      if (owner->prev)
	if (owner->prev->sig == FREE_SIGNATURE)
	  {
	    owner->prev->size += owner->size + sizeof (allocated_block);
	    owner->prev->next = owner->next;
	    if (owner->next)
	      owner->next->prev = owner->prev;
	    owner = owner->prev;
	  }

      if (owner->next == NULL)
	block->last = owner;
      else if (owner->next->sig == FREE_SIGNATURE)
	{
	  owner = owner->next;
	  owner->prev->size += owner->size + sizeof (allocated_block);
	  owner->prev->next = owner->next;
	  if (owner->next)
	    owner->next->prev = owner->prev;
	  owner = owner->prev;
	}
      if (owner->next == NULL && owner->prev == NULL)
	{			/* destroying block completely */
	  free (block->first);
	  block->first = block->last = NULL;
#ifdef DEBUG_ALLOCS
	  fprintf (stderr, "\ndeallocated temporary block # %d", i);
#endif
	  if (i == blocks_used - 1)
	    blocks_used--;
	}
      else if (owner->size > block->max_free)
	block->max_free = owner->size;
    }
}

/* end safemalloctmp */

/***********************************************************************
 *
 *  Procedure:
 *	safemalloc - mallocs specified space or exits if there's a 
 *		     problem
 *
 ***********************************************************************/
static int use_tmp_heap = 0;

int
set_use_tmp_heap (int on)
{
  int toret = use_tmp_heap;
  use_tmp_heap = on;
  return toret;
}

void *
safemalloc (size_t length)
{
  char *ptr;

  if (length <= 0)
    length = 1;

  if (use_tmp_heap && length < MIN_BLOCK_SIZE)
    ptr = malloc_func (length);
  else
    ptr = malloc (length);

  if (ptr == (char *) 0)
    {
      fprintf (stderr, "malloc of %d bytes failed. Exiting\n", length);
      exit (1);
    }
  return ptr;
}

void
safefree (void *ptr)
{
  if (use_tmp_heap)
    free_tmp (ptr);
  else if (ptr)
    free (ptr);
}
