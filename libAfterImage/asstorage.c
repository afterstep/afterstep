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

#ifdef _WIN32
# include "win32/afterbase.h"
#else
# include "afterbase.h"
#endif

#include "asstorage.h"

ASStorage *
create_asstorage()
{
	ASStorage *storage = safecalloc(1, sizeof(ASStorage));

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
	/* TODO */
	return 0;
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
							if(val){ printf("failed\n"); return False;} \
							else printf("success.\n")}while(0)
	TEST_EVAL( storage != NULL ); 
	
	printf("Testing store_data for data %p size = %d, and flags 0x%X...", test_data, test_data_size,
			test_flags);
	id = store_data( storage, test_data, test_data_size, 0 );
	TEST_EVAL( id == 0 ); 


	printf("Testing storage destruction ...");
	destroy_asstorage(&storage);
	TEST_EVAL( storage == NULL ); 
}
