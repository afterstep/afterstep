/*
 * Copyright (c) 2004 Sasha Vasko <sasha@aftercode.net>
 *
 * This program is free software; you can redistribute it and/or modify
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#define LOCAL_DEBUG
#define EVENT_TRACE

#include "../../configure.h" 
#include "../../libAfterStep/asapp.h"
#include <unistd.h>
#include "../../libAfterStep/screen.h"
#include "../../libAfterStep/module.h"
#include "../../libAfterStep/parser.h"
#include "../../libAfterStep/session.h"
#include "../../libAfterConf/afterconf.h"

#include "ASConfig.h"
#include "configfile.h"

/*************************************************************************/

ASConfigFileInfo ConfigFilesInfo[] = 
{
{CONFIG_BaseFile_ID, 		NULL, 0, NULL },
{CONFIG_ColorSchemeFile_ID, NULL, 0, NULL },
{CONFIG_LookFile_ID, 		NULL, 0, NULL },
{CONFIG_FeelFile_ID, 		NULL, 0, NULL },
{CONFIG_AutoExecFile_ID,	NULL, 0, NULL },
{CONFIG_StartDir_ID,		NULL, 0, NULL },			
{CONFIG_LookFile_ID,		NULL, 0, NULL },				
{CONFIG_DatabaseFile_ID,	NULL, 0, NULL },			   
{CONFIG_AfterStepFile_ID,   NULL, 0, NULL },		
{CONFIG_PagerFile_ID, 		NULL, 0, NULL },				
{CONFIG_WharfFile_ID, 		NULL, 0, NULL },				
{CONFIG_WinListFile_ID,		NULL, 0, NULL },
{0,NULL, 0, NULL}
};

/*************************************************************************/
void
init_ConfigFileInfo()
{
	int i = 0;
	while( ConfigFilesInfo[i].config_file_id != 0 ) 
	{
		switch( ConfigFilesInfo[i].config_file_id )
		{

			case CONFIG_BaseFile_ID :	
				ConfigFilesInfo[i].session_file	= make_session_file(Session, BASE_FILE, False ); 
				break ;	   
			case CONFIG_ColorSchemeFile_ID :	
				ConfigFilesInfo[i].session_file = (char*)get_session_file (Session, 0, F_CHANGE_COLORSCHEME, False); 
				ConfigFilesInfo[i].non_freeable = True ;
				break ;	  
			case CONFIG_LookFile_ID :	   
				ConfigFilesInfo[i].session_file = (char*)get_session_file (Session, 0, F_CHANGE_LOOK, False);
				ConfigFilesInfo[i].non_freeable = True ;
				break ;	  
			case CONFIG_FeelFile_ID :	   
				ConfigFilesInfo[i].session_file = (char*)get_session_file (Session, 0, F_CHANGE_FEEL, False);
				ConfigFilesInfo[i].non_freeable = True ;
				break ;	
			case CONFIG_AutoExecFile_ID :	   
				ConfigFilesInfo[i].session_file = make_session_file(Session, AUTOEXEC_FILE, False );
				break ;	
			case CONFIG_StartDir_ID :	   
				ConfigFilesInfo[i].session_file = make_session_dir (Session, START_DIR, False); 
				break ;	
			case CONFIG_DatabaseFile_ID :	   
				ConfigFilesInfo[i].session_file = make_session_file(Session, DATABASE_FILE, False ); 
				break ;	   
			case CONFIG_AfterStepFile_ID :	   
				ConfigFilesInfo[i].session_file = make_session_file(Session, "afterstep", False );;	
				break ;
			case CONFIG_PagerFile_ID :	   
				ConfigFilesInfo[i].session_file = make_session_file(Session, "pager", False ); 
				break ;	 
			case CONFIG_WharfFile_ID :	   
				ConfigFilesInfo[i].session_file = make_session_file(Session, "wharf", False ); 
				break ;	 
			case CONFIG_WinListFile_ID :	   
				ConfigFilesInfo[i].session_file = make_session_file(Session, "winlist", False ); 
				break ;	 
		}	 
		++i ;	
	}	 
	
}	  

const char*
get_config_file_name( int config_id ) 
{
	int i = 0;
	while( ConfigFilesInfo[i].config_file_id != 0 ) 
	{
		if( ConfigFilesInfo[i].config_file_id == config_id )
		{	
			if( ConfigFilesInfo[i].tmp_override_file != NULL )
				return ConfigFilesInfo[i].tmp_override_file;
			else
				return ConfigFilesInfo[i].session_file;
		}
		++i ;	  
	}		
	return NULL;
}	 

/*************************************************************************/
/**************************************************************************/
ASConfigFile *
load_config_file(const char *dirname, const char *filename, const char *myname, SyntaxDef *syntax, SpecialFunc special)
{
	ASConfigFile * ascf ;
		
	if( (dirname == NULL && filename == NULL) || syntax == NULL ) 
		return NULL;

	ascf = safecalloc( 1, sizeof(ASConfigFile));
	if( dirname == NULL ) 
		parse_file_name(filename, &(ascf->dirname), &(ascf->filename));
	else
	{
		ascf->dirname = mystrdup( dirname );
		ascf->filename = mystrdup( filename );
	}	 

	ascf->fullname = dirname?make_file_name( dirname, filename ): mystrdup(filename) ;
	ascf->writeable = (check_file_mode (ascf->fullname, W_OK) == 0);
	ascf->myname = mystrdup(myname);

	ascf->free_storage = file2free_storage(ascf->fullname, ascf->myname, syntax, special, NULL );
	ascf->syntax = syntax ;

	return ascf;
}	 

void destroy_config_file( ASConfigFile *ascf ) 
{
	if( ascf )
	{	
		if( ascf->dirname ) 
			free( ascf->dirname );
		if( ascf->filename ) 
			free( ascf->filename );
		if( ascf->fullname ) 
			free( ascf->fullname );
		if( ascf->myname ) 
			free( ascf->myname );
		if( ascf->free_storage ) 
			DestroyFreeStorage( &(ascf->free_storage) );
	}	
}	 

ASConfigFile *
dup_config_file( ASConfigFile *src )
{
	ASConfigFile *dst = NULL ;
	if( src != NULL	)
	{
		dst = safecalloc( 1, sizeof(ASConfigFile));
		*dst = *src ;
		if( src->dirname ) 
			dst->dirname = mystrdup(src->dirname);
		if( src->filename ) 
			dst->filename = mystrdup( src->filename );
		if( src->fullname ) 
			dst->fullname = mystrdup( src->fullname );
		if( src->myname ) 
			dst->myname = mystrdup( src->myname );
		if( src->free_storage ) 
		{
			dst->free_storage = NULL ;
			CopyFreeStorage (&(dst->free_storage), src->free_storage);
		}
	}	 

	return dst;
	
}	   

/*************************************************************************/

