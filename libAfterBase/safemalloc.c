/*
 * Copyright (c) 2001 Sasha Vasko <sasha at aftercode.net>
 *   and many others, who has not left their copyrights here :)
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

#define LOCAL_DEBUG
#include "config.h"

#include <stdlib.h>
#include <stdio.h>

#ifdef __CYGWIN__
#include <w32api/windows.h>
#define XMD_H
#define XPROTO_H
#endif


#include "astypes.h"
#include "output.h"
#include "selfdiag.h"
#include "safemalloc.h"


#ifdef DEBUG_ALLOCS
#include <string.h>
#undef malloc
#undef safemalloc
#undef safecalloc
#endif /* DEBUG_ALLOCS */

/* always undef free, as it will be both redefined with and without
   DEBUG_ALLOC */
#undef free

#if defined(__CYGWIN__)

#define AS_WIN32_PAGE_MAGIC		0xA36a5ea1
/* lets do some magic with Win32 memory allocation to test for memory corruption : */
static void *
alloc_guarded_memory( size_t length	)
{
#define WIN32_PAGE_SIZE		4096   		       /* assuming 4K pages in windows on x86 machines */
	LPVOID lpvAddr; 
    DWORD cbSize; 
    LPVOID commit, commit_guard; 
 
	/* Reserve whole page per allocation, plus another page - for guard */
	cbSize = length+sizeof(size_t)*2; 
	if( cbSize%WIN32_PAGE_SIZE == 0 )
		cbSize = (cbSize/WIN32_PAGE_SIZE)*WIN32_PAGE_SIZE; 
	else
		cbSize = ((cbSize/WIN32_PAGE_SIZE)+1)*WIN32_PAGE_SIZE; 
 
    /* Try to allocate some memory. */
    lpvAddr = VirtualAlloc(NULL,cbSize+WIN32_PAGE_SIZE,MEM_RESERVE,PAGE_NOACCESS); 
 
    if(lpvAddr == NULL) 
        fprintf(stderr,"MEMORY ERROR: VirtualAlloc failed on RESERVE with %ld\n", GetLastError()); 
	else
	{	/* Try to commit the allocated memory.  */
    	commit = VirtualAlloc(lpvAddr,cbSize,MEM_COMMIT,PAGE_READWRITE); 
		commit_guard = VirtualAlloc(lpvAddr+cbSize,WIN32_PAGE_SIZE,MEM_COMMIT,PAGE_READONLY|PAGE_GUARD); 
 
    	if(commit == NULL ) 
        	fprintf(stderr,"MEMORY ERROR: VirtualAlloc failed on COMMIT with %ld\n", GetLastError()); 
    	if(commit_guard == NULL ) 
        	fprintf(stderr,"MEMORY ERROR: VirtualAlloc failed on COMMIT GUARD with %ld\n", GetLastError()); 
		if( commit && commit_guard ) 
		{ 
			size_t *size_ptr = (size_t*)commit;
			void *ptr = commit + (cbSize - length);	 
			size_ptr[0] = length ;
			size_ptr[1] = AS_WIN32_PAGE_MAGIC ;
#ifdef LOCAL_DEBUG
			fprintf( stderr, "ALLOC: ptr %p points to a block of %d bytes. commit = %p\n", ptr, length, commit );
#endif
			return ptr ;
		}	
	}
	return NULL;
}

static size_t
get_guarded_memory_size( char *ptr )
{
	unsigned long long_ptr = (unsigned long)ptr ;
	size_t *size_ptr ;
	if( (long_ptr&0x00000FFF) == 0 )
		long_ptr -= WIN32_PAGE_SIZE ;
	size_ptr = (size_t*) (long_ptr&0xFFFFF000) ;
#ifdef LOCAL_DEBUG
	fprintf( stderr, "ALLOC: ptr %p seems to point to a block of %d bytes\n", ptr, *size_ptr );
#endif
	if( size_ptr[1] != AS_WIN32_PAGE_MAGIC  )
	{
		fprintf( stderr, "ALLOC: warning - ptr %p was not allocated properly\n", ptr );		   
	}	
	return size_ptr[0] ;
}	   

static void
free_guarded_memory( void *ptr )
{
	unsigned long long_ptr = (unsigned long)ptr ;
	size_t *size_ptr ;
	if( (long_ptr&0x00000FFF) == 0 )
		long_ptr -= WIN32_PAGE_SIZE ;
	size_ptr = (size_t*) (long_ptr&0xFFFFF000) ;
	if( size_ptr[1] != AS_WIN32_PAGE_MAGIC  )
	{	
		char *suicide = NULL;
		fprintf( stderr, "FREE: warning - freeing ptr %p that was not allocated properly (size_ptr = %p)\n", ptr, size_ptr );		   
		fflush( stderr );
		*suicide = 1 ;		
	}else
	{	
#ifdef LOCAL_DEBUG
		fprintf( stderr, "FREE: ptr %p points to a block of %d bytes. commit = %p\n", ptr, size_ptr[0], size_ptr );
#endif
		VirtualFree( size_ptr, 0, MEM_RELEASE );
	}
}	   

#endif

void
failed_alloc( const char * function, size_t size )
{
	char *suicide = NULL;
	fprintf (stderr, "%s of %lu bytes failed. Exiting\n", function, (unsigned long)size);
	*suicide = 1 ;
	exit (1);
}
	  


void         *
safemalloc (size_t length)
{
#if defined(__CYGWIN__) && defined(DEBUG_ALLOCS)
	return guarded_malloc (length);
#else
	char         *ptr = NULL ;

	if (length <= 0)
		length = 1;

	ptr = malloc (length);

	if (ptr == (char *)0)
		failed_alloc( "malloc", length );
	
	return ptr;
#endif
}

void         *
guarded_malloc (size_t length)
{
	char         *ptr = NULL ;

	if (length <= 0)
		length = 1;

#if defined(__CYGWIN__)
	/* lets do some magic with Win32 memory allocation to test for memory corruption : */
	ptr = alloc_guarded_memory( length );	   
#else
	ptr = malloc (length);
#endif

	if (ptr == (char *)0)
		failed_alloc( "malloc", length );
	
	return ptr;
}


void         *
saferealloc (void *orig_ptr, size_t length)
{
#if defined(__CYGWIN__) && defined(DEBUG_ALLOCS)
	return guarded_realloc( orig_ptr, length );
#else
	char         *ptr = NULL ;

	if (length <= 0)
		length = 1;

	ptr = realloc (orig_ptr, length);

	if (ptr == (char *)0)
		failed_alloc( "realloc", length );
	
	return ptr;
#endif
}

void         *
guarded_realloc (void *orig_ptr, size_t length)
{
	char         *ptr = NULL ;

	if (length <= 0)
		length = 1;

#if defined(__CYGWIN__)
	/* lets do some magic with Win32 memory allocation to test for memory corruption : */
	ptr = alloc_guarded_memory( length );	   
	if( orig_ptr && ptr ) 
	{
		size_t old_size = get_guarded_memory_size( orig_ptr );	
		if( length < old_size ) 
		{	
#ifdef LOCAL_DEBUG			
			fprintf( stderr, "ALLOC: trying to reallocate memory from %d bytes to %d bytes - truncating.\n", old_size, length );
#endif			
			old_size = length;
		}
		memcpy( ptr, orig_ptr, old_size );
		free_guarded_memory( orig_ptr );
	}
#else
	ptr = realloc (orig_ptr, length);
#endif

	if (ptr == (char *)0)
		failed_alloc( "guarded_realloc", length );
	
	return ptr;
}


void         *
safecalloc (size_t num, size_t blength)
{
#if defined(__CYGWIN__) && defined(DEBUG_ALLOCS)
	return guarded_calloc( num, blength );
#else
	char         *ptr;

    if (blength <= 0)
        blength = 1;
    
	if (num <= 0)
        num = 1;

	ptr = calloc (num, blength);
	if (ptr == (char *)0)
		failed_alloc( "calloc", num*blength );
	return ptr;
#endif
}

void         *
guarded_calloc (size_t num, size_t blength)
{
	char         *ptr;

    if (blength <= 0)
        blength = 1;
    
	if (num <= 0)
        num = 1;

#if defined(__CYGWIN__)
	ptr = alloc_guarded_memory( num * blength );	   
	if(ptr)
		memset( ptr, 0x00, num*blength );
#else    
	ptr = calloc (num, blength);
#endif

	if (ptr == (char *)0)
		failed_alloc( "guarded_calloc", num*blength );
	return ptr;
}


void
safefree (void *ptr)
{
    if (ptr)
#if defined(__CYGWIN__) && defined(DEBUG_ALLOCS)
		free_guarded_memory( ptr );		   
#else
		free (ptr);
#endif
}

void
guarded_free (void *ptr)
{
    if (ptr)
#if defined(__CYGWIN__)
		free_guarded_memory( ptr );		   
#else
		free (ptr);
#endif
}


void
dump_memory()
{
}

