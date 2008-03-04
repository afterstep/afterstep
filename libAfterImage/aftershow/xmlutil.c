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

ASHashTable *AfterShowVocabulary = NULL;
 = create_ashash( 7, casestring_hash_value, casestring_compare, string_destroy_without_data );

void 
aftershow_init_vocabulary (Bool free_resources)
{
	if (free_resources)
	{
		if (AfterShowVocabulary)
			destroy_ashash (&AfterShowVocabulary);
		return;
	}
	if (AfterShowVocabulary == NULL)
	{
		AfterShowVocabulary = create_ashash (7, casestring_hash_value, casestring_compare, string_destroy_without_data);
#define REGISTER_AFTERSHOW_TAG(tag)	add_hash_item(AfterShowVocabulary,AS_HASHABLE(#tag),AfterShow_##tag##_ID)
		REGISTER_AFTERSHOW_TAG(x);
		REGISTER_AFTERSHOW_TAG(y);
		REGISTER_AFTERSHOW_TAG(id);
		REGISTER_AFTERSHOW_TAG(layer);
		REGISTER_AFTERSHOW_TAG(width);
		REGISTER_AFTERSHOW_TAG(height);
		REGISTER_AFTERSHOW_TAG(parent);
		REGISTER_AFTERSHOW_TAG(screen);
		REGISTER_AFTERSHOW_TAG(window);
		REGISTER_AFTERSHOW_TAG(geometry);
#undef REGISTER_AFTERSHOW_TAG
	}
}

xml_elem_t *
aftershow_parse_xml_doc (const char *doc)
{
	return parse_xml_doc (doc, AfterShowVocabulary);
}


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

