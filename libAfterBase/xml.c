/*
 * Copyright (c) 2001 Sasha Vasko <sasha@aftercode.net>
 * Copyright (c) 2001 Eric Kowalski <eric@beancrock.net>
 * Copyright (c) 2001 Ethan Fisher <allanon@crystaltokyo.com>
 *
 * This module is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#undef LOCAL_DEBUG
#include "config.h"

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
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

#include "astypes.h"
#include "output.h"
#include "safemalloc.h"
#include "mystring.h"
#include "xml.h"
#include "selfdiag.h"
#include "ashash.h"
#include "audit.h"


char *interpret_ctrl_codes( char *text );

static char* cdata_str = XML_CDATA_STR;
static char* container_str = XML_CONTAINER_STR;
static ASHashTable *asxml_var = NULL;

void asxml_var_insert(const char* name, int value);

static char* lcstring(char* str) 
{
	char* ptr = str;
	for ( ; *ptr ; ptr++) if (isupper((int)*ptr)) *ptr = tolower((int)*ptr);
	return str;
}


void
asxml_var_init(void)
{
	if ( asxml_var == NULL )
	{
    	asxml_var = create_ashash(0, string_hash_value, string_compare, string_destroy_without_data);
    	if (!asxml_var) return;
#ifndef X_DISPLAY_MISSING
		{
			Display *dpy = get_current_X_display();
    		if ( dpy != NULL )
			{
    	    	asxml_var_insert("xroot.width",  XDisplayWidth (dpy, DefaultScreen(dpy)));
        		asxml_var_insert("xroot.height", XDisplayHeight(dpy, DefaultScreen(dpy)));
	      	}
		}
#endif
	}
}

void
asxml_var_insert(const char* name, int value)
{
	ASHashData hdata;

    if (!asxml_var) asxml_var_init();
    if (!asxml_var) return;

    /* Destroy any old data associated with this name. */
    remove_hash_item(asxml_var, AS_HASHABLE(name), NULL, True);

    show_progress("Defining var [%s] == %d.", name, value);

    hdata.i = value;
    add_hash_item(asxml_var, AS_HASHABLE(mystrdup(name)), hdata.vptr);
}

int
asxml_var_get(const char* name)
{
	ASHashData hdata = {0};

    if (!asxml_var) asxml_var_init();
    if (!asxml_var) return 0;
    if( get_hash_item(asxml_var, AS_HASHABLE(name), &hdata.vptr) != ASH_Success ) 
	{	
		show_debug(__FILE__, "asxml_var_get", __LINE__, "Use of undefined variable [%s].", name);
		return 0;
	}
    return hdata.i;
}

int
asxml_var_nget(char* name, int n) {
      int value;
      char oldc = name[n];
      name[n] = '\0';
      value = asxml_var_get(name);
      name[n] = oldc;
      return value;
}

void
asxml_var_cleanup(void)
{
	if ( asxml_var != NULL )
    	destroy_ashash( &asxml_var );

}


static int 
xml_name2id( const char *name, ASHashTable *vocabulary )
{
	ASHashData hdata;
	hdata.i = 0 ;
    get_hash_item(vocabulary, AS_HASHABLE(name), &hdata.vptr); 
	return hdata.i;		
}	 


xml_elem_t* xml_parse_parm(const char* parm, ASHashTable *vocabulary) {
	xml_elem_t* list = NULL;
	const char* eparm;

	if (!parm) return NULL;

	for (eparm = parm ; *eparm ; ) {
		xml_elem_t* p;
		const char* bname;
		const char* ename;
		const char* bval;
		const char* eval;

		/* Spin past any leading whitespace. */
		for (bname = eparm ; isspace((int)*bname) ; bname++);

		/* Check for a parm.  First is the parm name. */
		for (ename = bname ; xml_tagchar((int)*ename) ; ename++);

		/* No name equals no parm equals broken tag. */
		if (!*ename) { eparm = NULL; break; }

		/* No "=" equals broken tag.  We do not support HTML-style parms */
		/* with no value.                                                */
		for (bval = ename ; isspace((int)*bval) ; bval++);
		if (*bval != '=') { eparm = NULL; break; }

		while (isspace((int)*++bval));

		/* If the next character is a quote, spin until we see another one. */
		if (*bval == '"' || *bval == '\'') {
			char quote = *bval;
			bval++;
			for (eval = bval ; *eval && *eval != quote ; eval++);
		} else {
			for (eval = bval ; *eval && !isspace((int)*eval) ; eval++);
		}

		for (eparm = eval ; *eparm && !isspace((int)*eparm) ; eparm++);

		/* Add the parm to our list. */
		p = xml_elem_new();
		if (!list) list = p;
		else { p->next = list; list = p; }
		p->tag = lcstring(mystrndup(bname, ename - bname));
		if( vocabulary )
			p->tag_id = xml_name2id( p->tag, vocabulary );
		p->parm = mystrndup(bval, eval - bval);
	}

	if (!eparm) {
		while (list) {
			xml_elem_t* p = list->next;
			free(list->tag);
			free(list->parm);
			free(list);
			list = p;
		}
	}

	return list;
}

Bool xml_tags2xml_buffer( xml_elem_t *tags, ASXmlBuffer *xb, int tags_count, int depth);

#if 0 /* old version : */
/* The recursive version of xml_print(), so we can indent XML. */
static Bool xml_print_r(xml_elem_t* root, int depth) 
{
	xml_elem_t* child;
	Bool new_line = False, new_line_child = False ; 
	
	if (root->tag_id == XML_CDATA_ID || !strcmp(root->tag, cdata_str)) {
		char* ptr = root->parm;
		while (isspace((int)*ptr)) ptr++;
		fprintf(stderr, "%s", ptr);
	} else {
		if( root->child != NULL || root->next != NULL  ) 
		{
			new_line = True ;	  
			fprintf(stderr, "\n%*s", depth * 2, "");
		}
		fprintf(stderr, "<%s", root->tag);
		if (root->parm) {
			xml_elem_t* parm = xml_parse_parm(root->parm, NULL);
			while (parm) {
				xml_elem_t* p = parm->next;
				fprintf(stderr, " %s=\"%s\"", parm->tag, parm->parm);
				free(parm->tag);
				free(parm->parm);
				free(parm);
				parm = p;
			}
		}
		if (root->child) {
			fprintf(stderr, ">");
			for (child = root->child ; child ; child = child->next)
				if( xml_print_r(child, depth + 1) )
					new_line_child = True ;
			if( new_line_child ) 
				fprintf(stderr, "\n%*s", depth * 2, "");
			fprintf(stderr, "</%s>", root->tag);
		} else {
			fprintf(stderr, "/>");
		}
	}
	return new_line ;
}
#endif
void xml_print(xml_elem_t* root) 
{
	ASXmlBuffer xb;
	memset( &xb, 0x00, sizeof(xb));
	xml_tags2xml_buffer( root, &xb, -1, 0);
	add_xml_buffer_chars( &xb, "\0", 1 );
	printf ("%s", xb.buffer);
	free_xml_buffer_resources (&xb);
}

xml_elem_t* xml_elem_new(void) {
	xml_elem_t* elem = NEW(xml_elem_t);
	elem->next = elem->child = NULL;
	elem->parm = elem->tag = NULL;
	elem->tag_id = XML_UNKNOWN_ID ;
/*	LOCAL_DEBUG_OUT("elem = %p", elem); */
	return elem;
}

static xml_elem_t* xml_elem_remove(xml_elem_t** list, xml_elem_t* elem) {
	/* Splice the element out of the list, if it's in one. */
	if (list) {
		if (*list == elem) {
			*list = elem->next;
		} else {
			xml_elem_t* ptr;
			for (ptr = *list ; ptr->next ; ptr = ptr->next) {
				if (ptr->next == elem) {
					ptr->next = elem->next;
					break;
				}
			}
		}
	}
	elem->next = NULL;
	return elem;
}

void xml_elem_delete(xml_elem_t** list, xml_elem_t* elem) {
/*	LOCAL_DEBUG_OUT("elem = %p", elem); */

	if (list) xml_elem_remove(list, elem);
	while (elem) {
		xml_elem_t* ptr = elem;
		elem = elem->next;
		if (ptr->child) xml_elem_delete(NULL, ptr->child);
		if (ptr->tag && ptr->tag != cdata_str && ptr->tag != container_str) free(ptr->tag);
		if (ptr->parm) free(ptr->parm);
		free(ptr);
	}
}

xml_elem_t *
create_CDATA_tag()	
{ 
	xml_elem_t *cdata = xml_elem_new();
	cdata->tag = mystrdup(XML_CDATA_STR) ;
	cdata->tag_id = XML_CDATA_ID ;
	return cdata;
}

xml_elem_t *
create_CONTAINER_tag()	
{ 
	xml_elem_t *container = xml_elem_new();
	container->tag = mystrdup(XML_CONTAINER_STR) ;
	container->tag_id = XML_CONTAINER_ID ;
	return container;
}



xml_elem_t* xml_parse_doc(const char* str, ASHashTable *vocabulary) {
	xml_elem_t* elem = create_CONTAINER_tag();
	xml_parse(str, elem, vocabulary);
	return elem;
}

int xml_parse(const char* str, xml_elem_t* current, ASHashTable *vocabulary) {
	const char* ptr = str;
	
	xml_elem_t** tail = &(current->child);

	/* Find a tag of the form <tag opts>, </tag>, or <tag opts/>. */
	while (*ptr) {
		const char* oab = ptr;

		/* Look for an open oab bracket. */
		for (oab = ptr ; *oab && *oab != '<' ; oab++);

		/* If there are no oab brackets left, we're done. */
		if (*oab != '<') return oab - str;

		/* Does this look like a close tag? */
		if (oab[1] == '/') 
		{
			const char* etag;
			/* Find the end of the tag. */
			for (etag = oab + 2 ; xml_tagchar((int)*etag) ; etag++);

			while (isspace((int)*etag)) ++etag;
			/* If this is an end tag, and the tag matches the tag we're parsing, */
			/* we're done.  If not, continue on blindly. */
			if (*etag == '>') 
			{
				if (!mystrncasecmp(oab + 2, current->tag, etag - (oab + 2))) 
				{
					if (oab - ptr) 
					{
						xml_elem_t* child = create_CDATA_tag();
						child->parm = mystrndup(ptr, oab - ptr);
						*tail = child ; 
						tail = &(child->next);
						/* xml_insert(current, child); */
					}
					return (etag + 1) - str;
				}
			}

			/* This tag isn't interesting after all. */
			ptr = oab + 1;
		}

		/* Does this look like a start tag? */
		if (oab[1] != '/') {
			int empty = 0;
			const char* btag = oab + 1;
			const char* etag;
			const char* bparm;
			const char* eparm;

			/* Find the end of the tag. */
			for (etag = btag ; xml_tagchar((int)*etag) ; etag++);

			/* If we reached the end of the document, continue on. */
			if (!*etag) { ptr = oab + 1; continue; }

			/* Find the beginning of the parameters, if they exist. */
			for (bparm = etag ; isspace((int)*bparm) ; bparm++);

			/* From here on, we're looking for a sequence of parms, which have
			 * the form [a-z0-9-]+=("[^"]"|'[^']'|[^ \t\n]), followed by either
			 * a ">" or a "/>". */
			for (eparm = bparm ; *eparm ; ) {
				const char* tmp;

				/* Spin past any leading whitespace. */
				for ( ; isspace((int)*eparm) ; eparm++);

				/* Are we at the end of the tag? */
				if (*eparm == '>' || (*eparm == '/' && eparm[1] == '>')) break;

				/* Check for a parm.  First is the parm name. */
				for (tmp = eparm ; xml_tagchar((int)*tmp) ; tmp++);

				/* No name equals no parm equals broken tag. */
				if (!*tmp) { eparm = NULL; break; }

				/* No "=" equals broken tag.  We do not support HTML-style parms
				   with no value. */
				for ( ; isspace((int)*tmp) ; tmp++);
				if (*tmp != '=') { eparm = NULL; break; }

				while (isspace((int)*++tmp));

				/* If the next character is a quote, spin until we see another one. */
				if (*tmp == '"' || *tmp == '\'') {
					char quote = *tmp;
					for (tmp++ ; *tmp && *tmp != quote ; tmp++);
				}

				/* Now look for a space or the end of the tag. */
				for ( ; *tmp && !isspace((int)*tmp) && *tmp != '>' && !(*tmp == '/' && tmp[1] == '>') ; tmp++);

				/* If we reach the end of the string, there cannot be a '>'. */
				if (!*tmp) { eparm = NULL; break; }

				/* End of the parm.  */
				eparm = tmp;
				
				if (!isspace((int)*tmp)) break; 
				for ( ; isspace((int)*tmp) ; tmp++);
				if( *tmp == '>' || (*tmp == '/' && tmp[1] == '>') )
					break;
			}

			/* If eparm is NULL, the parm string is invalid, and we should
			 * abort processing. */
			if (!eparm) { ptr = oab + 1; continue; }

			/* Save CDATA, if there is any. */
			if (oab - ptr) {
				xml_elem_t* child = create_CDATA_tag();
				child->parm = mystrndup(ptr, oab - ptr);
				*tail = child ; 
				tail = &(child->next);
				/* xml_insert(current, child); */
			}

			/* We found a tag!  Advance the pointer. */
			for (ptr = eparm ; isspace((int)*ptr) ; ptr++);
			empty = (*ptr == '/');
			ptr += empty + 1;

			/* Add the tag to our children and parse it. */
			{
				xml_elem_t* child = xml_elem_new();
				child->tag = lcstring(mystrndup(btag, etag - btag));
				if( vocabulary )
					child->tag_id = xml_name2id( child->tag, vocabulary );
				if (eparm - bparm) child->parm = mystrndup(bparm, eparm - bparm);
				*tail = child ; 
				tail = &(child->next);
				/* xml_insert(current, child); */
				if (!empty) ptr += xml_parse(ptr, child, vocabulary);
			}
		}
	}
	return ptr - str;
}

void xml_insert(xml_elem_t* parent, xml_elem_t* child) {
	child->next = NULL;
	if (!parent->child) {
		parent->child = child;
		return;
	}
	for (parent = parent->child ; parent->next ; parent = parent->next);
	parent->next = child;
}


xml_elem_t *
find_tag_by_id( xml_elem_t *chain, int id )
{
	while( chain ) 
	{	
		if( chain->tag_id == id ) 
			return chain;
		chain = chain->next ;
	}
	return NULL ;
}

char *interpret_ctrl_codes( char *text )
{
	register char *ptr = text ;
	int len, curr = 0 ;
	if( ptr == NULL )  return NULL ;	

	len = strlen(ptr);
	while( ptr[curr] != '\0' ) 
	{
		if( ptr[curr] == '\\' && ptr[curr+1] != '\0' ) 	
		{
			char subst = '\0' ;
			switch( ptr[curr+1] ) 
			{
				case '\\': subst = '\\' ;  break ;	
				case 'a' : subst = '\a' ;  break ;	 
				case 'b' : subst = '\b' ;  break ;	 
				case 'f' : subst = '\f' ;  break ;	 
				case 'n' : subst = '\n' ;  break ;	 
				case 'r' : subst = '\r' ;  break ;	
				case 't' : subst = '\t' ;  break ;	
				case 'v' : subst = '\v' ;  break ;	 
			}	 
			if( subst ) 
			{
				register int i = curr ; 
				ptr[i] = subst ;
				while( ++i < len ) 
					ptr[i] = ptr[i+1] ; 
				--len ; 
			}
		}	 
		++curr ;
	}	 
	return text;
}	 

void reset_xml_buffer( ASXmlBuffer *xb )
{
	if( xb ) 
	{
		xb->current = xb->used = 0 ; 
		xb->state = ASXML_Start	 ;
		xb->level = 0 ;
		xb->verbatim = False ;
		xb->quoted = False ;
		xb->tag_type = ASXML_OpeningTag ;
		xb->tags_count = 0 ;
	}		  
}	 

void 
free_xml_buffer_resources (ASXmlBuffer *xb)
{
	if (xb && xb->buffer)
	{
		free (xb->buffer);
		xb->allocated = xb->current = xb->used = 0 ; 
		xb->buffer = NULL;
	}
}

static inline void
realloc_xml_buffer( ASXmlBuffer *xb, int len )
{
	if( xb->used + len > xb->allocated ) 
	{	
		xb->allocated = xb->used + (((len>>11)+1)<<11) ;	  
		xb->buffer = realloc( xb->buffer, xb->allocated );
	}
}
void 
add_xml_buffer_chars( ASXmlBuffer *xb, char *tmp, int len )
{
	realloc_xml_buffer (xb, len);
	memcpy( &(xb->buffer[xb->used]), tmp, len );
	xb->used += len ;
}

void 
add_xml_buffer_spaces( ASXmlBuffer *xb, int len )
{
	if (len > 0)
	{
		realloc_xml_buffer (xb, len);
		memset( &(xb->buffer[xb->used]), ' ', len );
		xb->used += len ;
	}
}

void 
add_xml_buffer_open_tag( ASXmlBuffer *xb, xml_elem_t *tag )
{
	int tag_len = strlen (tag->tag);
	int parm_len = 0;
	xml_elem_t* parm = NULL ; 
	
	if (tag->parm)
	{
		xml_elem_t *t = parm = xml_parse_parm(tag->parm, NULL);
		while (t)
		{
			parm_len += 1 + strlen(t->tag) + 1 + 1 + strlen(t->parm) + 1;
			t = t->next;
		}
	}
	realloc_xml_buffer (xb, 1+tag_len+1+parm_len+2);
	xb->buffer[(xb->used)++] = '<';
	memcpy (&(xb->buffer[xb->used]), tag->tag, tag_len);
	xb->used += tag_len ;

	while (parm) 
	{
		xml_elem_t* p = parm->next;
		int len;
		xb->buffer[(xb->used)++] = ' ';
		for (len = 0 ; parm->tag[len] ; ++len)
			xb->buffer[xb->used+len] = parm->tag[len];
		xb->used += len ;
		xb->buffer[(xb->used)++] = '=';
		xb->buffer[(xb->used)++] = '\"';
		for (len = 0 ; parm->parm[len] ; ++len)
			xb->buffer[xb->used+len] = parm->parm[len];
		xb->used += len ;
		xb->buffer[(xb->used)++] = '\"';
		free(parm->tag);
		free(parm->parm);
		free(parm);
		parm = p;
	}

	if (tag->child == NULL)
		xb->buffer[(xb->used)++] = '/';
	xb->buffer[(xb->used)++] = '>';
}

void 
add_xml_buffer_close_tag( ASXmlBuffer *xb, xml_elem_t *tag )
{
	int tag_len = strlen (tag->tag);
	realloc_xml_buffer (xb, tag_len+3);
	xb->buffer[(xb->used)++] = '<';
	xb->buffer[(xb->used)++] = '/';
	memcpy (&(xb->buffer[xb->used]), tag->tag, tag_len);
	xb->used += tag_len ;
	xb->buffer[(xb->used)++] = '>';
}

int 
spool_xml_tag( ASXmlBuffer *xb, char *tmp, int len )
{
	register int i = 0 ; 
	
	if( !xb->verbatim && !xb->quoted && 
		(xb->state != ASXML_Start || xb->level == 0 )) 
	{	/* skip spaces if we are not in string */
		while( i < len && isspace( (int)tmp[i] )) ++i;
		if( i >= len ) 
			return i;
	}
	if( xb->state == ASXML_Start ) 
	{     /* we are looking for the opening '<' */
		if( tmp[i] != '<' ) 
		{
			if( xb->level == 0 ) 	  
				xb->state = ASXML_BadStart ; 
			else
			{
				int start = i ; 
				while( i < len && tmp[i] != '<' ) ++i ;	  
				add_xml_buffer_chars( xb, &tmp[start], i - start );
				return i;
			}
		}else
		{	
			xb->state = ASXML_TagOpen; 	
			xb->tag_type = ASXML_OpeningTag ;
			add_xml_buffer_chars( xb, "<", 1 );
			if( ++i >= len ) 
				return i;
		}
	}
	
	if( xb->state == ASXML_TagOpen ) 
	{     /* we are looking for the beginning of tag name  or closing tag's slash */
		if( tmp[i] == '/' ) 
		{
			xb->state = ASXML_TagName; 
			xb->verbatim = True ; 		   
			xb->tag_type = ASXML_ClosingTag ;
			add_xml_buffer_chars( xb, "/", 1 );
			if( ++i >= len ) 
				return i;
		}else if( isalnum((int)tmp[i]) )	
		{	 
			xb->state = ASXML_TagName; 		   
			xb->verbatim = True ; 		   
		}else
			xb->state = ASXML_BadTagName ;
	}

	if( xb->state == ASXML_TagName ) 
	{     /* we are looking for the tag name */
		int start = i ;
		/* need to store attribute name in form : ' attr_name' */
		while( i < len && isalnum((int)tmp[i]) ) ++i ;
		if( i > start ) 
			add_xml_buffer_chars( xb, &tmp[start], i - start );
		if( i < len ) 
		{	
			if( isspace( (int)tmp[i] ) || tmp[i] == '>' ) 
			{
				xb->state = ASXML_TagAttrOrClose;
				xb->verbatim = False ; 
			}else
				xb->state = ASXML_BadTagName ;
		}			 
		return i;
	}

	if( xb->state == ASXML_TagAttrOrClose ) 
	{   /* we are looking for the attribute or closing '/>' or '>' */
		Bool has_slash = (xb->tag_type != ASXML_OpeningTag);

		if( !has_slash && tmp[i] == '/' )
		{	
			xb->tag_type = ASXML_SimpleTag ;
			add_xml_buffer_chars( xb, "/", 1 );		 			  
			++i ;
			has_slash = True ;
		}
		if( i < len ) 
		{	
			if( has_slash && tmp[i] != '>') 
				xb->state = ASXML_UnexpectedSlash ;	  
			else if( tmp[i] == '>' ) 
			{
				++(xb->tags_count);
				xb->state = ASXML_Start; 	
	 			add_xml_buffer_chars( xb, ">", 1 );		 			  
				++i ;
				if( xb->tag_type == ASXML_OpeningTag )
					++(xb->level);
				else if( xb->tag_type == ASXML_ClosingTag )					
				{
					if( xb->level <= 0 )
					{
				 		xb->state = ASXML_UnmatchedClose;
						return i;		   
					}else
						--(xb->level);			
				}		 			   
			}else if( !isalnum( (int)tmp[i] ) )	  
				xb->state = ASXML_BadAttrName ;
			else
			{	
				xb->state = ASXML_AttrName;		 
				xb->verbatim = True ;
				add_xml_buffer_chars( xb, " ", 1);
			}
		}
		return i;
	}

	if( xb->state == ASXML_AttrName ) 
	{	
		int start = i ;
		/* need to store attribute name in form : ' attr_name' */
		while( i < len && isalnum((int)tmp[i]) ) ++i ;
		if( i > start ) 
			add_xml_buffer_chars( xb, &tmp[start], i - start );
		if( i < len ) 
		{	
			if( isspace( (int)tmp[i] ) || tmp[i] == '=' ) 
			{
				xb->state = ASXML_AttrEq;
				xb->verbatim = False ; 
				/* should fall down to case below */
			}else
				xb->state = ASXML_BadAttrName ;
		}
	 	return i;				 
	}	

	if( xb->state == ASXML_AttrEq )                   /* looking for '=' */
	{
		if( tmp[i] == '=' ) 
		{
			xb->state = ASXML_AttrValueStart;				
			add_xml_buffer_chars( xb, "=", 1 );		 			  
			++i ;
		}else	 
			xb->state = ASXML_MissingAttrEq ;
		return i;
	}	
	
	if( xb->state == ASXML_AttrValueStart )/*looking for attribute value:*/
	{
		xb->state = ASXML_AttrValue ;
		if( tmp[i] == '"' )
		{
			xb->quoted = True ; 
			add_xml_buffer_chars( xb, "\"", 1 );
			++i ;
		}else	 
			xb->verbatim = True ; 
		return i;
	}	  
	
	if( xb->state == ASXML_AttrValue )  /* looking for attribute value : */
	{
		if( !xb->quoted && isspace((int)tmp[i]) ) 
		{
			add_xml_buffer_chars( xb, " ", 1 );
			++i ;
			xb->verbatim = False ; 
			xb->state = ASXML_TagAttrOrClose ;
		}else if( xb->quoted && tmp[i] == '"' ) 
		{
			add_xml_buffer_chars( xb, "\"", 1 );
			++i ;
			xb->quoted = False ; 
			xb->state = ASXML_TagAttrOrClose ;
		}else if( tmp[i] == '/' && !xb->quoted)
		{
			xb->state = ASXML_AttrSlash ;				
			add_xml_buffer_chars( xb, "/", 1 );		 			  
			++i ;
		}else if( tmp[i] == '>' )
		{
			xb->quoted = False ; 
			xb->verbatim = False ; 
			xb->state = ASXML_TagAttrOrClose ;				
		}else			
		{
			add_xml_buffer_chars( xb, &tmp[i], 1 );
			++i ;
		}
		return i;
	}	  
	if( xb->state == ASXML_AttrSlash )  /* looking for attribute value : */
	{
		if( tmp[i] == '>' )
		{
			xb->tag_type = ASXML_SimpleTag ;
			add_xml_buffer_chars( xb, ">", 1 );		 			  
			++i ;
			++(xb->tags_count);
			xb->state = ASXML_Start; 	
			xb->quoted = False ; 
			xb->verbatim = False ; 
		}else
		{
			xb->state = ASXML_AttrValue ;
		}		 
		return i;
	}

	return (i==0)?1:i;
}	   

/* reverse transformation - put xml tags into a buffer */
Bool 
xml_tags2xml_buffer( xml_elem_t *tags, ASXmlBuffer *xb, int tags_count, int depth)
{
	Bool new_line = False; 

	while (tags && tags_count != 0) /* not a bug - negative tags_count means unlimited !*/
	{
		if (tags->tag_id == XML_CDATA_ID || !strcmp(tags->tag, cdata_str)) 
		{
			/* TODO : add handling for cdata with quotes, amps and gt, lt */
			add_xml_buffer_chars( xb, tags->parm, strlen(tags->parm));
		}else 
		{
			if (depth >= 0 && (tags->child != NULL || tags->next != NULL)) 
			{
				add_xml_buffer_chars( xb, "\n", 1);
				add_xml_buffer_spaces( xb, depth*2);
				new_line = True ;	  
			}
			add_xml_buffer_open_tag( xb, tags);

			if (tags->child) 
			{
				if( xml_tags2xml_buffer( tags->child, xb, -1, (depth < 0)?-1:depth+1 ))
				{
					if (depth >= 0)
					{
						add_xml_buffer_chars( xb, "\n", 1);
						add_xml_buffer_spaces( xb, depth*2);
					}
				}
				add_xml_buffer_close_tag( xb, tags);
			}
		}		
		tags = tags->next;
		--tags_count;
	}
	return new_line;
}



xml_elem_t *
format_xml_buffer_state (ASXmlBuffer *xb)
{
	xml_elem_t *state_xml = NULL; 
	if (xb->state < 0) 
	{
		state_xml = xml_elem_new();
		state_xml->tag = mystrdup("error");
		state_xml->parm = safemalloc (64);
		sprintf(state_xml->parm, "code=%d level=%d tag_count=%d", xb->state, xb->level ,xb->tags_count );
		state_xml->child = create_CDATA_tag();
		switch( xb->state ) 
		{
			case ASXML_BadStart : state_xml->child->parm = mystrdup("Text encountered before opening tag bracket - not XML format"); break;
			case ASXML_BadTagName : state_xml->child->parm = mystrdup("Invalid characters in tag name" );break;
			case ASXML_UnexpectedSlash : state_xml->child->parm = mystrdup("Unexpected '/' encountered");break;
			case ASXML_UnmatchedClose : state_xml->child->parm = mystrdup("Closing tag encountered without opening tag" );break;
			case ASXML_BadAttrName : state_xml->child->parm = mystrdup("Invalid characters in attribute name" );break;
			case ASXML_MissingAttrEq : state_xml->child->parm = mystrdup("Attribute name not followed by '=' character" );break;
			default:
				state_xml->child->parm = mystrdup("Premature end of the input");break;
		}
	}else if (xb->state == ASXML_Start)
	{
		if (xb->tags_count > 0)
		{
			state_xml = xml_elem_new();
			state_xml->tag = mystrdup("success");
			state_xml->parm = safemalloc(64);
			sprintf(state_xml->parm, "tag_count=%d level=%d", xb->tags_count, xb->level );
		}
	}else
	{
		/* TODO */
	}
	return state_xml;
}

char 
translate_special_sequence( const char *ptr, int len,  int *seq_len )
{
	int c = '\0' ;
	int c_len = 0 ;
	if( ptr[0] == '&') 
	{ 
		if( 4 <= len ) 
		{	
			c_len = 4 ;
			if( strncmp(ptr,"&lt;", c_len ) == 0 ) c = '<' ;
			else if( strncmp(ptr,"&gt;", c_len ) == 0 ) c = '>' ;
		}
		if( c == '\0' && 5 <= len ) 
		{	
			c_len = 5 ;
			if( strncmp(ptr,"&amp;", c_len ) == 0 ) c = '&' ;
		}
				
		if( c == '\0' && 6 <= len ) 
		{	
			c_len = 6 ;
			if(      strncmp(ptr,"&quot;", c_len ) == 0 ) c = '"' ;
			else if( strncmp(ptr,"&circ;", c_len ) == 0 ) c = 'ˆ' ;
			else if( strncmp(ptr,"&nbsp;", c_len ) == 0 ) c = ' ' ;
			else if( strncmp(ptr,"&ensp;", c_len ) == 0 ) c = ' ' ;
			else if( strncmp(ptr,"&emsp;", c_len ) == 0 ) c = ' ' ;
			else if( strncmp(ptr,"&Yuml;", c_len ) == 0 ) c = 'Ÿ' ;
			else if( strncmp(ptr,"&euro;", c_len ) == 0 ) c = '€' ;					 
		}

		if( c == '\0' && 7 <= len ) 
		{	
			c_len = 7 ;
			if( strncmp(ptr,"&OElig;", c_len ) == 0 ) c = 'Œ' ;
			else if( strncmp(ptr,"&oelig;", c_len ) == 0 ) c = 'œ' ;
			else if( strncmp(ptr,"&tilde;", c_len ) == 0 ) c = '˜' ;
			else if( strncmp(ptr,"&ndash;", c_len ) == 0 ) c = '–' ;
			else if( strncmp(ptr,"&mdash;", c_len ) == 0 ) c = '—' ;
			else if( strncmp(ptr,"&lsquo;", c_len ) == 0 ) c = '‘' ;
			else if( strncmp(ptr,"&rsquo;", c_len ) == 0 ) c = '’' ;
			else if( strncmp(ptr,"&sbquo;", c_len ) == 0 ) c = '‚' ;
			else if( strncmp(ptr,"&ldquo;", c_len ) == 0 ) c = '“' ;
			else if( strncmp(ptr,"&rdquo;", c_len ) == 0 ) c = '”' ;
			else if( strncmp(ptr,"&bdquo;", c_len ) == 0 ) c = '„' ;
		}				
		if( c == '\0' && 8 <= len ) 
		{	
			c_len = 8 ;
			if( strncmp(ptr,"&Scaron;", c_len ) == 0 ) c = 'Š' ;
			else if( strncmp(ptr,"&scaron;", c_len ) == 0 ) c = 'š' ;
			else if( strncmp(ptr,"&thinsp;", c_len ) == 0 ) c = ' ' ;
			else if( strncmp(ptr,"&dagger;", c_len ) == 0 ) c = '†' ;
			else if( strncmp(ptr,"&Dagger;", c_len ) == 0 ) c = '‡' ;
			else if( strncmp(ptr,"&permil;", c_len ) == 0 ) c = '‰' ;
			else if( strncmp(ptr,"&lsaquo;", c_len ) == 0 ) c = '‹' ;
			else if( strncmp(ptr,"&rsaquo;", c_len ) == 0 ) c = '›' ;
		}
	}		
						
	if( seq_len )    
		*seq_len = (c == '\0')?0:c_len ;
	return c;   				 
}

void	   
append_cdata( xml_elem_t *cdata_tag, const char *line, int len )
{
	int i, k; 
	int tabs_count = 0 ;
	int old_length = 0;
	char *ptr ;
	
	for( i = 0 ; i < len ; ++i ) 
		if( line[i] == '\t' )
			++tabs_count ;
	if( cdata_tag->parm ) 
		old_length = strlen(cdata_tag->parm);

	cdata_tag->parm = realloc( cdata_tag->parm, old_length + 1 + len + tabs_count*3+1);
	ptr = &(cdata_tag->parm[old_length]) ;
	if( old_length > 0 )
		if( cdata_tag->parm[old_length-1] != '\n') 
		{	
			ptr[0] = '\n' ;
			++ptr ;
		}
	k = 0 ;
	for( i = 0 ; i < len ; ++i ) 
	{	
		if( line[i] == '\t' )
		{
			int tab_stop = (((k+3)/4)*4) ; 
			if( tab_stop == k ) 
				tab_stop += 4 ;
/*			fprintf( stderr, "k = %d, tab_stop = %d, len = %d\n", k, tab_stop, len ); */
			while( k < tab_stop )
				ptr[k++] = ' ' ;
		}else if( line[i] == '\n' )
		{
			ptr[k] = '\n' ;
			ptr += k+1 ;
			k = 0 ;	  
		}else
			ptr[k++] = line[i] ;
	}		
	ptr[k] = '\0';
}	 


void 
append_CDATA_line( xml_elem_t *tag, const char *line, int len )
{
	xml_elem_t *cdata_tag = find_tag_by_id(tag->child, XML_CDATA_ID );
	LOCAL_DEBUG_CALLER_OUT("tag->tag = \"%s\", line_len = %d", tag->tag, len ); 

	if( cdata_tag == NULL ) 
	{
		cdata_tag = create_CDATA_tag() ;
		xml_insert(tag, cdata_tag);
	}	 
	append_cdata( cdata_tag, line, len );
}

