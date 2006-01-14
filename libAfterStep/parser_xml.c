/*
 * Copyright (c) 2005 Sasha Vasko <sasha at aftercode.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.   See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#define LOCAL_DEBUG
#undef DO_CLOCKING
#undef UNKNOWN_KEYWORD_WARNING

#include "../configure.h"

#include <fcntl.h>
#include <unistd.h>

#ifdef DO_CLOCKING
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
#endif
#include "asapp.h"
#include "afterstep.h"
#include "functions.h"
#include "parser.h"
#include "../libAfterBase/xml.h"


/* AfterStep config into xml tree code : */

void 
append_comments_tag( xml_elem_t  *parent, const char *comments )
{
	xml_elem_t *c_tag = xml_elem_new(); 
	c_tag->tag = mystrdup("comments"); 
	//c_tag->tag_id = pterm->id ;
	xml_insert( parent, c_tag );

	c_tag->child = create_CDATA_tag();	  
	c_tag->child->parm = mystrdup( comments );
}	 

void
statement2xml_elem (ConfigDef * config)
{
	xml_elem_t  *item, **tail = config->current_tail->storage;
	TermDef      *pterm = config->current_term;
	char *ptr = config->current_data;
	char *name  = NULL ; 
	char *side  = NULL ; 
	char *index = NULL ; 
	int parm_len = 0 ; 
	char *comments = NULL ;

LOCAL_DEBUG_OUT( "checking for foreign option ...%s", "" );
	if (IsForeignOption (config))
		return;

	item = xml_elem_new(); 
	item->tag = pterm->keyword[0]?mystrdup(pterm->keyword):mystrdup("item");
	LOCAL_DEBUG_OUT( "adding storage ... pterm is \"%s\" (%s)", item->tag, get_flags(pterm->flags, TF_NAMED)?"named":"");
	item->tag_id = pterm->id ;
	*tail = item ; 
    
	print_trimmed_str( "ptr", ptr );

	if( ptr )
	{	
		if ( get_flags(pterm->flags, TF_NAMED) )
		{
			ptr = parse_filename (ptr, &name);
		}else if (get_flags(pterm->flags, TF_DIRECTION_INDEXED))
		{
			ptr = parse_filename (ptr, &side);
		}else if (get_flags(pterm->flags, TF_INDEXED))
		{
			int i = 0 ; 
			while( isdigit(ptr[i]) ) ++i;
			if( i > 0 ) 
			{	
        		index = mystrndup(ptr, i);
				ptr = tokenskip( ptr, 1 );
			}
		}	 

		if( name )
			parm_len = sizeof(" name=") + 1+strlen(name)+1+1;
		if( side )
			parm_len += sizeof(" side=") + 1+strlen(side)+1+1;
		if( index )
			parm_len += sizeof(" index=") +1+strlen(index)+1+1;

		if( parm_len > 0 ) 
		{
			char *parm_ptr = safemalloc( parm_len );
			item->parm = parm_ptr ; 
			if( name ) 
			{	
				sprintf( parm_ptr,"name=\"%s\"", name );	   
				while( *parm_ptr ) ++parm_ptr ;
				destroy_string( &name );
			}
			if( side ) 
			{	  
				sprintf( parm_ptr,"side=\"%s\"", side );	   
				while( *parm_ptr ) ++parm_ptr ;
				destroy_string( &side ); 
			}
			if( index ) 
			{	
				sprintf( parm_ptr,"index=\"%s\"", index );	   
				destroy_string( &index );
			}	 
			LOCAL_DEBUG_OUT( "parm = \"%s\"", item->parm );
		}	 

	}
//	if ((pNext = AddFreeStorageElem (config->syntax, config->current_tail->storage, pterm, ID_ANY)) == NULL)
//		return;
//LOCAL_DEBUG_OUT( "parsing stuff ...%s", "" );
//	pNext->flags = config->current_flags;

	if (config->current_data_len > 0)
	{
		if(!get_flags(pterm->flags, TF_DONT_REMOVE_COMMENTS) )
		{
			ptr = stripcomments2 (ptr, &comments);
		    /* stripcomments2 modifies original string by 0-terminating 
		     * it at comments point - kinda nasty */
			config->current_data_len = strlen (config->current_data);
		}
	}
	if( pterm->sub_syntax == NULL && ptr && ptr[0] ) 
	{                          /* add value as CDATA */
		item->child = create_CDATA_tag();	  
		item->child->parm = mystrdup( ptr );
	}
	/* otherwise value will be present in child tags */
    print_trimmed_str( "config->current_data", config->current_data );
    //LOCAL_DEBUG_OUT( "curr_data_len = %d", config->current_data_len);

	if( comments ) 
	{
		append_comments_tag( item, comments );		
		destroy_string( &comments );
	}	 
//	args2FreeStorage (pNext, config->current_data, config->current_data_len);
	config->current_tail->storage = &(item->next);
	if (pterm->sub_syntax)
	{	
		tail = &(item->child);
		while( *tail ) tail = &((*tail)->next);
		ProcessSubSyntax (config, tail, pterm->sub_syntax);
	}
	return;
}


xml_elem_t*
file2xml_tree(const char *filename, char *myname, SyntaxDef *syntax, SpecialFunc special )
{
	ConfigData cd ;
	ConfigDef    *config_reader ;
	xml_elem_t *tree = NULL ;
	
	cd.filename = filename ;
	config_reader = InitConfigReader (myname, syntax, CDT_Filename, cd, special);
	if ( config_reader != NULL )
	{
		config_reader->statement_handler = statement2xml_elem;
		tree = create_CONTAINER_tag();
		tree->child = xml_elem_new(); 
		tree->child->tag = mystrdup(syntax->display_name);
		//c_tag->tag_id = pterm->id ;
		config2tree_storage(config_reader, (ASTreeStorageModel **)&(tree->child->child));	   
		DestroyConfig (config_reader);   
	}
	return tree;
}	 


#ifdef TEST_PARSER_XML

#include "../libAfterConf/afterconf.h"

# define CONFIG_FILE	"~/.afterstep/wharf"
# define CONFIG_SYNTAX	&WharfSyntax
# define CONFIG_SPECIAL	WharfSpecialFunc
# define CONFIG_MYNAME  "Wharf"

int 
main( int argc, char ** argv ) 
{
	char *fullfilename;
	xml_elem_t* tree ;
	InitMyApp ("TestParserXML", argc, argv, NULL, NULL, 0 );
	LinkAfterStepConfig();
	InitSession();
	
	fullfilename = PutHome( CONFIG_FILE );

	tree = file2xml_tree(fullfilename, CONFIG_MYNAME, CONFIG_SYNTAX, CONFIG_SPECIAL );
	
	xml_print(tree);	   
	xml_elem_delete(NULL, tree);

	FreeMyAppResources();
#   ifdef DEBUG_ALLOCS
	print_unfreed_mem ();
#   endif /* DEBUG_ALLOCS */
	return 1;
}
#endif

