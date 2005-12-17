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

#define LOCAL_DEBUG
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

void
asxml_var_init(void)
{
	if ( asxml_var == NULL )
	{
    	asxml_var = create_ashash(0, string_hash_value, string_compare, string_destroy_without_data);
    	if (!asxml_var) return;
    	if ( dpy != NULL )
		{
        	asxml_var_insert("xroot.width",  XDisplayWidth (dpy, DefaultScreen(dpy)));
        	asxml_var_insert("xroot.height", XDisplayHeight(dpy, DefaultScreen(dpy)));
      	}
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

/* The recursive version of xml_print(), so we can indent XML. */
static void xml_print_r(xml_elem_t* root, int depth) {
	xml_elem_t* child;
	if (!strcmp(root->tag, cdata_str)) {
		char* ptr = root->parm;
		while (isspace((int)*ptr)) ptr++;
		fprintf(stderr, "%s", ptr);
	} else {
		fprintf(stderr, "%*s<%s", depth * 2, "", root->tag);
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
			fprintf(stderr, ">\n");
			for (child = root->child ; child ; child = child->next)
				xml_print_r(child, depth + 1);
			fprintf(stderr, "%*s</%s>\n", depth * 2, "", root->tag);
		} else {
			fprintf(stderr, "/>\n");
		}
	}
}

void xml_print(xml_elem_t* root) {
	xml_print_r(root, 0);
}

xml_elem_t* xml_elem_new(void) {
	xml_elem_t* elem = NEW(xml_elem_t);
	elem->next = elem->child = NULL;
	elem->parm = elem->tag = NULL;
	elem->tag_id = XML_UNKNOWN_ID ;
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
						xml_insert(current, child);
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
				xml_insert(current, child);
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
				xml_insert(current, child);
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

static char* lcstring(char* str) 
{
	char* ptr = str;
	for ( ; *ptr ; ptr++) if (isupper((int)*ptr)) *ptr = tolower((int)*ptr);
	return str;
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
		xb->used = 0 ; 
		xb->state = ASXML_Start	 ;
		xb->level = 0 ;
		xb->verbatim = False ;
		xb->quoted = False ;
		xb->tag_type = ASXML_OpeningTag ;
		xb->tags_count = 0 ;
	}		  
}	 


void 
add_xml_buffer_chars( ASXmlBuffer *xb, char *tmp, int len )
{
	if( xb->used + len > xb->allocated ) 
	{	
		xb->allocated = xb->used + (((len>>11)+1)<<11) ;	  
		xb->buffer = realloc( xb->buffer, xb->allocated );
	}
	memcpy( &(xb->buffer[xb->used]), tmp, len );
	xb->used += len ;
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
	{   /* we are looking for the atteribute or closing '/>' or '>' */
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

