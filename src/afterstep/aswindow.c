/*
 * Copyright (c) 2000 Sasha Vasko <sashav@sprintmail.com>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#define LOCAL_DEBUG

#include "../../configure.h"

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

#include "asinternals.h"
#include "../../libAfterStep/session.h"
#include "../../libAfterStep/wmprops.h"

Bool on_dead_aswindow( Window w );
/********************************************************************************/
/* window list management */

void auto_destroy_aswindow ( void *data )
{
    if( data && ((ASMagic*)data)->magic == MAGIC_ASWINDOW )
    {
        ASWindow *asw = (ASWindow*)data;
        Destroy( asw, False );
    }
}

void destroy_aslayer  (ASHashableValue value, void *data);

ASWindowList *
init_aswindow_list()
{
    ASWindowList *list ;

    list = safecalloc( 1, sizeof(ASWindowList) );
    list->clients = create_asbidirlist(auto_destroy_aswindow);
    list->aswindow_xref = create_ashash( 0, NULL, NULL, NULL );
    list->layers = create_ashash( 7, NULL, desc_long_compare_func, destroy_aslayer );
	list->bookmarks = create_ashash( 7, string_hash_value, string_compare, string_destroy_without_data );

    list->circulate_list = create_asvector( sizeof(ASWindow*) );
    list->sticky_list = create_asvector( sizeof(ASWindow*) );

    Scr.on_dead_window = on_dead_aswindow ;

    return list;
}

struct SaveWindowAuxData
{
    char  this_host[MAXHOSTNAME];
    FILE *f;
	ASHashTable *res_name_counts;
	Bool only_modules ;
};

int
get_res_name_count( ASHashTable *res_name_counts, char *res_name )
{
	ASHashData hdata;

	if( res_name == NULL || res_name_counts == NULL )
		return 0;
	if( get_hash_item( res_name_counts, AS_HASHABLE(res_name), &hdata.vptr ) == ASH_Success )
	{
		int val = *(hdata.iptr) ;
		++val ;
		*(hdata.iptr) = val ;
		return val;
	}else
	{
		hdata.iptr = safemalloc( sizeof(int) );
		*(hdata.iptr) = 1 ;
		add_hash_item( res_name_counts, AS_HASHABLE(mystrdup(res_name)), hdata.vptr );
	}
	return 1;
}

Bool
check_aswindow_name_unique( char *name, ASWindow *asw )
{
    ASBiDirElem *e = LIST_START(Scr.Windows->clients) ;
    while( e != NULL )
    {
        ASWindow *curr = (ASWindow*)LISTELEM_DATA(e);
        if( curr != asw && strcmp(ASWIN_NAME(curr), name) == 0 )
			return False;
        LIST_GOTO_NEXT(e);
    }
	return True;
}

// ask for program command line
char *pid2cmd ( int pid )
{
#define MAX_CMDLINE_SIZE 2048
   	FILE *f;
   	static char buf[MAX_CMDLINE_SIZE];
	static char path[128];
	char *cmd = NULL ;

	sprintf (path, "/proc/%d/cmdline", pid);
   	if ((f = fopen (path, "r")) !=NULL )
	{	
		if( fgets (buf, MAX_CMDLINE_SIZE, f) != NULL )
		{	
			buf[MAX_CMDLINE_SIZE-1] = '\0' ;
			cmd = mystrdup( &buf[0] );
		}		
	    fclose (f);
	}
	return cmd;
}

static 
char *filter_out_geometry( char *cmd_args, char **geom_keyword, char **original_size )
{
	char *clean = mystrdup(cmd_args); 
	char *token_start ; 
	char *token_end = clean;
	
	do
	{
		token_start = token_end ;
  
		if( token_start[0] == '-' ) 
		{	
			if( strncmp( token_start, "-g ", 3 ) == 0 ||
				strncmp( token_start, "-geometry ", 10 ) == 0 ||
				strncmp( token_start, "--geometry ", 11 ) == 0 )
			{
				unsigned int width = 0 ;
  				unsigned int height = 0 ;
				int flags = 0 ;
				char *geom_start = token_start ; 
				if( geom_keyword ) 
					*geom_keyword = tokencpy (geom_start);
		 		token_end = tokenskip( token_start, 1 );
				if( token_end > token_start ) 
				{	
					token_start = token_end ;
					token_end = parse_geometry(token_start, NULL, NULL, &width, &height, &flags ); 
					if( token_end > token_start ) 
					{
						int i = 0; 
						for( i = 0 ; token_end[i] != '\0' ; ++i)
							geom_start[i] = token_end[i] ;
						geom_start[i]	= '\0' ;
					}
				}
				if( original_size && get_flags(flags, WidthValue|HeightValue) )
				{
					char *tmp = *original_size = safemalloc(64);
					if( get_flags(flags, WidthValue) && width > 0 )
					{	
						sprintf( tmp, "%d", width );
						while( *tmp ) ++tmp;
					}
					if( get_flags(flags, HeightValue) && height > 0 )
						sprintf( tmp, "x%d", height );
				}	 
				break;
			}
		}
		token_end = tokenskip( token_start, 1 );
	}while( token_end > token_start );
	return clean;
}	 

static void 
stripreplace_geometry_size( char **pgeom, char *original_size )
{
	char *geom = *pgeom ;
	if( geom ) 
	{
		
		int i = 0 ; 
		if( isdigit(geom[0]) )
			while( geom[++i] ) if( geom[i] == '+' || geom[i] == '-' ) break;
		if( original_size != NULL || i > 0 )
		{
			int size = strlen( &geom[i] );
			if( original_size ) 
				size += strlen(original_size);
			*pgeom = safemalloc( size + 1 );
			if( original_size ) 
				sprintf( *pgeom, "%s%s", original_size, &geom[i] );
			else
				strcpy( *pgeom, &geom[i] );
			free( geom );		 	  
		}	 
	}		  
}

static char *
make_application_name( ASWindow *asw, char *rclass, char *rname )
{
	char *name = ASWIN_NAME(asw);
	/* need to check if we can use window name in the pattern. It has to be :
		* 1) Not changed since window initial mapping
		* 2) all ASCII
		* 3) shorter then 80 chars
		* 4) must not match class or res_name
		* 5) Unique
		*/
	if( name == rname || name == rclass ) 
		name = NULL ;
	else if( name != NULL && get_flags( asw->internal_flags, ASWF_NameChanged ) ) 
	{/* allow changed names only for terms, as those could launch sub app inside */
		int rclass_len = rclass?strlen(rclass):0;
		if( rclass_len != 5 )
			name = NULL ;
		else if( strstr( rclass+1, "term" ) != 0 ) 	  
			name = NULL ;
	}
	if(	name )
	{
		int i  = 0;
		while( name[i] != '\0' )
		{   /* we do not want to have path in names as well */
			if( !isalnum(name[i]) && !isspace(name[i]))
				break;
			if( ++i >= 80 )
				break;
		}
		if( name[i] != '\0' )
			name = NULL ;
		else
		{
			if( strcmp( rclass, name ) == 0 ||
				strcmp( rname, name ) == 0 	)
				name = NULL ;
			else                   /* check that its unique */
				if( !check_aswindow_name_unique( name, asw ) )
					name = NULL ;
		}
	}
	return name;
}	  

Bool
make_aswindow_cmd_iter_func(void *data, void *aux_data)
{
    struct SaveWindowAuxData *swad = (struct SaveWindowAuxData *)aux_data ;
    ASWindow *asw = (ASWindow*)data ;
	LOCAL_DEBUG_OUT( "window \"%s\", is a %smodule", ASWIN_NAME(asw), ASWIN_HFLAGS(asw,AS_Module)?
	" ":"non ");
    if( asw && swad )
    {
		Bool same_host = (asw->hints->client_host == NULL || mystrcasecmp( asw->hints->client_host, swad->this_host )== 0);

		/* don't want to save modules - those are started from autoexec anyways */
		if( ASWIN_HFLAGS(asw,AS_Module) ) 
		{	
			if( !swad->only_modules	)
				return True;
		}else if( swad->only_modules	)
			return True;

		if( asw->hints->client_cmd == NULL && same_host )
		{
		 	if( ASWIN_HFLAGS(asw, AS_PID) && asw->hints->pid > 0 )
				asw->hints->client_cmd = pid2cmd( asw->hints->pid );
			
		}
		LOCAL_DEBUG_OUT( "same_host = %d, client_smd = \"%s\"", same_host,
		asw->hints->client_cmd?asw->hints->client_cmd:"(null)" );

        if( asw->hints->client_cmd != NULL && same_host )
        {
			char *pure_geometry = NULL ;
			char *geom = make_client_geometry_string( ASDefaultScr, asw->hints, asw->status, &(asw->anchor), Scr.Vx, Scr.Vy, &pure_geometry );
			/* format :   [<res_class>]:[<res_name>]:[[#<seq_no>]|<name>]  */
			int app_no = get_res_name_count( swad->res_name_counts, asw->hints->res_name );
			char *rname = asw->hints->res_name?asw->hints->res_name:"*" ;
			char *rclass = asw->hints->res_class?asw->hints->res_class:"*" ;
			char *name = ASWIN_NAME(asw);
			char *app_name = "*" ;
			char *cmd_app = NULL, *cmd_args ;
			Bool supports_geometry = False ;
			char *geometry_keyword = NULL ; 
			char *clean_cmd_args = NULL ;
			char *original_size = NULL ;

			name = make_application_name( asw, rclass, rname );
			
			if( name == NULL )
			{
				app_name = safemalloc( strlen(rclass)+1+strlen(rname)+1+1+15+1 );
				sprintf( app_name, "%s:%s:#%d", rclass, rname, app_no );
			}else
			{
				app_name = safemalloc( strlen(rclass)+1+strlen(rname)+1+strlen(name)+1 );
				sprintf( app_name, "%s:%s:%s", rclass, rname, name );
			}
				
			cmd_args = parse_token(asw->hints->client_cmd, &cmd_app );
			if( cmd_app )  /* we want -geometry to be the first arg, so that terms could correctly launch app with -e arg */
				clean_cmd_args = filter_out_geometry( cmd_args, &geometry_keyword, &original_size ) ;
			
			if( geometry_keyword == NULL ) 
				geometry_keyword = mystrdup(ASWIN_HFLAGS(asw,AS_Module)?"--geometry":"-geometry"); 
			else
				supports_geometry = True ;
			
			if( !ASWIN_HFLAGS(asw,AS_Handles) )
			{   /* we want to remove size from geometry here, 
				 * unless it was requested in original cmd-line geometry */
				stripreplace_geometry_size( &geom, original_size );
			}	 
			
			if( cmd_app ) 
			{   
				fprintf( swad->f, 	"\tExec \"I:%s\" %s %s %s %s &\n", app_name, cmd_app, geometry_keyword, geom, clean_cmd_args );
				destroy_string(&cmd_app);	
			}else
	            fprintf( swad->f, 	"\tExec \"I:%s\" %s %s %s &\n", app_name, asw->hints->client_cmd, geometry_keyword, geom );
			
			destroy_string(&clean_cmd_args);
			destroy_string(&geometry_keyword);
			destroy_string(&original_size);

			if( ASWIN_HFLAGS(asw,AS_Module) ) 
			{
				fprintf( swad->f, "\tWait \"I:%s\" Layer %d\n", app_name, ASWIN_LAYER(asw));
			}else if( ASWIN_GET_FLAGS(asw,AS_Sticky) )
			{
				if( supports_geometry ) 	  
					fprintf( swad->f, 	"\tWait \"I:%s\" Layer %d"
										", Sticky"
										", StartsOnDesk %d"
										", %s"
										"\n",
						     			app_name, ASWIN_LAYER(asw),
										ASWIN_DESK(asw),
										ASWIN_GET_FLAGS(asw,AS_Iconic)?"StartIconic":"StartNormal");
				else
					fprintf( swad->f, 	"\tWait \"I:%s\" DefaultGeometry %s"
							        	", Layer %d"
										", Sticky"
										", StartsOnDesk %d"
										", %s"
										"\n",
						     			app_name, pure_geometry,
							 			ASWIN_LAYER(asw),
										ASWIN_DESK(asw),
										ASWIN_GET_FLAGS(asw,AS_Iconic)?"StartIconic":"StartNormal");
			}else
			{
				if( supports_geometry ) 
					fprintf( swad->f, 	"\tWait \"I:%s\" Layer %d"
										", Slippery"
										", StartsOnDesk %d"
										", ViewportX %d"
										", ViewportY %d"
										", %s"
										"\n",
						     			app_name, ASWIN_LAYER(asw),
										ASWIN_DESK(asw),
										((asw->status->x + asw->status->viewport_x) / Scr.MyDisplayWidth)*Scr.MyDisplayWidth,
										((asw->status->y + asw->status->viewport_y) / Scr.MyDisplayHeight)*Scr.MyDisplayHeight,
										ASWIN_GET_FLAGS(asw,AS_Iconic)?"StartIconic":"StartNormal");
				else										   
					fprintf( swad->f, 	"\tWait \"I:%s\" DefaultGeometry %s"
							        	", Layer %d"
										", Slippery"
										", StartsOnDesk %d"
										", ViewportX %d"
										", ViewportY %d"
										", %s"
										"\n",
						     			app_name, pure_geometry,
							 			ASWIN_LAYER(asw),
										ASWIN_DESK(asw),
										((asw->status->x + asw->status->viewport_x) / Scr.MyDisplayWidth)*Scr.MyDisplayWidth,
										((asw->status->y + asw->status->viewport_y) / Scr.MyDisplayHeight)*Scr.MyDisplayHeight,
										ASWIN_GET_FLAGS(asw,AS_Iconic)?"StartIconic":"StartNormal");
			}	 
			destroy_string(&pure_geometry);
			destroy_string(&geom);
			destroy_string(&app_name);
        }
        return True;
    }
    return False;
}

void
save_aswindow_list( ASWindowList *list, char *file )
{
    struct SaveWindowAuxData swad ;

    if( list == NULL )
        return ;

    if (!mygethostname (swad.this_host, MAXHOSTNAME))
	{
        show_error ("Could not get HOST environment variable!");
		return;
	}

	if( file != NULL )
	{
	    char *realfilename = PutHome( file );
		if( realfilename == NULL )
        	return;

    	swad.f = fopen (realfilename, "w+");
	    if ( swad.f == NULL)
    	    show_error( "Unable to save your workspace state into the %s - cannot open file for writing!", realfilename);
    	free (realfilename);
	}else
	{
		swad.f = fopen (get_session_ws_file(Session,False), "w+");
	    if ( swad.f == NULL)
    	    show_error( "Unable to save your workspace state into the default file %s - cannot open file for writing!", get_session_ws_file(Session,False));
	}

	if( swad.f )
    {
		fprintf( swad.f, "Function \"WorkspaceState\"\n" );
		swad.only_modules = False ;
		swad.res_name_counts = create_ashash(0, string_hash_value, string_compare, string_destroy);
        iterate_asbidirlist( list->clients, make_aswindow_cmd_iter_func, &swad, NULL, False );
		destroy_ashash( &(swad.res_name_counts) );
		fprintf( swad.f, "EndFunction\n\n" );
		fprintf( swad.f, "Function \"WorkspaceModules\"\n" );
		swad.only_modules = True ;
		swad.res_name_counts = create_ashash(0, string_hash_value, string_compare, string_destroy);
        iterate_asbidirlist( list->clients, make_aswindow_cmd_iter_func, &swad, NULL, False );
		destroy_ashash( &(swad.res_name_counts) );
		fprintf( swad.f, "EndFunction\n" );
        fclose( swad.f );
    }
}

void
destroy_aswindow_list( ASWindowList **list, Bool restore_root )
{
    if( list )
        if( *list )
        {
            if( restore_root )
                InstallRootColormap ();

            destroy_asbidirlist( &((*list)->clients ));
            destroy_ashash(&((*list)->aswindow_xref));
            destroy_ashash(&((*list)->layers));
			destroy_ashash(&((*list)->bookmarks));
            destroy_asvector(&((*list)->sticky_list));
            destroy_asvector(&((*list)->circulate_list));
            free(*list);
            *list = NULL ;
        }
}

void
publish_aswindow_list( ASWindowList *list, Bool stacking_only )
{
	int clients_num = list->clients->count, used = 0 ;
	Window *client_list	;
	int i ;
    ASLayer **layers = NULL ;
    unsigned long layers_in ;
	
	if( clients_num == 0 ) 
		return;

	client_list = safecalloc( clients_num, sizeof(Window));
	if( !stacking_only ) 
	{
		ASBiDirElem *curr = LIST_START(list->clients);
		used = 0 ;
		while( curr && used < clients_num )
		{
			ASWindow *asw = (ASWindow*)LISTELEM_DATA(curr);
			client_list[used++] = asw->w ;
			LIST_GOTO_NEXT(curr);	
		}	 
		LOCAL_DEBUG_OUT( "Setting Client List property to include %d windows (clients_num = %d) ", used, clients_num );
		set_clients_list (Scr.wmprops, client_list, used);
	}		  
    
    layers = safecalloc( list->layers->items_num, sizeof(ASLayer*) );
	if( (layers_in = sort_hash_items (list->layers, NULL, (void**)layers, 0)) == 0 )
        return ;

    used = clients_num ;       /* we have to reverse the order - bottommost window comes first */
	for( i = 0 ; i < layers_in ; i++ )
    {
        register int k, end_k = PVECTOR_USED(layers[i]->members) ;
        ASWindow **members = PVECTOR_HEAD(ASWindow*,layers[i]->members);
        if( end_k > used )
            end_k = used ;
        for( k = 0 ; k < end_k ; k++ )
        {
            register ASWindow *asw = members[k] ;
            if( !ASWIN_GET_FLAGS(asw, AS_Dead) )
            	client_list[--used] = asw->w;
        }
    }
	set_stacking_order (Scr.wmprops, &client_list[used], clients_num - used);

	free( layers );
	free( client_list );
		   
}

/*************************************************************************
 * We maintain crossreference of X Window ID to ASWindow structure - that is
 * faster then using XContext since we don't have to worry about multiprocessing,
 * thus saving time on interprocess synchronization, that Xlib has to do in
 * order to access list of window contexts.
 *************************************************************************/
ASWindow *window2ASWindow( Window w )
{
	ASHashData hdata = {0} ;
    if( Scr.Windows->aswindow_xref )
        if( get_hash_item( Scr.Windows->aswindow_xref, AS_HASHABLE(w), &hdata.vptr ) != ASH_Success )
            hdata.vptr = NULL;
    return hdata.vptr;
}

Bool register_aswindow( Window w, ASWindow *asw )
{
    if( w && asw )
        return (add_hash_item( Scr.Windows->aswindow_xref, AS_HASHABLE(w), asw ) == ASH_Success );
    return False;
}

Bool unregister_aswindow( Window w )
{
    if( w )
        return (remove_hash_item( Scr.Windows->aswindow_xref, AS_HASHABLE(w), NULL, False ) == ASH_Success);
    return False;
}

Bool destroy_registered_window( Window w )
{
	Bool res = False ;
    if( w )
    {
        res = unregister_aswindow( w );
        XDestroyWindow( dpy, w );
    }
    return res;
}

Bool 
bookmark_aswindow( ASWindow *asw, char *bookmark )
{
	Bool success = False ;
	if( bookmark ) 
	{	
		remove_hash_item(  Scr.Windows->bookmarks, AS_HASHABLE(bookmark), NULL, False );
		LOCAL_DEBUG_OUT( "Bookmark \"%s\" cleared", bookmark );
    	if( asw )
		{
			ASHashData hd ;
			char *tmp = mystrdup(bookmark) ;
			hd.c32 = asw->w ;
			success = (add_hash_item( Scr.Windows->bookmarks, AS_HASHABLE(tmp), hd.vptr ) == ASH_Success );
			if( !success ) 
				free(tmp);
			LOCAL_DEBUG_OUT( "Added Bookmark for window %p, ID=%8.8lX, -> \"%s\"", asw, asw->w, bookmark );
		}
    }
	return success;
}


ASWindow *
bookmark2ASWindow( const char *bookmark )
{
	ASWindow *asw = NULL ;
	Bool success = False;
	ASHashData hd ;
	hd.c32 = None ;
	if( bookmark ) 
	{	
		if( get_hash_item( Scr.Windows->bookmarks, AS_HASHABLE(bookmark), &(hd.vptr) ) == ASH_Success )
		{	
			success = True ;
			asw = window2ASWindow( hd.c32 );
		}
#if defined(LOCAL_DEBUG) && !defined(NO_DEBUG_OUTPUT)
		print_ashash ( Scr.Windows->bookmarks, string_print);
#endif
	}
	LOCAL_DEBUG_OUT( "Window %p, ID=%8.8lX, %sfetched for bookmark \"%s\"", asw, hd.c32, success?"":"not ", bookmark );
	return asw;
}

ASWindow *
pattern2ASWindow( const char *pattern )
{
    ASWindow *asw = bookmark2ASWindow( pattern );
	if( asw == NULL ) 
	{	
		wild_reg_exp *wrexp = compile_wild_reg_exp( pattern );
		if( wrexp != NULL )
    	{
        	ASBiDirElem *e = LIST_START(Scr.Windows->clients) ;
        	while( e != NULL )
        	{
            	asw = (ASWindow*)LISTELEM_DATA(e);
            	if( match_string_list (asw->hints->names, MAX_WINDOW_NAMES, wrexp) == 0 )
					break;
				else
					asw = NULL ;
            	LIST_GOTO_NEXT(e);
        	}
    	}
    	destroy_wild_reg_exp( wrexp );
	}
    return asw;
}

char *parse_semicolon_token( char *src,  char *dst, int *len )
{
	int i = 0 ;
	while( *src != '\0' )
	{
		if( *src == ':' )
		{
			if( *(src+1) == ':' )
				++src ;
			else
				break;
		}
		dst[i] = *src ;
		++i ;
		++src ;
	}
	dst[i] = '\0' ;
	*len = i ;
	return (*src == ':')?src+1:src;
}

ASWindow *
complex_pattern2ASWindow( char *pattern )
{
	ASWindow *asw = NULL ;
	/* format :   [<res_class>]:[<res_name>]:[[#<seq_no>]|<name>]  */
	LOCAL_DEBUG_OUT( "looking for window matchng pattern \"%s\"", pattern );
	if( pattern && pattern[0] )
    {
		wild_reg_exp *res_class_wrexp = NULL ;
    	wild_reg_exp *res_name_wrexp = NULL ;
		int res_name_no = 1 ;
    	wild_reg_exp *name_wrexp = NULL ;
        ASBiDirElem *e = LIST_START(Scr.Windows->clients) ;
		char *ptr = pattern ;
		char *tmp = safemalloc( strlen(pattern)+1 );
		int tmp_len = 0;
		Bool matches_reqired = 0 ;

		ptr = parse_semicolon_token( ptr, tmp, &tmp_len );
		LOCAL_DEBUG_OUT( "res_class pattern = \"%s\"", tmp );
		if( tmp[0] )
		{
			res_class_wrexp = compile_wild_reg_exp_sized( tmp, tmp_len );
			++matches_reqired ;
			ptr = parse_semicolon_token( ptr, tmp, &tmp_len );
			LOCAL_DEBUG_OUT( "res_name pattern = \"%s\"", tmp );
			if( tmp[0] )
			{
				res_name_wrexp = compile_wild_reg_exp_sized( tmp, tmp_len );
				++matches_reqired ;
				ptr = parse_semicolon_token( ptr, tmp, &tmp_len );
				LOCAL_DEBUG_OUT( "final pattern = \"%s\"", tmp );
				if( tmp[0] == '#' && isdigit(tmp[1]) )
				{
					res_name_no = atoi( tmp+1 ) ;
					LOCAL_DEBUG_OUT( "res_name_no = %d", res_name_no );					
				}else if( tmp[0] )
				{	
					name_wrexp = compile_wild_reg_exp_sized( tmp, tmp_len );
					++matches_reqired ;
				}
			}else
			{
				res_name_wrexp = res_class_wrexp ;
				name_wrexp = res_class_wrexp;	
			}	 
		}
		free( tmp );

        for( ; e != NULL && (asw == NULL || res_name_no > 0 ) ; LIST_GOTO_NEXT(e) )
        {
            ASWindow *curr = (ASWindow*)LISTELEM_DATA(e);
			int matches = 0 ;
			LOCAL_DEBUG_OUT( "matching res_class \"%s\"", curr->hints->res_class );
			if( res_class_wrexp != NULL )
				if( match_wild_reg_exp( curr->hints->res_class, res_class_wrexp) == 0 )
					++matches;
			LOCAL_DEBUG_OUT( "matching res_name \"%s\"", curr->hints->res_name );
			if( res_name_wrexp != NULL )
				if( match_wild_reg_exp( curr->hints->res_name, res_name_wrexp) == 0 )
					++matches;
			LOCAL_DEBUG_OUT( "matching name \"%s\"", curr->hints->names[0] );
			if( name_wrexp != NULL )
				if( match_wild_reg_exp( curr->hints->names[0], name_wrexp) == 0 ||
					match_wild_reg_exp( curr->hints->icon_name, name_wrexp) == 0  )
					++matches;

			if( matches < matches_reqired ) 
				continue;
			asw = curr ;
			--res_name_no ;
			LOCAL_DEBUG_OUT( "matches = %d, res_name_no = %d, asw = %p", matches, res_name_no, asw  );
        }

		if( res_class_wrexp )
	    	destroy_wild_reg_exp( res_class_wrexp );
		if( res_name_wrexp != res_class_wrexp && res_name_wrexp )
	    	destroy_wild_reg_exp( res_name_wrexp );
		if( name_wrexp && name_wrexp != res_class_wrexp )
	    	destroy_wild_reg_exp( name_wrexp );
		if( res_name_no > 0 )
			asw = NULL ;                       /* not found with requested seq no */
    }
    return asw;
}


Bool on_dead_aswindow( Window w )
{
    ASWindow *asw = window2ASWindow( w );
    if( asw )
    {
        if( w == asw->w )
        {
            ASWIN_SET_FLAGS( asw, AS_Dead );
            show_progress( "marking client's window as destroyed for client \"%s\", window 0x%X", ASWIN_NAME(asw), w );
            return True;
        }
    }
    return False;
}

/*******************************************************************************/
/* layer management */

ASLayer *
get_aslayer( int layer, ASWindowList *list )
{
    ASLayer *l = NULL ;
    if( list && list->layers )
    {
        ASHashableValue hlayer = AS_HASHABLE(layer);
		ASHashData hdata = {0} ;
        if( get_hash_item( list->layers, hlayer, &hdata.vptr ) != ASH_Success )
        {
            l = safecalloc( 1, sizeof(ASLayer));
            if( add_hash_item( list->layers, hlayer, l ) == ASH_Success )
            {
                l->members = create_asvector( sizeof(ASWindow*) );
                l->layer = layer ;
                LOCAL_DEBUG_OUT( "added new layer %p(%d) to hash", l, layer );
            }else
            {
                free( l );
                LOCAL_DEBUG_OUT( "failed to add new layer %p(%d) to hash", l, layer );
                l = NULL ;
            }
        }else
			l = hdata.vptr ;
    }
    return l;
}

void
destroy_aslayer  (ASHashableValue value, void *data)
{
    if( data )
    {
        ASLayer *l = data ;
        LOCAL_DEBUG_OUT( "destroying layer %p(%d)", l, l->layer );
        destroy_asvector( &(l->members));
        free( data );
    }
}
/********************************************************************************/
/* ASWindow management */

void
tie_aswindow( ASWindow *t )
{
    if( t->hints->transient_for != None )
    {
        ASWindow *transient_owner  = window2ASWindow(t->hints->transient_for);
        if( transient_owner != NULL )
        {
            t->transient_owner = transient_owner ;
            if( transient_owner->transients == NULL )
                transient_owner->transients = create_asvector( sizeof( ASWindow* ) );
            vector_insert_elem( transient_owner->transients, &t, 1, NULL, True );
        }
    }
    if( t->hints->group_lead != None )
    {
        ASWindow *group_lead  = window2ASWindow( t->hints->group_lead );
        if( group_lead != NULL )
        {
            t->group_lead = group_lead ;
            if( group_lead->group_members == NULL )
                group_lead->group_members = create_asvector( sizeof( ASWindow* ) );
            vector_insert_elem( group_lead->group_members, &t, 1, NULL, True );
        }
    }
}

void
untie_aswindow( ASWindow *t )
{
    if( t->transient_owner != NULL && t->transient_owner->magic == MAGIC_ASWINDOW )
    {
        if( t->transient_owner != NULL )
            vector_remove_elem( t->transient_owner->transients, &t );
        t->transient_owner = NULL ;
    }
    if( t->group_lead && t->group_lead->magic == MAGIC_ASWINDOW )
    {
        if( t->group_lead->group_members )
            vector_remove_elem( t->group_lead->group_members, &t );
        t->group_lead = NULL ;
    }
}

void
add_aswindow_to_layer( ASWindow *asw, int layer )
{
    if( !AS_ASSERT(asw) )
    {
        ASLayer  *dst_layer = get_aslayer( layer, Scr.Windows );
        /* inserting window into the top of the new layer */
    LOCAL_DEBUG_OUT( "adding window %p to layer %p (%d)", asw, dst_layer, layer );
        if( !AS_ASSERT(dst_layer) )
            vector_insert_elem( dst_layer->members, &asw, 1, NULL, True );
    }
}

void
remove_aswindow_from_layer( ASWindow *asw, int layer )
{
    if( !AS_ASSERT(asw) )
    {
        ASLayer  *src_layer = get_aslayer( layer, Scr.Windows );
    LOCAL_DEBUG_OUT( "removing window %p from layer %p (%d)", asw, src_layer, layer );
        if( !AS_ASSERT(src_layer) )
	{
LOCAL_DEBUG_OUT( "can be found at index %d", vector_find_data(	src_layer->members, &asw ) );
	    while( vector_find_data( src_layer->members, &asw ) < src_layer->members->used )
	    {
    	        vector_remove_elem( src_layer->members, &asw );
        LOCAL_DEBUG_OUT( "after delition can be found at index %d(used%d)", vector_find_data(   src_layer->members, &asw ), src_layer->members->used );
	    }
	}
    }
}


Bool
enlist_aswindow( ASWindow *t )
{
    if( Scr.Windows == NULL )
        Scr.Windows = init_aswindow_list();

    append_bidirelem( Scr.Windows->clients, t );
    vector_insert_elem( Scr.Windows->circulate_list, &t, 1, NULL, True );

    add_aswindow_to_layer( t, ASWIN_LAYER(t) );
    tie_aswindow( t );
	
	publish_aswindow_list( Scr.Windows, False );	
    
	return True;
}

void
delist_aswindow( ASWindow *t )
{
    Bool skip_winlist;
    if( Scr.Windows == NULL )
        return ;

    if( AS_ASSERT(t) )
        return;
    /* set desktop for window */
    if( t->w != Scr.Root )
        vector_remove_elem( Scr.Windows->circulate_list, &t );

    if( ASWIN_GET_FLAGS(t, AS_Sticky ) )
        vector_remove_elem( Scr.Windows->sticky_list, &t );

    remove_aswindow_from_layer( t, ASWIN_LAYER(t) );

    untie_aswindow( t );
    skip_winlist = get_flags(t->hints->flags, AS_SkipWinList);
    discard_bidirelem( Scr.Windows->clients, t );

	publish_aswindow_list( Scr.Windows, False );	   
}

#if 0
void
update_visibility( int desk )
{
    static ASVector *ptrs = NULL ;
    static ASVector *layers = NULL ;
    static ASVector *rects = NULL ;
    unsigned long layers_in ;
    register long windows_num ;
    ASLayer **l ;
    ASWindow  **asws ;
    XRectangle *vrect;
    XRectangle srect ;
    int i;

    if( !IsValidDesk(desk) )
    {
        if( ptrs )
            destroy_asvector( &ptrs );
        if( layers )
            destroy_asvector( &layers );
        if( rects )
            destroy_asvector( &rects );
        return;
    }

    if( Scr.Windows->clients->count == 0)
        return;

    if( layers == NULL )
        layers = create_asvector( sizeof(ASLayer*) );
    if( Scr.Windows->layers->items_num > layers->allocated )
        realloc_vector( layers, Scr.Windows->layers->items_num );

    if( ptrs == NULL )
        ptrs = create_asvector( sizeof(ASWindow*) );
    else
        flush_vector( ptrs );
    if( Scr.Windows->clients->count > ptrs->allocated )
        realloc_vector( ptrs, Scr.Windows->clients->count );

    if( (layers_in = sort_hash_items (Scr.Windows->layers, NULL, (void**)VECTOR_HEAD_RAW(*layers), 0)) == 0 )
        return ;

    l = VECTOR_HEAD(ASLayer*,*layers);
    asws = VECTOR_HEAD(ASWindow*,*ptrs);
    windows_num = 0 ;
    for( i = 0 ; i < layers_in ; i++ )
    {
        register int k, end_k = VECTOR_USED(*(l[i]->members)) ;
        register ASWindow **members = VECTOR_HEAD(ASWindow*,*(l[i]->members));
        if( end_k > ptrs->allocated )
            end_k = ptrs->allocated ;
        for( k = 0 ; k < end_k ; k++ )
            if( ASWIN_DESK(members[k]) == desk && !ASWIN_GET_FLAGS(members[k], AS_Dead))
                asws[windows_num++] = members[k];
    }
    VECTOR_USED(*ptrs) = windows_num ;

    if( rects == NULL )
        rects = create_asvector( sizeof(XRectangle) );
    VECTOR_USED(*rects) = 0;

    srect.x = 0;
    srect.y = 0;
    srect.width = Scr.MyDisplayWidth ;
    srect.height = Scr.MyDisplayHeight ;

    append_vector( rects, &srect, 1 );
    vrect = VECTOR_HEAD(XRectangle,*rects);

    for( i = 0 ; i < windows_num ; ++i )
    {
        ASCanvas *client = asws[i]->client_canvas ;
        ASCanvas *frame = asws[i]->frame_canvas ;
		int client_left, client_right, client_bottom, client_top ; 
        int r ;
		Bool visible = False;

        if( ASWIN_GET_FLAGS( asws[i], AS_Iconic ) )
        {
            frame = client = asws[i]->icon_canvas ;
            if( frame == NULL )
                continue ;
        }else if( ASWIN_GET_FLAGS( asws[i], AS_Shaded ) )
            client = frame ;

        ASWIN_CLEAR_FLAGS( asws[0], AS_Hidden );

        r = VECTOR_USED(*rects);
		client_left = client->root_x ;
		client_top = client->root_y ;		
		client_right = client_left + (int)client->width+(int)client->bw*2 ;		
		client_bottom = client_top + (int)client->height+(int)client->bw*2 ;
        while( --r >= 0 )
        {
            if( client_right  > vrect[r].x &&
                client_left   < vrect[r].x+(int)vrect[r].width &&
                client_bottom > vrect[r].y &&
                client_top    < vrect[r].y+(int)vrect[r].height )
            {
				visible = True ;
                break;
            }
        }
		if( !visible ) 
			ASWIN_SET_FLAGS( asws[0], AS_Hidden );
    }
}
#endif

void
restack_window_list( int desk, Bool send_msg_only )
{
    static ASVector *ids = NULL ;
    static ASVector *layers = NULL ;
    unsigned long layers_in, i ;
    register long windows_num ;
    ASLayer **l ;
    Window  *windows ;

    if( !IsValidDesk(desk) )
    {
        if( ids )
            destroy_asvector( &ids );
        if( layers )
            destroy_asvector( &layers );
        return;
    }

    if( Scr.Windows->clients->count == 0)
        return;

    if( layers == NULL )
        layers = create_asvector( sizeof(ASLayer*) );
    if( Scr.Windows->layers->items_num > layers->allocated )
        realloc_vector( layers, Scr.Windows->layers->items_num );

    if( ids == NULL )
        ids = create_asvector( sizeof(Window) );
    else
        flush_vector( ids );
    if( Scr.Windows->clients->count+1 > ids->allocated )
        realloc_vector( ids, Scr.Windows->clients->count+1 );

    if( (layers_in = sort_hash_items (Scr.Windows->layers, NULL, (void**)VECTOR_HEAD_RAW(*layers), 0)) == 0 )
        return ;

    l = PVECTOR_HEAD(ASLayer*,layers);
    windows = PVECTOR_HEAD(Window,ids);
    windows_num = 0 ;
    LOCAL_DEBUG_OUT( "filling up array with window IDs: layers_in = %ld, Total clients = %d", layers_in, Scr.Windows->clients->count );
    for( i = 0 ; i < layers_in ; i++ )
    {
        register int k, end_k = PVECTOR_USED(l[i]->members) ;
        ASWindow **members = PVECTOR_HEAD(ASWindow*,l[i]->members);
        if( end_k > ids->allocated )
            end_k = ids->allocated ;
        LOCAL_DEBUG_OUT( "layer[%ld] = %d, end_k = %d", i, l[i]->layer, end_k );
        for( k = 0 ; k < end_k ; k++ )
        {
            register ASWindow *asw = members[k] ;
            if( ASWIN_DESK(asw) == desk )
                if( !ASWIN_GET_FLAGS(asw, AS_Dead) )
                    windows[windows_num++] = asw->w;
        }
    }
    LOCAL_DEBUG_OUT( "Sending stacking order: windows_num = %ld, ", windows_num );
    PVECTOR_USED(ids) = windows_num ;
    SendStackingOrder (-1, M_STACKING_ORDER, desk, ids);
	
	publish_aswindow_list( Scr.Windows, True );

    if( !send_msg_only )
    {
        l = PVECTOR_HEAD(ASLayer*,layers);
        windows = PVECTOR_HEAD(Window,ids);
        windows_num = 0 ;
        if( get_desktop_cover_window() )
        {    
            windows[0] = get_desktop_cover_window();
            ++windows_num ;
        }
        LOCAL_DEBUG_OUT( "filling up array with window frame IDs: layers_in = %ld, ", layers_in );
        for( i = 0 ; i < layers_in ; i++ )
        {
            register int k, end_k = PVECTOR_USED(l[i]->members) ;
            register ASWindow **members = PVECTOR_HEAD(ASWindow*,l[i]->members);
            if( windows_num+end_k > ids->allocated )
                end_k = ids->allocated - windows_num ;
            LOCAL_DEBUG_OUT( "layer[%ld] = %d, end_k = %d", i, l[i]->layer, end_k );
            for( k = 0 ; k < end_k ; k++ )
                if( ASWIN_DESK(members[k]) == desk && !ASWIN_GET_FLAGS(members[k], AS_Dead))
				{
                    windows[windows_num++] = get_window_frame(members[k]);
					LOCAL_DEBUG_OUT( "\t windows[%d] = %lX", k, windows[windows_num-1] );
				}
        }

        LOCAL_DEBUG_OUT( "Setting stacking order: windows_num = %ld, ", windows_num );
        if( windows_num > 0 )
        {
            XRaiseWindow( dpy, windows[0] );
            if( windows_num > 1 )
                XRestackWindows( dpy, windows, windows_num );
            XSync(dpy, False);
        }
        raise_scren_panframes (ASDefaultScr);
        XRaiseWindow(dpy, Scr.ServiceWin);
    }
}

/*
 * we better have our own routine for changing window stacking order,
 * instead of simply passing it to X server, whenever client request
 * such thing, as we know more about window layout then server does
 */
/* From Xlib reference :
If a sibling and a stack_mode are specified, the window is restacked
 as follows:
 Above 		The window is placed just above the sibling.
 Below    	The window is placed just below the sibling.
 TopIf          If any sibling occludes the window, the window is placed
                at the top of the stack.
 BottomIf       If the window occludes any sibling, the window is placed
                at the bottom of the stack.
 Opposite       If any sibling occludes the window, the window is placed
                at the top of the stack. If the window occludes any
                sibling, the window is placed at the bottom of the stack.
*/
#define OCCLUSION_ABOVE		-1
#define OCCLUSION_NONE		 0
#define OCCLUSION_BELOW		 1

/* Checks if rectangle above is at least partially obscuring client below */
inline Bool
is_rect_overlaping (ASRectangle * above, ASRectangle *below)
{
	if (above == NULL)
		return False;
	if (below == NULL)
		return True;

	return (above->x < below->x + below->width && above->x + above->width > below->x &&
			above->y < below->y + below->height && above->y + above->height > below->y);
}

inline Bool
is_status_overlaping (ASStatusHints * above, ASStatusHints *below)
{
	if (above == NULL)
		return False;
	if (below == NULL)
		return True;

	return (above->x < below->x + below->width && above->x + above->width > below->x &&
			above->y < below->y + below->height && above->y + above->height > below->y);
}

inline Bool
is_canvas_overlaping (ASCanvas * above, ASCanvas *below)
{
	if (above == NULL)
		return False;
	if (below == NULL)
		return True;
	else
	{
		int below_left = below->root_x ;
		int below_right = below_left + (int)below->width + (int)below->bw*2 ;
		int above_left = above->root_x ;
		int above_right = above_left + (int)above->width + (int)above->bw*2 ;
	    if( above_left < below_right && above_right > below_left ) 
		{
			int below_top = below->root_y ;
			int below_bottom = below_top + (int)below->height + (int)below->bw*2 ;
			int above_top = above->root_y ;
			int above_bottom = above_top + (int)above->height + (int)above->bw*2 ;
		  	
			return (above_top < below_bottom && above_bottom > below_top);
		}
	}
	return False ;
}

#define IS_OVERLAPING(a,b)    is_canvas_overlaping((a)->frame_canvas,(b)->frame_canvas)

Bool
is_window_obscured (ASWindow * above, ASWindow * below)
{
    ASLayer           *l ;
    ASWindow **members ;

	if (above != NULL && below != NULL)
        return IS_OVERLAPING(above, below);

	if (above == NULL && below != NULL)
    {/* checking if window "below" is completely obscured by any of the
        windows with the same layer above it in stacking order */
        register int i, end_i ;

        l = get_aslayer( ASWIN_LAYER(below), Scr.Windows );
	if( AS_ASSERT(l) )
	    return False;

        end_i = l->members->used ;
        members = VECTOR_HEAD(ASWindow*,*(l->members));
        for (i = 0 ; i < end_i ; i++ )
        {
            register ASWindow *t ;
            if( (t = members[i]) == below )
			{
				return False;
            }else if( ASWIN_DESK(t) == ASWIN_DESK(below) )
			{
                if (IS_OVERLAPING(t,below))
				{
					return True;
				}
			}
        }
    }else if (above != NULL )
    {   /* checking if window "above" is completely obscuring any of the
           windows with the same layer below it in stacking order */
        register int i ;

        l = get_aslayer( ASWIN_LAYER(above), Scr.Windows );
	if( AS_ASSERT(l) )
	    return False;
        members = VECTOR_HEAD(ASWindow*,*(l->members));
        for (i = VECTOR_USED(*(l->members))-1 ; i >= 0 ; i-- )
        {
            register ASWindow *t ;
            if( (t = members[i]) == above )
				return False;
            else if( ASWIN_DESK(t) == ASWIN_DESK(above) )
                if (IS_OVERLAPING(above,t) )
					return True;
        }
    }
	return False;
}

void
restack_window( ASWindow *t, Window sibling_window, int stack_mode )
{
    ASWindow *sibling = NULL;
    ASLayer  *dst_layer = NULL, *src_layer ;
    Bool above ;
    int occlusion = OCCLUSION_NONE;

    if( t == NULL )
        return ;

LOCAL_DEBUG_CALLER_OUT( "%p,%lX,%d", t, sibling_window, stack_mode );
    src_layer = get_aslayer( ASWIN_LAYER(t), Scr.Windows );

    if( sibling_window )
        if( (sibling = window2ASWindow( sibling_window )) != NULL )
        {
            if ( sibling->transient_owner == t )
                sibling = NULL;                    /* can't restack relative to its own transient */
            else if (ASWIN_DESK(sibling) != ASWIN_DESK(t) )
                sibling = NULL;                    /* can't restack relative to window on the other desk */
            else
                dst_layer = get_aslayer( ASWIN_LAYER(sibling), Scr.Windows );
        }

    if( dst_layer == NULL )
        dst_layer = src_layer ;

    /* 2. do all the occlusion checks whithin our layer */
	if (stack_mode == TopIf)
	{
		LOCAL_DEBUG_OUT( "stack_mode = %s", "TopIf");
        if (is_window_obscured (sibling, t))
			occlusion = OCCLUSION_BELOW;
	} else if (stack_mode == BottomIf)
	{
		LOCAL_DEBUG_OUT( "stack_mode = %s", "BottomIf");
        if (is_window_obscured (t, sibling))
			occlusion = OCCLUSION_ABOVE;
	} else if (stack_mode == Opposite)
	{
        if (is_window_obscured (sibling, t))
		{
			occlusion = OCCLUSION_BELOW;
			LOCAL_DEBUG_OUT( "stack_mode = opposite, occlusion = %s", "below");
        }else if (is_window_obscured (t, sibling))
		{
			occlusion = OCCLUSION_ABOVE;
			LOCAL_DEBUG_OUT( "stack_mode = opposite, occlusion = %s", "above");
		}else
		{
			LOCAL_DEBUG_OUT( "stack_mode = opposite, occlusion = %s","none" );
		}
	}
	if (sibling)
        if (ASWIN_LAYER(sibling) != ASWIN_LAYER(t) )
			occlusion = OCCLUSION_NONE;

	if (!((stack_mode == TopIf && occlusion == OCCLUSION_BELOW) ||
		  (stack_mode == BottomIf && occlusion == OCCLUSION_ABOVE) ||
		  (stack_mode == Opposite && occlusion != OCCLUSION_NONE) ||
		  stack_mode == Above || stack_mode == Below))
	{
		return;								   /* nothing to be done */
	}

    above = ( stack_mode == Above || stack_mode == TopIf ||
              (stack_mode == Opposite && occlusion == OCCLUSION_BELOW));

    if( stack_mode != Above && stack_mode != Below )
        sibling = NULL ;

    if( src_layer )
        vector_remove_elem( src_layer->members, &t );
    if( dst_layer )
        vector_insert_elem( dst_layer->members, &t, 1, sibling, above );

    t->last_restack_time = Scr.last_Timestamp ;
    restack_window_list( ASWIN_DESK(t), False );
}


/*
 * Find next window in circulate csequence forward (dir 1) or backward (dir -1)
 * from specifyed window. when we reach top or bottom we are turning back
 * checking AutoRestart here to determine what to do when we have warped through
 * all the windows, and came back to start.
 */

ASWindow     *
get_next_window (ASWindow * curr_win, char *action, int dir)
{
    int end_i, i;
    ASWindow **clients;

    if( Scr.Windows == NULL || curr_win == NULL )
        return NULL;

    end_i = VECTOR_USED(*(Scr.Windows->circulate_list)) ;
    clients = VECTOR_HEAD(ASWindow*,*(Scr.Windows->circulate_list));

    if( end_i <= 1 )
        return NULL;
    for( i = 0 ; i < end_i ; ++i )
        if( clients[i] == curr_win )
        {
            if( i == 0 && dir < 0 )
                return clients[end_i-1];
            else if( i == end_i-1 && dir > 0 )
                return clients[0];
            else
                return clients[i+dir];
        }

    return NULL;
}

/********************************************************************
 * hides focus for the screen.
 **********************************************************************/
void
hide_focus()
{
    if (get_flags(Scr.Feel.flags, ClickToFocus) && Scr.Windows->ungrabbed != NULL)
        grab_aswindow_buttons( Scr.Windows->ungrabbed, False );

	LOCAL_DEBUG_CALLER_OUT( "CHANGE Scr.Windows->focused from %p to NULL", Scr.Windows->focused );
    Scr.Windows->focused = NULL;
    Scr.Windows->ungrabbed = NULL;
    XRaiseWindow(dpy, Scr.ServiceWin);
	LOCAL_DEBUG_OUT( "XSetInputFocus(window= %lX (service_win), time = %lu)", Scr.ServiceWin, Scr.last_Timestamp);
    XSetInputFocus (dpy, Scr.ServiceWin, RevertToParent, Scr.last_Timestamp);
    XSync(dpy, False );
}

/********************************************************************
 * Sets the input focus to the indicated window.
 **********************************************************************/
void
commit_circulation()
{
    ASWindow *asw = Scr.Windows->active ;
LOCAL_DEBUG_OUT( "circulation completed with active window being %p", asw );
    if( asw )
        if( vector_remove_elem( Scr.Windows->circulate_list, &asw ) == 1 )
        {
            LOCAL_DEBUG_OUT( "reinserting %p into the head of circulation list : ", asw );
            vector_insert_elem( Scr.Windows->circulate_list, &asw, 1, NULL, True );
        }
    Scr.Windows->warp_curr_index = -1 ;
}

void autoraise_aswindow( void *data )
{
    struct timeval tv;
    time_t msec = Scr.Feel.AutoRaiseDelay ;
    time_t exp_sec = Scr.Windows->last_focus_change_sec + (msec * 1000 + Scr.Windows->last_focus_change_usec) / 1000000;
    time_t exp_usec = (msec * 1000 + Scr.Windows->last_focus_change_usec) % 1000000;

    if( Scr.Windows->focused && !get_flags( AfterStepState, ASS_HousekeepingMode) )
    {
        gettimeofday (&tv, NULL);
        if( exp_sec < tv.tv_sec ||
            (exp_sec == tv.tv_sec && exp_usec <= tv.tv_usec ) )
        {
            RaiseObscuredWindow(Scr.Windows->focused);
        }
    }
}

Bool
focus_window( ASWindow *asw, Window w )
{
	LOCAL_DEBUG_CALLER_OUT( "asw = %p, w = %lX", asw, w );

  	if( asw != NULL )
        if (get_flags(asw->hints->protocols, AS_DoesWmTakeFocus) && !ASWIN_GET_FLAGS(asw, AS_Dead))
            send_wm_protocol_request (asw->w, _XA_WM_TAKE_FOCUS, Scr.last_Timestamp);

    ASSync(False);
    LOCAL_DEBUG_OUT( "focusing window %lX, client %lX, frame %lX, asw %p", w, asw->w, asw->frame, asw );
	/* using last_Timestamp here causes problems when moving between screens */
	/* at the same time using CurrentTime all the time seems to cause some apps to fail,
	 * most noticeably GTK-perl 
	 * 
	 * Take 2: disabled CurrentTime altogether as it screwes up focus handling
	 * Basically if you use CurrentTime when there are still bunch of Events 
	 * in the queue, those evens will not have any effect if you try setting 
	 * focus using their time, as X aready used its own friggin current time.
	 * Don't ask, its a mess.
	 * */
	if( w != None && ASWIN_HFLAGS(asw, AS_AcceptsFocus))
	{
		Time t = /*(Scr.Windows->focused == NULL)?CurrentTime:*/Scr.last_Timestamp ;
		LOCAL_DEBUG_OUT( "XSetInputFocus(window= %lX, time = %lu)", w, t);	  
	    XSetInputFocus ( dpy, w, RevertToParent, t );
	}

    ASSync(False );
    return (w!=None);
}

void autoraise_window( ASWindow *asw ) 
{
    if (Scr.Feel.AutoRaiseDelay == 0)
    {
        RaiseWindow( asw );
    }else if (Scr.Feel.AutoRaiseDelay > 0)
    {
        struct timeval tv;
		LOCAL_DEBUG_OUT( "setting autoraise timer for asw %p", Scr.Windows->focused );
        gettimeofday (&tv, NULL);
        Scr.Windows->last_focus_change_sec =  tv.tv_sec;
        Scr.Windows->last_focus_change_usec = tv.tv_usec;
        timer_new (Scr.Feel.AutoRaiseDelay, autoraise_aswindow, Scr.Windows->focused);
    }

}

Bool
focus_aswindow( ASWindow *asw, Bool suppress_autoraise )
{
    Bool          do_hide_focus = False ;
    Bool          do_nothing = False ;
    Window        w = None;

LOCAL_DEBUG_CALLER_OUT( "asw = %p", asw );
    if( asw )
    {
        if (!get_flags( AfterStepState, ASS_WarpingMode) )
            if( vector_remove_elem( Scr.Windows->circulate_list, &asw ) == 1 )
                vector_insert_elem( Scr.Windows->circulate_list, &asw, 1, NULL, True );

#if 0
        /* ClickToFocus focus queue manipulation */
        if ( asw != Scr.Focus )
            asw->focus_sequence = Scr.next_focus_sequence++;
#endif
        do_hide_focus = (ASWIN_DESK(asw) != Scr.CurrentDesk) ||
                        (ASWIN_GET_FLAGS( asw, AS_Iconic ) &&
                            asw->icon_canvas == NULL && asw->icon_title_canvas == NULL );

        if( !ASWIN_FOCUSABLE(asw))
        {
            if( Scr.Windows->focused != NULL && ASWIN_DESK(Scr.Windows->focused) == Scr.CurrentDesk )
                do_nothing = True ;
            else
                do_hide_focus = True ;
        }
    }else
        do_hide_focus = True ;

    if (Scr.NumberOfScreens > 1 && !do_hide_focus )
	{
        Window pointer_root ;
        /* if pointer went onto another screen - we need to release focus
         * and let other screen's manager manage it from now on, untill
         * pointer comes back to our screen :*/
        ASQueryPointerRoot(&pointer_root,&w);
        if(pointer_root != Scr.Root)
        {
            do_hide_focus = True;
            do_nothing = False ;
        }
    }
    if( !do_nothing && do_hide_focus )
        hide_focus();
    if( do_nothing || do_hide_focus )
        return False;

    if (get_flags(Scr.Feel.flags, ClickToFocus) && Scr.Windows->ungrabbed != asw)
    {  /* need to grab all buttons for window that we are about to
        * unfocus */
        grab_aswindow_buttons( Scr.Windows->ungrabbed, False );
        grab_aswindow_buttons( asw, True );
        Scr.Windows->ungrabbed = asw;
    }

    if( ASWIN_GET_FLAGS(asw, AS_Iconic ) )
    { /* focus icon window or icon title of the iconic window */
        if( asw->icon_canvas && !ASWIN_GET_FLAGS(asw, AS_Dead) && validate_drawable(asw->icon_canvas->w, NULL, NULL) != None )
            w = asw->icon_canvas->w;
        else if( asw->icon_title_canvas )
            w = asw->icon_title_canvas->w;
    }else if( ASWIN_GET_FLAGS(asw, AS_Shaded ) )
    { /* focus frame window of shaded clients */
        w = asw->frame ;
    }else if( !ASWIN_GET_FLAGS(asw, AS_Dead) )
    { /* clients with visible top window can get focus directly:  */
        w = asw->w ;
    }

    if( w == None )
        show_warning( "unable to focus window %lX for client %lX, frame %lX", w, asw->w, asw->frame );
    else if( !ASWIN_GET_FLAGS(asw, AS_Mapped) )
        show_warning( "unable to focus unmapped window %lX for client %lX, frame %lX", w, asw->w, asw->frame );
    else if( ASWIN_GET_FLAGS(asw, AS_UnMapPending) )
        show_warning( "unable to focus window %lX that is about to be unmapped for client %lX, frame %lX", w, asw->w, asw->frame );
    else
    {
		focus_window( asw, w );

		LOCAL_DEBUG_CALLER_OUT( "CHANGE Scr.Windows->focused from %p to %p", Scr.Windows->focused, asw );
        Scr.Windows->focused = asw ;
		if( !suppress_autoraise ) 
			autoraise_window( asw ); 
    }

    XSync(dpy, False );
    return True;
}

/*********************************************************************/
/* focus management goes here :                                      */
/*********************************************************************/
/* making window active : */
/* handing over actuall focus : */
Bool
focus_active_window()
{
    /* don't fiddle with focus if we are in housekeeping mode !!! */
LOCAL_DEBUG_CALLER_OUT( "checking if we are in housekeeping mode (%ld)", get_flags(AfterStepState, ASS_HousekeepingMode) );
    if( get_flags(AfterStepState, ASS_HousekeepingMode) || Scr.Windows->active == NULL )
        return False ;

    if( Scr.Windows->focused == Scr.Windows->active )
        return True ;                          /* already has focus */

    return focus_aswindow( Scr.Windows->active, FOCUS_ASW_CAN_AUTORAISE );
}

/* second version of above : */
void
focus_next_aswindow( ASWindow *asw )
{
    ASWindow     *new_focus = NULL;

    if( get_flags(Scr.Feel.flags, ClickToFocus))
        new_focus = get_next_window (asw, NULL, 1);
    if( !activate_aswindow( new_focus, False, False) )
        hide_focus();
}

void
focus_prev_aswindow( ASWindow *asw )
{
    ASWindow     *new_focus = NULL;

    if( get_flags(Scr.Feel.flags, ClickToFocus))
        new_focus = get_next_window (asw, NULL, -1);
    if( !activate_aswindow( new_focus, False, False) )
        hide_focus();
}

void
warp_to_aswindow( ASWindow *asw, Bool deiconify )
{
    if( asw )
        activate_aswindow( asw, True, deiconify );
}

/*************************************************************************/
/* end of the focus management                                           */
/*************************************************************************/


/*********************************************************************************
 * Find next window in circulate csequence forward (dir 1) or backward (dir -1)
 * from specifyed window. when we reach top or bottom we are turning back
 * checking AutoRestart here to determine what to do when we have warped through
 * all the windows, and came back to start.
 *********************************************************************************/
ASWindow     *
warp_aswindow_list ( ASWindowList *list, Bool backwards )
{
    register int i;
    register int dir = backwards ? -1 : 1 ;
    int end_i;
    ASWindow **clients;
    int loop_count = 0 ;

    if( list == NULL ) return NULL;

	end_i = VECTOR_USED(*(list->circulate_list)) ;
	clients = VECTOR_HEAD(ASWindow*,*(list->circulate_list));

    if( end_i <= 1 )
        return NULL;

    if( list->warp_curr_index < 0 )
    { /* need to initialize warping : */
        list->warp_curr_index = (dir > 0)? 0 : end_i ;
        list->warp_user_dir = dir ;
        list->warp_init_dir = dir ;
        list->warp_curr_dir = dir ;
    }else if( dir == list->warp_user_dir )
    {
        dir = list->warp_curr_dir ;
    }else
    {
        list->warp_user_dir = dir ;
        /* user reversed direction - so do we : */
        dir = (list->warp_curr_dir > 0)? -1 : 1 ;
        list->warp_curr_dir = dir ;
    }

    i = list->warp_curr_index+dir ;
    do
    {
LOCAL_DEBUG_OUT("checking i(%d)->end_i(%d)->dir(%d)->AutoReverse(%d)", i, end_i, dir, Scr.Feel.AutoReverse);
        if( 0 > i || i >= end_i )
        {
            if( Scr.Feel.AutoReverse == AST_OpenLoop )
                i = (dir < 0)? end_i - 1 : 0 ;
            else if( Scr.Feel.AutoReverse == AST_ClosedLoop )
            {
                i = (dir < 0)? 0 : end_i - 1 ;
                list->warp_curr_dir = dir = (dir < 0 )? 1 : -1 ;
                i += dir;                      /* we need to skip the one that was focused at the moment ! */
            }else
                return NULL;
            if( ++loop_count >= 2 )
                return NULL;
        }

        list->warp_curr_index = i ;
        if( !(ASWIN_HFLAGS(clients[i], AS_DontCirculate)) &&
            !(ASWIN_GET_FLAGS(clients[i], AS_Iconic) && get_flags(Scr.Feel.flags, CirculateSkipIcons)) &&
			 (ASWIN_DESK(clients[i]) == Scr.CurrentDesk || get_flags(Scr.Feel.flags,AutoTabThroughDesks )))
	    {
            return clients[i];
		}
        i += dir ;
    }while(1);
    return NULL;
}

/********************************************************************************/
/* window list menus regeneration :                                             */
/********************************************************************************/
static inline void
ASWindow2func_data( FunctionCode func, ASWindow *asw, FunctionData *fdata, char *scut, Bool icon_name ) 
{
    fdata->func = F_RAISE_IT;
    fdata->name = mystrdup(icon_name ? ASWIN_ICON_NAME(asw) : ASWIN_NAME(asw));
	if( !icon_name )
	 	fdata->name_encoding = ASWIN_NAME_ENCODING(asw) ;
    fdata->func_val[0] = (long) asw;
    fdata->func_val[1] = (long) asw->w;
	if (++(*scut) == ('9' + 1))
		(*scut) = 'A';		/* Next shortcut key */
    fdata->hotkey = (*scut);
}

struct ASSortMenu_Aux
{
	FunctionData fdata ; 
	void *ref_data ; 
};

int
compare_menu_func_data_name(const void *a, const void *b) 
{
	struct ASSortMenu_Aux *aa = *(struct ASSortMenu_Aux **)a ;
	struct ASSortMenu_Aux *ab = *(struct ASSortMenu_Aux **)b ;

/*	LOCAL_DEBUG_OUT( "aa = %p, ab = %p", aa, ab ); */
	return strcmp(aa->fdata.name ? aa->fdata.name : "", ab->fdata.name ? ab->fdata.name : "");
}


MenuData *
make_desk_winlist_menu(  ASWindowList *list, int desk, int sort_order, Bool icon_name )
{
    char menu_name[256];
    MenuData *md ;
    MenuDataItem *mdi ;
    FunctionData  fdata ;
    char          scut = '0';                  /* Current short cut key */
    ASWindow **clients;
    int i, max_i;

    if( list == NULL )
        return NULL;;
    
	clients = PVECTOR_HEAD(ASWindow*,list->circulate_list);
    max_i = PVECTOR_USED(list->circulate_list);

    if( IsValidDesk( desk ) )
        sprintf( menu_name, "Windows on Desktop #%d", desk );
    else
        sprintf( menu_name, "Windows on All Desktops" );

    if( (md = create_menu_data (menu_name)) == NULL )
        return NULL;

    memset(&fdata, 0x00, sizeof(FunctionData));
    fdata.func = F_TITLE ;
    fdata.name = mystrdup(menu_name);
    add_menu_fdata_item( md, &fdata, NULL, NULL );

	if( sort_order == ASO_Alpha ) 
	{
		struct ASSortMenu_Aux **menuitems = safecalloc(max_i, sizeof(struct ASSortMenu_Aux *));
		int numitems = 0;
        
		for( i = 0 ; i < max_i ; ++i ) 
		{
            if ((ASWIN_DESK(clients[i]) == desk || !IsValidDesk(desk)) && !ASWIN_HFLAGS(clients[i], AS_SkipWinList)) 
			{
				menuitems[numitems] = safecalloc(1, sizeof(struct ASSortMenu_Aux));
/*				LOCAL_DEBUG_OUT( "menuitems[%d] = %p", numitems, menuitems[numitems] ); */
				menuitems[numitems]->ref_data = clients[i];
				ASWindow2func_data( F_RAISE_IT, clients[i], &(menuitems[numitems]->fdata), &scut, icon_name ); 
				++numitems;
            }
        }
		qsort(menuitems, numitems, sizeof(struct ASSortMenu_Aux *), compare_menu_func_data_name);
		for( i = 0 ; i < numitems ; ++i ) 
		{
			if( (mdi = add_menu_fdata_item( md, &(menuitems[i]->fdata), NULL, 
								 get_flags( Scr.Feel.flags, WinListHideIcons) ? NULL : 
								 get_client_icon_image(ASDefaultScr, ((ASWindow*)(menuitems[i]->ref_data))->hints))) != NULL )
				set_flags( mdi->flags, MD_ScaleMinipixmapDown );
			safefree(menuitems[i]); /* scrubba-dub-dub */
		}
		safefree(menuitems);
    } else /* if( sort_order == ASO_Circulation || sort_order == ASO_Stacking ) */
	{
        for( i = 0 ; i < max_i ; ++i ) 
        {
            if ((ASWIN_DESK(clients[i]) == desk || !IsValidDesk(desk)) && !ASWIN_HFLAGS(clients[i], AS_SkipWinList))
			{
				ASWindow2func_data( F_RAISE_IT, clients[i], &fdata, &scut, icon_name ); 
                if( (mdi = add_menu_fdata_item( md, &fdata, NULL, get_flags( Scr.Feel.flags, WinListHideIcons)? NULL : 
									 get_client_icon_image( ASDefaultScr, clients[i]->hints))) != NULL ) 
					set_flags( mdi->flags, MD_ScaleMinipixmapDown );									 	
            }
        }
    }
    return md;
}


