/*
 * Copyright (C) 2000 Sasha Vasko <sasha at aftercode.net>
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

/**************************************************************************
 *
 * styledb management functionality
 *
 **************************************************************************/

#include "../configure.h"

#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "../include/afterbase.h"
#include "../include/aftersteplib.h"
#include "../include/afterstep.h"
#include "../include/asdatabase.h"

ASDatabaseRecord *make_asdb_record ( name_list * nl, struct wild_reg_exp *regexp,
                                     ASDatabaseRecord *reusable_memory )
{
    ASDatabaseRecord *db_rec = NULL;
    register int i ;

    if( nl )
    {
        if( reusable_memory )
        {
            db_rec = reusable_memory ;
            memset( db_rec, 0x00, sizeof( ASDatabaseRecord ));
        }else
            db_rec = (ASDatabaseRecord*)safecalloc(1, sizeof(ASDatabaseRecord));

        db_rec->magic = MAGIC_ASDBRECORD;

        db_rec->regexp = regexp ;

        db_rec->set_flags   = nl->set_flags;
        db_rec->flags       = nl->flags;
        /* TODO: implement set_buttons/buttons in name_list as well */
        db_rec->set_buttons = nl->on_buttons|nl->off_buttons;
        db_rec->buttons     = nl->on_buttons;

        if( db_rec->set_buttons != 0 )
            set_flags( db_rec->set_flags, STYLE_BUTTONS );

        db_rec->desk        = nl->Desk;
        db_rec->layer       = nl->layer;
        db_rec->viewport_x  = nl->ViewportX;
        db_rec->viewport_y  = nl->ViewportY;
        db_rec->border_width= nl->border_width;
        db_rec->resize_width= nl->resize_width;
        db_rec->gravity     = nl->gravity;

        if( nl->icon_file )
        {
            db_rec->icon_file = nl->icon_file ;
            nl->icon_file = NULL ;
        }
        if( nl->frame_name )
        {
            db_rec->frame_name = nl->frame_name ;
            nl->frame_name = NULL ;
        }
        for( i = 0 ; i < BACK_STYLES ; i++ )
            if( nl->window_styles[i] )
            {
                db_rec->window_styles[i] = nl->window_styles[i];
                nl->window_styles[i] = NULL ;
                set_flags( db_rec->set_flags, STYLE_MYSTYLES );
            }
        db_rec->own_strings = True ;
    }
    return db_rec;
}

static Bool
build_matching_list( ASDatabase *db, char **names )
{
    int last = 0 ;
    if( names && db && db->match_list)
    {
        register int i, k ;
        while ( *names )
        {
            for( i = 0 ; i < db->styles_num ; ++i )
            {
                for( k = 0 ; k < last ; ++k )  /* little optimization */
                    if( db->match_list[k] == i )
                        break;
                if( k >= last )
                    if( match_wild_reg_exp( *names, db->styles_table[i].regexp ) == 0 )
                        db->match_list[last++] = i ;
            }
            names++ ;
        }
        db->match_list[last++] = -1 ;
    }
    return (last > 0);
}

typedef enum {
    MATCH_Flags         = STYLE_FLAGS,
    MATCH_Buttons       = STYLE_BUTTONS,

    MATCH_Desk          = STYLE_STARTUP_DESK,
    MATCH_layer         = STYLE_LAYER       ,
    MATCH_ViewportX     = STYLE_VIEWPORTX   ,
    MATCH_ViewportY     = STYLE_VIEWPORTY   ,
    MATCH_border_width  = STYLE_BORDER_WIDTH,
    MATCH_resize_width  = STYLE_HANDLE_WIDTH,
    MATCH_gravity       = STYLE_GRAVITY     ,
    MATCH_DefaultGeometry = STYLE_DEFAULT_GEOMETRY,

    MATCH_Icon          = STYLE_ICON,
    MATCH_Frame         = STYLE_FRAME,
    MATCH_MyStyle       = STYLE_MYSTYLES
}DBMatchType ;

static ASDatabaseRecord *
get_asdb_record( ASDatabase *db, int index )
{
    if( index < 0 || index >= db->styles_num )
        return &(db->style_default );
    return &(db->styles_table[index]);
}

static void
match_flags( unsigned long *pset_flags, unsigned long *pflags,
             ASDatabase *db, DBMatchType type)
{
    if( db && pset_flags && pflags )
    {
        unsigned long curr_set_flags, curr_flags;
        register ASDatabaseRecord *db_rec ;
        register int i = 0;
        do
        {
            db_rec = get_asdb_record( db, db->match_list[i] );
            curr_set_flags = (type == MATCH_Buttons)? db_rec->set_buttons : db_rec->set_flags;
            curr_flags     = (type == MATCH_Buttons)? db_rec->buttons : db_rec->flags ;
            /* we should not touch flags that are already set */
            set_flags(*pflags, get_flags( curr_flags, ~(*pset_flags) )) ;
            set_flags(*pset_flags, curr_set_flags );
        }while( db->match_list[i++] >= 0 );
    }
}

static int
match_int( ASDatabase *db, DBMatchType type )
{
    if( db )
    {
        register ASDatabaseRecord *db_rec ;
        register int i = 0;
        do{
            db_rec = get_asdb_record( db, db->match_list[i] );
            if( get_flags( db_rec->set_flags, type ) )
            {
                switch( type )
                {
                    case MATCH_Desk         : return db_rec->desk ;
                    case MATCH_layer        : return db_rec->layer ;
                    case MATCH_ViewportX    : return db_rec->viewport_y ;
                    case MATCH_ViewportY    : return db_rec->viewport_x ;
                    case MATCH_border_width : return db_rec->border_width ;
                    case MATCH_resize_width : return db_rec->resize_width ;
                    case MATCH_gravity      : return db_rec->gravity ;
                    default: break ;
                }
                break;
            }
        }while( db->match_list[i++] >= 0 );
    }
    return 0;
}

static void *
match_struct( ASDatabase *db, DBMatchType type )
{
    if( db )
    {
        register ASDatabaseRecord *db_rec ;
        register int i = 0;
        do{
            db_rec = get_asdb_record( db, db->match_list[i] );
            if( get_flags( db_rec->set_flags, type ) )
            {
                switch( type )
                {
                    case MATCH_DefaultGeometry : return &(db_rec->default_geometry) ;
                    default: break ;
                }
                break;
            }
        }while( db->match_list[i++] >= 0 );
    }
    return NULL;
}


static char *
match_string( ASDatabase *db, DBMatchType type, unsigned int index, Bool dup_strings )
{
    char *res = NULL ;
    if( db )
    {
        register ASDatabaseRecord *db_rec ;
        register int i = 0 ;
        do{
            db_rec = get_asdb_record( db, db->match_list[i] );
            if( get_flags( db_rec->set_flags, type ) )
            {
                switch( type )
                {
                    case MATCH_Icon    : res = db_rec->icon_file ; break ;
                    case MATCH_Frame   : res = db_rec->frame_name; break ;
                    case MATCH_MyStyle :
                        res = db_rec->window_styles[(index<BACK_STYLES)?index:0] ;
                    default: break ;
                }
                break;
            }
        }while( db->match_list[i++] >= 0 );
    }
    if( res != NULL && dup_strings )
        res = mystrdup( res );
    return res;
}

/* NULL terminated list of names/aliases sorted in order of descending importance */
ASDatabaseRecord *fill_asdb_record (ASDatabase *db, char **names,
                                    ASDatabaseRecord *reusable_memory, Bool dup_strings)
{
    ASDatabaseRecord *db_rec = NULL ;
    if( db && names )
    {
        if( reusable_memory )
        {
            db_rec = reusable_memory ;
            memset( db_rec, 0x00, sizeof( ASDatabaseRecord ));
        }else
            db_rec = (ASDatabaseRecord*)safecalloc(1, sizeof(ASDatabaseRecord));

        db_rec->magic = MAGIC_ASDBRECORD;

        /* TODO : */
        /* build list of matching styles in order of descending importance */
        if( build_matching_list( db, names ) )
        {/* go through the list trying to extract as much data as possible  */
            register int i ;

            match_flags( &(db_rec->set_flags), &(db_rec->flags), db, MATCH_Flags );
            if( get_flags( db_rec->set_flags, STYLE_BUTTONS ) )
                match_flags( &(db_rec->set_buttons), &(db_rec->buttons), db, MATCH_Buttons );
            if( get_flags( db_rec->set_flags, STYLE_STARTUP_DESK ) )
                db_rec->desk        = match_int( db, MATCH_Desk );
            if( get_flags( db_rec->set_flags, STYLE_LAYER ) )
                db_rec->layer       = match_int( db, MATCH_layer );
            if( get_flags( db_rec->set_flags, STYLE_VIEWPORTY ) )
                db_rec->viewport_x  = match_int( db, MATCH_ViewportX );
            if( get_flags( db_rec->set_flags, STYLE_VIEWPORTX ) )
                db_rec->viewport_y  = match_int( db, MATCH_ViewportY );
            if( get_flags( db_rec->set_flags, STYLE_BORDER_WIDTH ) )
                db_rec->border_width= match_int( db, MATCH_border_width );
            if( get_flags( db_rec->set_flags, STYLE_HANDLE_WIDTH ) )
                db_rec->resize_width= match_int( db, MATCH_resize_width );
            if( get_flags( db_rec->set_flags, STYLE_GRAVITY ) )
                db_rec->gravity     = match_int( db, MATCH_gravity );

            if( get_flags( db_rec->set_flags, STYLE_DEFAULT_GEOMETRY ) )
                db_rec->default_geometry = *((ASGeometry*)match_struct( db, MATCH_DefaultGeometry ));

            if( get_flags( db_rec->set_flags, STYLE_ICON ) )
                db_rec->icon_file   = match_string( db, MATCH_Icon, 0, dup_strings );
            if( get_flags( db_rec->set_flags, STYLE_FRAME ) )
                db_rec->frame_name  = match_string( db, MATCH_Frame, 0, dup_strings );
            if( get_flags( db_rec->set_flags, STYLE_MYSTYLES ) )
                for( i = 0 ; i < BACK_STYLES ; i++ )
                    if( db_rec->window_styles[i] )
                        db_rec->window_styles[i] = match_string( db, MATCH_MyStyle, i, dup_strings );
        }
    }
    return db_rec;
}

void destroy_asdb_record(ASDatabaseRecord *rec, Bool reusable)
{
    if( rec == NULL ) return ;
    if( rec->magic == MAGIC_ASDBRECORD && rec->own_strings )
    {
        register int i ;

        rec->magic = 0 ;

        if( rec->regexp )
            destroy_wild_reg_exp( rec->regexp );

        if( rec->icon_file )
            free( rec->icon_file );
        if( rec->frame_name )
            free( rec->frame_name ) ;
        for( i = 0 ; i < BACK_STYLES ; i++ )
            if( rec->window_styles[i] )
                free( rec->window_styles[i] );
    }
    if( reusable )
        memset( rec, 0x00, sizeof(ASDatabaseRecord));  /* we are being paranoid */
    else
        free( rec );
}

static Bool /* returns True if record is completely new */
replace_record( ASDatabaseRecord *trg, name_list *nl, wild_reg_exp *regexp )
{
    Bool res = True ;
    if( trg->magic == MAGIC_ASDBRECORD )
    {
        destroy_asdb_record( trg, True );
        res = False ;
    }

    trg = make_asdb_record( nl, regexp, trg );
    return res;
}

static int
place_new_record( ASDatabase* db, wild_reg_exp *regexp )
{
    int res = -1 ;
    if( db && regexp )
    {
        register int i ;
        ASDatabaseRecord *tmp ;
        /* we have to find a location for regexp, by comparing it to others.
         * longer regexps should go first.
         * If we find exactly the same regexp - then we replace record.
         * Otherwise we reallocate space as needed.
         */
        for( i = 0 ; i < db->styles_num ; i++ )
        {
            res = compare_wild_reg_exp(db->styles_table[i].regexp, regexp);
            if( res == 0 ) return i ;
            else if( res < 0 ) break;
        }

        res = i ;

        if( db->allocated_num < db->styles_num + 1 )
        {       /* reallocating memory as needed : */
            db->allocated_num = db->styles_num + 1 ;
            /* we always want to allocate a little more space to prevent too many reallocs*/
            db->allocated_num += db->allocated_num >> 1 ;
            tmp = (ASDatabaseRecord*)safecalloc( db->allocated_num, sizeof(ASDatabaseRecord));
            if( db->styles_table && res > 0 )
                memcpy( tmp, db->styles_table, sizeof(ASDatabaseRecord)*res );
        }else   /* we can use the same memory */
            tmp = db->styles_table ;

        /* freeing up space by shifting elements ahead by 1 :*/
        if( db->styles_table )
        {
            register int k ;
            for( k = db->styles_num ; k > i ; k-- )
                memcpy( &(tmp[k]), &(db->styles_table[k-1]), sizeof(ASDatabaseRecord) );
            if( k < db->styles_num )
                tmp[k].magic = 0;              /* invalidating record */
        }

        if( db->styles_table != tmp )
        {
            if( db->styles_table != NULL )
                free( db->styles_table );
            db->styles_table = tmp ;
        }
    }
    return res ;
}

ASDatabase* build_asdb( name_list *nl )
{
    ASDatabase* db = NULL ;
    if( nl )  db = (ASDatabase*)safecalloc(1, sizeof(ASDatabase));

    while( nl )
    {
        wild_reg_exp *regexp ;

        if( (regexp = compile_wild_reg_exp( nl->name )) != NULL )
        {
            /* First check if that is the default style */
            if( strcmp( regexp->raw, "*" ) == 0 )
            {
                destroy_wild_reg_exp( regexp );
                replace_record( &(db->style_default), nl, NULL );
            }else
            {
                int where ;
                /* we have to find the place of this regexp among other
                 * in order of decreasing length
                 */
				where = place_new_record( db, regexp );
                if( where >= 0 )
                    if( replace_record( &(db->styles_table[where]), nl, regexp ))
                        db->styles_num++;
            }
        }
        nl = nl->next ;
    }
    if( db )
    {
        if( db->styles_num < db->allocated_num )
        {
            db->allocated_num = db->styles_num ;
            if( db->styles_num == 0 )
            {
                free( db->styles_table );
                db->styles_table = NULL ;
            }else
                db->styles_table = (ASDatabaseRecord*)realloc( db->styles_table,
                                                        db->allocated_num*sizeof(ASDatabaseRecord));
        }
        db->match_list = (int*) safecalloc ( db->styles_num+1, sizeof(int));
        db->match_list[0] = -1 ;
    }
    return db;
}

void destroy_asdb( ASDatabase **db )
{
    if( *db )
    {
        if( (*db)->styles_table )
        {
            register int i ;

            for( i = 0 ; i < (*db)->styles_num ; i++ )
                destroy_asdb_record( &((*db)->styles_table[i]), True );
            free( (*db)->styles_table );
        }
        destroy_asdb_record( &((*db)->style_default), True );
		free( (*db)->match_list );
        free( *db );
        *db = NULL ;
    }
}

/************************************************************************************
 * Printing functions
 ************************************************************************************/
void
print_asgeometry( stream_func func, void *stream, ASGeometry *g, const char *prompt1, const char *prompt2 )
{
    if(!pre_print_check( &func, &stream, g, NULL ) )
        return ;
    func( stream, "%s.%s.flags = 0x%lX;\n", prompt1, prompt2, g->flags );
    if( get_flags( g->flags, XValue ) )
        func( stream, "%s.%s.x = %d;\n", prompt1, prompt2, g->x );
    if( get_flags( g->flags, YValue ) )
        func( stream, "%s.%s.y = %d;\n", prompt1, prompt2, g->y );
    if( get_flags( g->flags, WidthValue ) )
        func( stream, "%s.%s.width = %d;\n", prompt1, prompt2, g->width );
    if( get_flags( g->flags, HeightValue ) )
        func( stream, "%s.%s.width = %d;\n", prompt1, prompt2, g->height );
}

void
print_asdb_record( stream_func func, void *stream, ASDatabaseRecord *db_rec, const char *prompt )
{
    register int i ;
    if( !pre_print_check( &func, &stream, db_rec, NULL ) )
        return ;
    if( db_rec->regexp )
        func( stream, "%s.regexp = \"%s\";\n", prompt, db_rec->regexp->raw );

    func( stream, "%s.set_flags = 0x%lX;\n", prompt, db_rec->set_flags );
    func( stream, "%s.flags = 0x%lX;\n", prompt, db_rec->flags     );
    if( get_flags( db_rec->set_flags, STYLE_BUTTONS ) )
	{
	    func( stream, "%s.set_buttons = 0x%lX;\n", prompt, db_rec->set_buttons);
  		func( stream, "%s.buttons = 0x%lX;\n", prompt, db_rec->buttons   );
	}

    if( get_flags( db_rec->set_flags, STYLE_DEFAULT_GEOMETRY ) )
        print_asgeometry( func, stream, &(db_rec->default_geometry), prompt, "default_geometry") ;

    if( get_flags( db_rec->set_flags, STYLE_STARTUP_DESK ) )
  		func( stream, "%s.desk = %d;\n", prompt, db_rec->desk        );
    if( get_flags( db_rec->set_flags, STYLE_LAYER ) )
	    func( stream, "%s.layer = %d;\n", prompt, db_rec->layer       );
    if( get_flags( db_rec->set_flags, STYLE_VIEWPORTY ) )
  		func( stream, "%s.viewport_y = %d;\n", prompt, db_rec->viewport_x  );
    if( get_flags( db_rec->set_flags, STYLE_VIEWPORTX ) )
  		func( stream, "%s.viewport_x = %d;\n", prompt, db_rec->viewport_y  );
    if( get_flags( db_rec->set_flags, STYLE_BORDER_WIDTH ) )
  		func( stream, "%s.border_width = %u;\n", prompt, db_rec->border_width);
    if( get_flags( db_rec->set_flags, STYLE_HANDLE_WIDTH ) )
  		func( stream, "%s.resize_width = %u;\n", prompt, db_rec->resize_width);
    if( get_flags( db_rec->set_flags, STYLE_GRAVITY ) )
  		func( stream, "%s.gravity = %u;\n", prompt, db_rec->gravity     );

    if( db_rec->icon_file )
        func( stream, "%s.icon_file = \"%s\";\n", prompt, db_rec->icon_file);
    if( db_rec->frame_name )
        func( stream, "%s.frame_name = \"%s\";\n", prompt, db_rec->frame_name);
    for( i = 0 ; i < BACK_STYLES ; i++ )
        if( db_rec->window_styles[i] )
            func( stream, "%s.MyStyle[%d] = \"%s\";\n", prompt, i, db_rec->window_styles[i]);

    func( stream, "%s.own_strings = %d;\n", prompt, db_rec->own_strings );
}

void
print_asdb_match_list( stream_func func, void *stream, ASDatabase *db )
{
    register int i ;

    if( !pre_print_check( &func, &stream, db, "ASDatabase unavailable(NULL)!" ) )
        return ;

    for( i = 0 ; db->match_list[i] >= 0 ; i++ )
    {
        if( db->styles_table[db->match_list[i]].regexp )
            func( stream, "ASDB.match_list[%d].regexp = \"%s\";\n", i, db->styles_table[db->match_list[i]].regexp->raw );
    }
}

void
print_asdb( stream_func func, void *stream, ASDatabase *db )
{
    char prompt[128] ;
    register int i ;

    if( !pre_print_check( &func, &stream, db, "ASDatabase unavailable(NULL)!" ) )
        return ;

    func( stream, "ASDB.allocated_num = %d;\n", db->allocated_num ) ;
    func( stream, "ASDB.styles_num = %d;\n", db->styles_num );

    for( i = 0 ; i < db->styles_num ; i++ )
    {
        sprintf( prompt, "ASDB.styles_table[%d]", i );
        print_asdb_record( func, stream, &(db->styles_table[i]), prompt );
    }
    print_asdb_record( func, stream, &(db->style_default), "ASDB.style_default" );
    print_asdb_match_list( func, stream, db );
}

void
print_asdb_matched_rec( stream_func func, void *stream, ASDatabase *db, ASDatabaseRecord *db_rec )
{
    if( db == NULL ||
        !pre_print_check( &func, &stream, db, "No matches!" ) )
        return ;
    print_asdb_record( func, stream, db_rec, "MATCHED_RECORD" );
    print_asdb_match_list( func, stream, db );
}
