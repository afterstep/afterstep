/*
 * Copyright (c) 2008 Sasha Vasko <sasha at aftercode.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License.
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

/*********************************************************************************
 * Utility functions to handle X stuff
 *********************************************************************************/

#ifdef _WIN32
#include "../win32/config.h"
#else
#include "../config.h"
#endif

#undef LOCAL_DEBUG

#include <string.h>

#include "../afterbase.h"
#include "../afterimage.h"
#include "aftershow.h"

void 
aftershow_add_tags_to_queue( xml_elem_t* tags, xml_elem_t **phead, xml_elem_t **ptail)
{
	if (*phead == NULL)
		*ptail = *phead = tags;
	else
		(*ptail)->next = tags;
	
	while ((*ptail)->next) *ptail = (*ptail)->next;
}

/*********************************************************************************
 * The end !!!! 																 
 ********************************************************************************/

