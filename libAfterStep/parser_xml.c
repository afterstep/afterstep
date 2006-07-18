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
#define DO_CLOCKING
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
char *_as_config_tags[TT_VALUE_TYPES] = 
{
	"unknown",
 	"flag",
 	"int",
 	"uint",
 	"color",	
 	"font",
 	"filename",
 	"pathname",
 	"geometry",
 	"text",

	"quoted_text",
 	"optional_pathname",
 	"icon_list",
 	"direction",
 	"function",
 	"box",	
 	"button",
 	"binding",
 	"bitlist",
 	"intarray",
 	
	"cursor",
 	"comment",
 	"inline_comment"
};	 


#define ASXMLOpt_moduleAttrName "module"
					
#define ASXMLOpt_indexAttrName  "index"
#define ASXMLOpt_sideAttrName  "side" 
#define ASXMLOpt_nameAttrName  "name" 


void 
append_asconfig_value_tag( xml_elem_t  *parent, int type, char **val )
{
	xml_elem_t *v_tag = NULL;
	if( type >= 0 && type < TT_VALUE_TYPES ) 
	{	
		v_tag = xml_elem_new(); 
		v_tag->tag = mystrdup(_as_config_tags[type]); 
		v_tag->tag_id = type ;
		if( val && *val ) 
		{	
			v_tag->child = create_CDATA_tag();	  
			v_tag->child->parm = *val ;
			*val = NULL ;
		}
	}else
	{		
		v_tag = create_CDATA_tag();
		if( val && *val ) 
		{	
			v_tag->parm = *val ;
			*val = NULL ;
		}
	}
	xml_insert( parent, v_tag );
}	 

void
statement2xml_elem (ConfigDef * config)
{
	xml_elem_t  *item, **tail = config->current_tail->storage;
	TermDef      *pterm = config->current_term;
	char *ptr = config->current_data;
	char *name  = NULL ; 
	char *module  = NULL; 
	char *side  = NULL ; 
	char *index = NULL ; 
	int parm_len = 0 ; 
	char *comments = NULL ;

	if( get_flags(pterm->flags, TF_SYNTAX_TERMINATOR) ) 
		return;                /* no need to terminate XML structure */
		
	item = xml_elem_new(); 
	*tail = item ; 
	config->current_tail->storage = &(item->next);
	
	if ( pterm->type == TT_COMMENT || pterm->type == TT_ANY )
	{	
		item->tag = mystrdup(_as_config_tags[pterm->type]); 
		item->tag_id = pterm->type ;
		if (config->current_data_len > 0)
		{	
			char *cdata = NULL ;
			if( pterm->type == TT_COMMENT ) 
				cdata = mystrdup(config->current_data);
			else
			{
				char *token = NULL ; 
				parse_token( config->tline, &token );
				cdata = safemalloc( 1+strlen( token )+1+config->current_data_len +1);
				sprintf( cdata, get_flags( config->current_flags, CF_PUBLIC_OPTION)?"*%s %s":"%s %s", token, config->current_data );
				free( token );
			}	 
			item->child = create_CDATA_tag();	  
			item->child->parm = cdata;
		}
		return;
	}

	item->tag = pterm->keyword[0]?mystrdup(pterm->keyword):mystrdup("item");
	LOCAL_DEBUG_OUT( "adding storage ... pterm is \"%s\" (%s)", item->tag, get_flags(pterm->flags, TF_NAMED)?"named":"");
	item->tag_id = pterm->id ;
    
	if( get_flags( config->current_flags, CF_FOREIGN_OPTION ) ) 
	{
		int token_len ;
		parse_token( config->tline, &module );
		LOCAL_DEBUG_OUT( "foreign option beginning with token \"%s\"", module);
		token_len = strlen(module); 
		if( token_len > 0 ) 
		{	
			if( token_len > pterm->keyword_len && pterm->keyword_len > 0 ) 
				if( mystrncasecmp( &module[token_len-pterm->keyword_len], pterm->keyword, pterm->keyword_len ) == 0 )
					module[token_len-pterm->keyword_len] = '\0';
		}else
			destroy_string(&module);
	}else if( !get_flags( config->current_flags, CF_PUBLIC_OPTION ) && 
			  !get_flags(pterm->flags, TF_NO_MYNAME_PREPENDING) ) 
	{
		module = config->myname;
	}
	if( ptr )
	{	
		if ( get_flags(pterm->flags, TF_NAMED) )
		{
			ptr = parse_token_strip_quotes(ptr, &name);
		}else if (get_flags(pterm->flags, TF_DIRECTION_INDEXED))
		{
			ptr = parse_token_strip_quotes (ptr, &side);
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
	}		
	if( name )
		parm_len  = sizeof(" " ASXMLOpt_nameAttrName  "=") + 1+strlen(name)+1+1;
	if( side )
		parm_len += sizeof(" " ASXMLOpt_sideAttrName  "=") + 1+strlen(side)+1+1;
	if( index )
		parm_len += sizeof(" " ASXMLOpt_indexAttrName "=") +1+strlen(index)+1+1;
	if( module )
		parm_len += sizeof(" " ASXMLOpt_moduleAttrName "=") +1+strlen(module)+1+1;

	if( parm_len > 0 ) 
	{
		char *parm_ptr = safemalloc( parm_len );
		item->parm = parm_ptr ; 
		if( name ) 
		{	
			sprintf( parm_ptr, ASXMLOpt_nameAttrName "=\"%s\"", name );	   
			while( *parm_ptr ) ++parm_ptr ;
			destroy_string( &name );
		}else if( side ) 
		{	  
			sprintf( parm_ptr,ASXMLOpt_sideAttrName "=\"%s\"", side );	   
			while( *parm_ptr ) ++parm_ptr ;
			destroy_string( &side ); 
		}else if( index ) 
		{	
			sprintf( parm_ptr, ASXMLOpt_indexAttrName "=\"%s\"", index );	   
			while( *parm_ptr ) ++parm_ptr ;
			destroy_string( &index );
		}	 
		if( module ) 
		{	
			sprintf( parm_ptr, (parm_ptr>item->parm)?" %s=\"%s\"":"%s=\"%s\"", ASXMLOpt_moduleAttrName, module );	   
			if( module != config->myname )
				destroy_string( &module );
		}	 
		LOCAL_DEBUG_OUT( "parm = \"%s\"", item->parm );
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
	if( ptr && ptr[0] ) 
	{                          /* add value as CDATA */
		if( pterm->sub_syntax == NULL )
		{	
			char *val = mystrdup(ptr);
			append_asconfig_value_tag( item, -1, &val );
		}else		
		{	
			int max_tokens = GetTermUseTokensCount(pterm);
			while( max_tokens > 0 && ptr[0] ) 
			{
				char *token = NULL;
				ptr = parse_token( ptr, &token );
		 			  
				append_asconfig_value_tag( item, pterm->type, &token );		   
				--max_tokens;
			}
		}
	}
	/* otherwise value will be present in child tags */
    print_trimmed_str( "config->current_data", config->current_data );
    //LOCAL_DEBUG_OUT( "curr_data_len = %d", config->current_data_len);

	if( comments ) 
		append_asconfig_value_tag( item, TT_INLINE_COMMENT, &comments );		
//	args2FreeStorage (pNext, config->current_data, config->current_data_len);
	if (pterm->sub_syntax)
	{	
		tail = &(item->child);
		while( *tail ) tail = &((*tail)->next);
		ProcessSubSyntax (config, tail, pterm->sub_syntax);
	}
	return;
}

Bool 
xml_tree2file(FILE *fp , char *myname, SyntaxDef *syntax, xml_elem_t* tree, int level )
{
	Bool do_cleanup_syntax = False ; 
	Bool last_unterminated = False ;

	if( fp == NULL || syntax == NULL || tree == NULL ) 
		return False;		   

	if( myname == NULL )
		myname = MyName ;

	if( level == 0 ) 
	{	   
		if( tree->tag_id == XML_CONTAINER_ID ) 
			if( (tree = tree->child) == NULL ) 
				return False;
	
		if( mystrcmp(tree->tag, syntax->display_name) != 0 ) 
		{	
			if( tree->tag_id == 0 )
				return False;
		}else
		{
			if( (tree = tree->child) == NULL ) 
				return False;	  
		}
	}

	for( ; ; tree = tree->next )
	{
		char sub_terminator = ' ' ;
		ASHashData hdata = {0};
		TermDef *pterm = NULL ;
		int i;
		
		if( tree == NULL )     /* need to check for syntax terminator */
		{
			if( syntax->terminator == '\n' && level > 0 )
			{	
				for( i = 0 ; syntax->terms[i].keyword != NULL ; ++i ) 
					if( get_flags( syntax->terms[i].flags, TF_SYNTAX_TERMINATOR ) )
					{
						pterm = &(syntax->terms[i]) ;
						break;
					}		
				if( pterm && last_unterminated ) 
					fputc( '\n', fp );
			}
		}else if( tree->tag_id == TT_ANY ) 
		{
			
		}else if( tree->tag_id == TT_COMMENT ) 
		{
			if( IsCDATA(tree->child) ) 
				fprintf( fp, "#%s\n", tree->child->parm );
			else
				fputs( "#\n", fp );
			continue;
		}else if( tree->tag_id == TT_INLINE_COMMENT ) 
		{
			if( IsCDATA(tree->child) ) 
				fprintf( fp, "#%s", tree->child->parm );
		}else
		{		 
			if( tree->tag_id )
			{	
				for( i = 0 ; syntax->terms[i].keyword != NULL ; ++i ) 
					if( syntax->terms[i].id == tree->tag_id ) 
					{
						pterm = &(syntax->terms[i]) ;
						break;
					}
			}
			if( pterm == NULL )
			{
				if( syntax->term_hash == NULL ) 
				{	
					PrepareSyntax (syntax);
					do_cleanup_syntax = True ;
				}
				if( get_hash_item (syntax->term_hash, AS_HASHABLE(tree->tag), &hdata.vptr)==ASH_Success  )
					pterm = hdata.vptr;
			}
			if( pterm == NULL ) 
				continue;
		}
		if( pterm ) 
		{		 
			xml_elem_t *child = tree?tree->child:NULL; 
			char *tag_myname = NULL ; 
			char *prefix = NULL ; 
			xml_elem_t *parm_tree = NULL ; 
			
			if( syntax->terminator == '\n' || syntax->terminator == '\0' ) 
				fprintf (fp, "%*s", level*4, " ");

			if( tree && tree->parm ) 
			{
				xml_elem_t *c; 
				parm_tree = xml_parse_parm(tree->parm, NULL);
				for( c = parm_tree ; c ; c = c->next )
				{	
					if( strcmp( c->tag, ASXMLOpt_moduleAttrName ) == 0 )
						tag_myname = c->parm ; 	
					else if( strcmp( c->tag, ASXMLOpt_indexAttrName ) == 0 || 
							 strcmp( c->tag, ASXMLOpt_sideAttrName ) == 0  ||
							 strcmp( c->tag, ASXMLOpt_nameAttrName ) == 0 )
						prefix = c->parm ; 	
				}
			}	 
			if( tag_myname == NULL ) 
				tag_myname = myname ;


			if( !get_flags( pterm->flags, TF_NO_MYNAME_PREPENDING )  )
			{	
				fprintf (fp, "*%s", tag_myname);
				if( pterm->keyword[0] == '~' )
					fputc( ' ', fp );
			}
			fprintf (fp, "%s", pterm->keyword);

			if( prefix ) 
			{
				if( get_flags( pterm->flags, TF_NAMED))
					fprintf( fp, " \"%s\"", prefix );
				else if( get_flags( pterm->flags, TF_INDEXED|TF_DIRECTION_INDEXED) )				
					fprintf( fp, " %s", prefix );
			}	 
			
			if( pterm->sub_syntax ) 
			{
				int token_count = GetTermUseTokensCount(pterm);
				
				for (i = 0; i < token_count && child ; i++, child = child->next )
				{	
					if( IsCDATA(child) ) 
						fprintf( fp, " %s", child->parm );
					else if( IsCDATA(child->child) ) 
						fprintf( fp, " %s", child->child->parm );
				}	 
				if( token_count == 0 && child )
					fputc( ' ', fp );
				if( child ) 
				{	
					if( pterm->sub_syntax->terminator == '\n' ) 
						fputc( '\n', fp );		
					xml_tree2file(fp , tag_myname, pterm->sub_syntax, child, level+1 );
					sub_terminator = pterm->sub_syntax->file_terminator ;
					if( sub_terminator == '\0' ) 
						sub_terminator = '\n' ;
				}
			}else if( IsCDATA(child) ) 
			{
				fprintf( fp, " %s", child->parm );
			}		 
		}
		if( tree ) 
		{	
			if( syntax->terminator != '\0' &&
				(sub_terminator == ' ' || sub_terminator != syntax->terminator ) ) 
			{
				last_unterminated = (tree->next == NULL );
				if( !last_unterminated )
					fputc( syntax->terminator, fp );
			}
		}else
		{
			if( pterm && syntax->terminator != '\0' ) 
				fputc( syntax->terminator, fp );
			break;
		}
	}

	if( syntax->file_terminator != '\0' ) 
		fputc( syntax->file_terminator, fp );
	else
		fputc( '\n', fp );

	if( do_cleanup_syntax ) 
		FreeSyntaxHash (syntax);

	return True;
}

xml_elem_t*
file2xml_tree(const char *filename, char *myname, SyntaxDef *syntax )
{
	ConfigData cd ;
	ConfigDef    *config_reader ;
	xml_elem_t *tree = NULL ;
	
	cd.filename = filename ;
	config_reader = InitConfigReader (myname, syntax, CDT_Filename, cd, NULL);
	if ( config_reader != NULL )
	{
		ASTreeStorageModel *model = NULL ;
		config_reader->statement_handler = statement2xml_elem;
		tree = create_CONTAINER_tag();
		tree->child = xml_elem_new(); 
		tree->child->tag = mystrdup(syntax->display_name);
		//c_tag->tag_id = pterm->id ;
		config2tree_storage(config_reader, &model);	   
		tree->child->child = (xml_elem_t*)model ;
		DestroyConfig (config_reader);   
	}
	return tree;
}	 

/**************************************************************************
 * This stuff below is for manipulating xml_elem_t tree of options : 
 *************************************************************************/

static void 
parse_curr_opt_attributes( ASXmlOptionTreeContext *context )
{
	xml_elem_t *parms = xml_parse_parm( context->current->parm, NULL );
	xml_elem_t *c;
	context->current_id[0] = '\0' ;
	context->current_flags = ASXmlOptTree_Public ;
	for( c = parms ; c != NULL ; c = c->next )
	{
		if( mystrcasecmp( c->tag, ASXMLOpt_moduleAttrName ) == 0 )
		{	
			clear_flags(context->current_flags, ASXmlOptTree_Public ) ;	  
			if( context->module[0] == '\0' || strcmp(c->parm, context->module ) == 0 ) 
				clear_flags(context->current_flags, ASXmlOptTree_Foreign ) ;
			else
				set_flags(context->current_flags, ASXmlOptTree_Foreign ) ;
		}else if( mystrcasecmp( c->tag, ASXMLOpt_indexAttrName ) == 0 )
		{	
			strncpy( context->current_id, c->parm, MAX_XMLATRR_LENGTH ); 
			context->current_id_type = ASXmlIDT_Index ;
			context->current_id[MAX_XMLATRR_LENGTH] = '\0' ;
		}else if( mystrcasecmp( c->tag, ASXMLOpt_sideAttrName ) == 0 )
		{	
			strncpy( context->current_id, c->parm, MAX_XMLATRR_LENGTH ); 
			context->current_id_type = ASXmlIDT_Side ;
			context->current_id[MAX_XMLATRR_LENGTH] = '\0' ;
		}else if( mystrcasecmp( c->tag, ASXMLOpt_nameAttrName ) == 0 )
		{	
			strncpy( context->current_id, c->parm, MAX_XMLATRR_LENGTH ); 
			context->current_id_type = ASXmlIDT_Name ;
			context->current_id[MAX_XMLATRR_LENGTH] = '\0' ;
		}
	}	 
}	 

static void
find_next_valid_opt( ASXmlOptionTreeContext *context )
{
	while( context->current != NULL )
	{
		parse_curr_opt_attributes( context );
		if( ( get_flags( context->flags, ASXmlOptTree_ExcludeForeign ) && 
			  get_flags( context->current_flags, ASXmlOptTree_Foreign )) ||
			( get_flags( context->flags, ASXmlOptTree_ExcludePublic ) && 
			  get_flags( context->current_flags, ASXmlOptTree_Public )))
		{
			context->current = context->current->next ;
		}else
			break;	 
	}
}


ASXmlOptionTreeContext *
xml_opt_tree_start_traversing( SyntaxDef  *syntax, xml_elem_t **pcontainer, xml_elem_t *subtree, const char *module )
{
/* if pcontainer is not NULL, then context takes over management of the memory of *pcontainer, 
 * and will deallocate it upon its destruction */
	
	ASXmlOptionTreeContext *context = NULL ; 
	xml_elem_t *root ;
	
	if( pcontainer == NULL && subtree == NULL ) 
		return NULL;

	root = subtree ;
	if( root == NULL )
	{	
		root = *pcontainer ;
		if( root && root->tag_id == XML_CONTAINER_ID )
			root = root->child ;
	}

	if( root == NULL || syntax == NULL ) 
		return NULL;

	context = safecalloc( 1, sizeof(ASXmlOptionTreeContext));
	context->syntax = syntax ; 
	context->container = *pcontainer ; 
	context->root = root ; 
	*pcontainer = NULL ;

	context->module = mystrdup(module);
	context->current = context->root ; 	
	
	find_next_valid_opt( context );
	return context;
}	  

ASXmlOptionTreeContext *
file2xml_tree_context( const char *filename, char *myname, SyntaxDef *syntax )
{
	xml_elem_t *root = file2xml_tree(filename, myname, syntax );
	return xml_opt_tree_start_traversing( syntax, &root, NULL, myname );
}



void
destroy_xml_opt_tree_context( ASXmlOptionTreeContext **pcontext )
{
	if( pcontext ) 
	{
		ASXmlOptionTreeContext *context	= *pcontext ;
		if( context ) 
		{
			if( context->module ) 
				destroy_string( &(context->module) );
			context->syntax = NULL ; 
			context->root = NULL ; 
			if( context->container ) 
				xml_elem_delete(NULL, context->container);
		}	 
	}	 
}

Bool
xml_opt_tree_go_first( ASXmlOptionTreeContext *context )
{
	if( context == NULL  ) 
		return False;
	
	context->current = context->root ; 	   
	find_next_valid_opt( context );

	return (context->current != NULL);   
}	  

Bool
xml_opt_tree_go_next( ASXmlOptionTreeContext *context )
{
	if( context == NULL || context->current == NULL ) 
		return False;
	
	context->current = context->current->next ; 	   
	find_next_valid_opt( context );

	return (context->current != NULL);   
}	  

ASXmlOptionTreeContext *
xml_opt_tree_traverse_subtree( ASXmlOptionTreeContext *context )
{
	ASXmlOptionTreeContext *subtree_context = NULL ; 

	if( context ) 
	{
		
		
	}	 
	
	return subtree_context;
}	  

Bool
xml_opt_tree_delete_current( ASXmlOptionTreeContext *context )
{
	if( context == NULL || context->current == NULL ) 
		return False;
	
	return True;
}	  

Bool
xml_opt_tree_delete_subtree( ASXmlOptionTreeContext *context )
{
	if( context == NULL || context->current == NULL ) 
		return False;
	
	return True;
}	  

Bool
xml_opt_tree_insert( ASXmlOptionTreeContext *context, int term_id, Bool module_specific, ASXmlOptIDType id_type, const char *id, const char *value)
{
	if( context == NULL ) 
		return False;
	
	return True;
}	  


Bool
xml_opt_tree_copy( ASXmlOptionTreeContext *context, Bool module_specific, ASXmlOptIDType id_type, const char *id, const char *value )
{
	if( context == NULL || context->current == NULL ) 
		return False;
	
	return True;
}	  


#ifdef TEST_PARSER_XML

#include "../libAfterConf/afterconf.h"

# define CONFIG_FILE	"~/.afterstep/wharf"
# define CONFIG_SYNTAX	&WharfSyntax
# define CONFIG_SPECIAL	NULL   /*WharfSpecialFunc*/
# define CONFIG_MYNAME  "Wharf"

int 
main( int argc, char ** argv ) 
{
	char *fullfilename;
	xml_elem_t* tree ;
	InitMyApp ("TestParserXML", argc, argv, NULL, NULL, 0 );
	START_TIME(started);
	
	LinkAfterStepConfig();
	InitSession();
	
	fullfilename = PutHome( CONFIG_FILE );

	tree = file2xml_tree(fullfilename, CONFIG_MYNAME, CONFIG_SYNTAX );
	
	xml_print(tree);	   

	SHOW_TIME("before xml_tree2file result ",started);
	show_progress( "xml_tree2file result: \n#############################");
	xml_tree2file(stderr, CONFIG_MYNAME, CONFIG_SYNTAX, tree, 0 );
	show_progress( "END xml_tree2file result: \n#############################");
	SHOW_TIME("after xml_tree2file result " ,started);

	xml_elem_delete(NULL, tree);
	SHOW_TIME(__FUNCTION__,started);

	FreeMyAppResources();
#   ifdef DEBUG_ALLOCS
	print_unfreed_mem ();
#   endif /* DEBUG_ALLOCS */
	return 1;
}
#endif

