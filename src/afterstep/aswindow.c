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

#include "../../configure.h"

#include <stdio.h>

#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif /* SHAPE */

#include "../../include/aftersteplib.h"
#include "../../include/afterstep.h"
#include "../../include/clientprops.h"
#include "../../include/hints.h"
#include "../../include/screen.h"
#include "../../include/module.h"
#include "../../include/parser.h"
#include "../../include/confdefs.h"
#include "asinternals.h"

/********************************************************************************/
/* window list management */

void auto_destroy_aswindow ( void *data )
{}

void destroy_aslayer  (ASHashableValue value, void *data);

ASWindowList *
init_aswindow_list()
{
    ASWindowList *list ;

    list = safecalloc( 1, sizeof(ASWindowList) );
    list->clients = create_asbidirlist(auto_destroy_aswindow);
    list->aswindow_xref = create_ashash( 0, NULL, NULL, NULL );
    list->layers = create_ashash( 7, NULL, desc_long_compare_func, destroy_aslayer );

    list->circulate_list = create_asvector( sizeof(ASWindow*) );
    list->sticky_list = create_asvector( sizeof(ASWindow*) );

//    list->root = safecalloc (0, sizeof (ASWindow ));
//    list->root->w = Scr.Root ;
//    add_hash_item( list->clients, (ASHashableValue)Scr.Root, list->root );

    return list;
}

struct SaveWindowAuxData
{
    char  this_host[MAXHOSTNAME];
    FILE *f;
};

Bool
make_aswindow_cmd_iter_func(void *data, void *aux_data)
{
    struct SaveWindowAuxData *swad = (struct SaveWindowAuxData *)aux_data ;
    ASWindow *asw = (ASWindow*)data ;
    if( asw && swad )
    {
        if( asw->hints->client_host && asw->hints->client_cmd )
            if( mystrcasecmp( asw->hints->client_host, swad->this_host ) == 0 )
            {
                register char *cmd ;
                if( (cmd = make_client_command( &Scr, asw->hints, asw->status, &(asw->anchor), Scr.Vx, Scr.Vy )) != NULL )
                {
                    fprintf( swad->f, "%s &\n", cmd );
                    free( cmd );
                }
            }
        return True;
    }
    return False;
}

void
save_aswindow_list( ASWindowList *list, char *file )
{
    char *realfilename ;
    struct SaveWindowAuxData swad ;

    if( list == NULL )
        return ;

    if (!mygethostname (swad.this_host, MAXHOSTNAME))
	{
        show_error ("Could not get HOST environment variable!");
		return;
	}

    if( (realfilename = PutHome( file )) == NULL )
        return;

    if ( (swad.f = fopen (realfilename, "w+")) == NULL)
	{
        free (realfilename);
        show_error( "Unable to save your session into the %s - cannot open file for writing!", file);;
		return;
	}

    iterate_asbidirlist( list->clients, make_aswindow_cmd_iter_func, &swad, NULL, False );
    fclose( swad.f );
}

void
destroy_aswindow_list( ASWindowList **list, Bool restore_root )
{
    if( list )
        if( *list )
        {
            if( restore_root )
                InstallRootColormap ();

            destroy_asvector(&((*list)->sticky_list));
            destroy_asvector(&((*list)->circulate_list));
            destroy_ashash(&((*list)->layers));
            destroy_ashash(&((*list)->aswindow_xref));
            destroy_asbidirlist( &((*list)->clients ));
            free(*list);
            *list = NULL ;
        }
}

/*************************************************************************
 * We maintain crossreference of X Window ID to ASWindow structure - that is
 * faster then using XContext since we don't have to worry about multiprocessing,
 * thus saving time on interprocess synchronization, that Xlib has to do in
 * order to access list of window contexts.
 *************************************************************************/
ASWindow *window2ASWindow( Window w )
{
    ASWindow *asw = NULL ;
    if( Scr.Windows->aswindow_xref )
        if( get_hash_item( Scr.Windows->aswindow_xref, AS_HASHABLE(w), (void**)&asw ) == ASH_Success )
            return asw;
    return asw;
}

Bool register_aswindow( Window w, ASWindow *asw )
{
    if( w && asw )
    {
        if( Scr.Windows->aswindow_xref == NULL )


        if( add_hash_item( Scr.Windows->aswindow_xref, AS_HASHABLE(w), asw ) == ASH_Success )
            return True;
    }
    return False;
}

Bool unregister_aswindow( Window w )
{
    if( w )
    {
        if( Scr.Windows->aswindow_xref != NULL )
		{
            if( remove_hash_item( Scr.Windows->aswindow_xref, AS_HASHABLE(w), NULL, False ) == ASH_Success )
  		        return True;
		}
    }
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

ASWindow *
pattern2ASWindow( const char *pattern )
{
    wild_reg_exp *wrexp = compile_wild_reg_exp( pattern );

    if( wrexp )
    {
        ASBiDirElem *e = LIST_START(Scr.Windows->clients) ;
        while( e != NULL )
        {
            ASWindow *curr = (ASWindow*)LISTELEM_DATA(e);
            if( match_string_list (curr->hints->names, MAX_WINDOW_NAMES, wrexp) == 0 )
            {
                destroy_wild_reg_exp( wrexp );
                return curr;
            }
            LIST_GOTO_NEXT(e);
        }
    }
    destroy_wild_reg_exp( wrexp );
    return NULL;
}


/*******************************************************************************/
/* layer management */

ASLayer *
get_aslayer( int layer, ASWindowList *list )
{
    ASLayer *l = NULL ;
    if( list )
    {
        ASHashableValue hlayer = AS_HASHABLE(layer);
        if( get_hash_item( list->layers, hlayer, (void**)&l ) != ASH_Success )
        {
            l = safecalloc( 1, sizeof(ASLayer));
            if( add_hash_item( list->layers, hlayer, l ) == ASH_Success )
            {
                l->members = create_asvector( sizeof(ASWindow*) );
                l->layer = layer ;
            }else
            {
                free( l );
                l = NULL ;
            }
        }
    }
    return l;
}

void
destroy_aslayer  (ASHashableValue value, void *data)
{
    if( data )
    {
        ASLayer *l = data ;
        destroy_asvector( &(l->members));
        free( data );
    }
}
/********************************************************************************/
/* ASWindow management */

Bool
enlist_aswindow( ASWindow *t )
{
    if( Scr.Windows == NULL )
        Scr.Windows = init_aswindow_list();

    append_bidirelem( Scr.Windows->clients, t );
    vector_insert_elem( Scr.Windows->circulate_list, &t, 1, NULL, True );

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
    if (!get_flags(t->hints->flags, AS_SkipWinList))
        update_windowList ();
    return True;
}

void
delist_aswindow( ASWindow *t )
{
    ASLayer *l;
    if( Scr.Windows == NULL )
        return ;

    discard_bidirelem( Scr.Windows->clients, t );

    /* set desktop for window */
    if( t->w != Scr.Root )
        vector_remove_elem( Scr.Windows->circulate_list, &t );

    if( ASWIN_GET_FLAGS(t, AS_Sticky ) )
        vector_remove_elem( Scr.Windows->sticky_list, &t );

    if((l = get_aslayer(ASWIN_LAYER(t), Scr.Windows )) != NULL )
        vector_remove_elem( l->members, &t );

    if( t->transient_owner != None )
    {
        vector_remove_elem( t->transient_owner->transients, &t );
		if( t->transient_owner->transients->used == 0 )
			destroy_asvector( &(t->transient_owner->transients) );
    }
    if( t->hints->group_lead != None )
    {
        vector_remove_elem( t->group_lead->group_members, &t );
		if( t->group_lead->group_members->used == 0 )
			destroy_asvector( &(t->group_lead->group_members) );
    }
    if (!get_flags(t->hints->flags, AS_SkipWinList))
        update_windowList ();
}

void
restack_window_list( int desk )
{
    static ASVector *ids = NULL ;
    static ASVector *layers = NULL ;
    unsigned long layers_in, i ;
    register long windows_num = 0 ;
    ASLayer *l ;
    Window  *windows ;

    if( layers == NULL )
        layers = create_asvector( sizeof(ASLayer*) );
    if( Scr.Windows->layers->items_num > layers->allocated )
        realloc_vector( layers, Scr.Windows->layers->items_num );

    if( ids == NULL )
        ids = create_asvector( sizeof(Window) );
    else
        flush_vector( ids );
    if( Scr.Windows->clients->count > layers->allocated )
        realloc_vector( layers, Scr.Windows->clients->count );

    if( (layers_in = sort_hash_items (Scr.Windows->layers, NULL, (void**)VECTOR_HEAD_RAW(*layers), 0)) == 0 )
        return ;
    l = VECTOR_HEAD(ASLayer,*layers);
    windows = VECTOR_HEAD(Window,*ids);
    for( i = 0 ; i < layers_in ; i++ )
    {
        register int k, end_k = VECTOR_USED(*(l[i].members)) ;
        register ASWindow **members = VECTOR_HEAD(ASWindow*,*(l[i].members));
        if( end_k > ids->allocated )
            end_k = ids->allocated ;
        for( k = 0 ; k < end_k ; k++ )
            if( ASWIN_DESK(members[k]) == desk )
                windows[windows_num++] = get_window_frame(members[k]);
    }
    if( windows_num > 0 )
    {
        XRaiseWindow( dpy, windows[0] );
        XRestackWindows( dpy, windows, windows_num );
        XSync(dpy, False);
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

Bool
is_window_obscured (ASWindow * above, ASWindow * below)
{
    ASLayer           *l ;
    ASWindow **members ;

	if (above != NULL && below != NULL)
        return is_status_overlaping (above->status, below->status);

	if (above == NULL && below != NULL)
    {/* checking if window "below" is completely obscured by any of the
        windows with the same layer above it in stacking order */
        register int i, end_i ;

        l = get_aslayer( ASWIN_LAYER(below), Scr.Windows );
        end_i = l->members->used ;
        members = VECTOR_HEAD(ASWindow*,*(l->members));
        for (i = 0 ; i < end_i ; i++ )
        {
            register ASWindow *t ;
            if( (t = members[i]) == below )
				return False;
            else if( ASWIN_DESK(t) == ASWIN_DESK(below) )
                if (is_status_overlaping (t->status, below->status))
					return True;
        }
    }else if (above != NULL )
    {   /* checking if window "above" is completely obscuring any of the
           windows with the same layer below it in stacking order */
        register int i ;

        l = get_aslayer( ASWIN_LAYER(above), Scr.Windows );
        members = VECTOR_HEAD(ASWindow*,*(l->members));
        for (i = VECTOR_USED(*(l->members))-1 ; i >= 0 ; i-- )
        {
            register ASWindow *t ;
            if( (t = members[i]) == above )
				return False;
            else if( ASWIN_DESK(t) == ASWIN_DESK(above) )
                if (is_status_overlaping (above->status, t->status) )
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
        if (is_window_obscured (sibling, t))
			occlusion = OCCLUSION_BELOW;
	} else if (stack_mode == BottomIf)
	{
        if (is_window_obscured (t, sibling))
			occlusion = OCCLUSION_ABOVE;
	} else if (stack_mode == Opposite)
	{
        if (is_window_obscured (sibling, t))
			occlusion = OCCLUSION_BELOW;
        else if (is_window_obscured (t, sibling))
			occlusion = OCCLUSION_ABOVE;
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

    vector_remove_elem( src_layer->members, &t );
    vector_insert_elem( dst_layer->members, &t, 1, sibling, above );

	restack_window_list( ASWIN_DESK(t) );
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
//#warning TODO: implement window circulation
	return curr_win;

}

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
    register int dir = backwards ? 1 : -1 ;
    int end_i;
    ASWindow **clients;
    ASWindow *next = NULL ;
    int loop_count = 0 ;

    if( list == NULL ) return NULL;

	end_i = VECTOR_USED(*(list->circulate_list)) ;
	clients = VECTOR_HEAD(ASWindow*,*(list->circulate_list));

    if( list->warp_curr_index < 0 )
    { /* need to initialize warping : */
        list->warp_curr_index = (dir > 0)? 0 : end_i ;
        list->warp_user_dir = dir ;
        list->warp_init_dir = dir ;
        list->warp_curr_dir = dir ;
    }else if( dir == list->warp_user_dir )
    {
            if( Scr.AutoReverse == AST_ClosedLoop )
                dir = list->warp_curr_dir ;
    }else
    {
        list->warp_user_dir = dir ;
        /* user reversed direction - so do we : */
        dir = (list->warp_curr_dir > 0)? -1 : 1 ;
        list->warp_curr_dir = dir ;
    }

    for( i = list->warp_curr_index+dir ; loop_count < 2 ; i+=dir )
    {

        if( 0 > i || i >= end_i )
        {
            if( Scr.AutoReverse == AST_OpenLoop )
                i = (i <= 0)? end_i : 0 ;
            else if( Scr.AutoReverse == AST_ClosedLoop )
                list->warp_curr_dir = dir = (dir < 0 )? 1 : -1 ;
            else
                return NULL;
            loop_count++ ;
            continue;
        }

        if( !(ASWIN_HFLAGS(clients[i], AS_DontCirculate)) &&
            !(ASWIN_GET_FLAGS(clients[i], AS_Iconic) && get_flags(Scr.flags, CirculateSkipIcons)) &&
            !(ASWIN_DESK(clients[i]) != Scr.CurrentDesk && get_flags(Scr.flags,AutoTabThroughDesks )))
        {
            next = clients[i] ;
            break;
        }
    }
    if( next != NULL )
        list->warp_curr_index = i ;
    return next;
}

/********************************************************************************/
/* Placement management */
/*
 * There are several reasons why window has to be moved/resized :
 * Client actions :
 * 1) Initial Placement :
 *      - we take initial placement and calculate anchor based on it.
 *      - additionally we may choose to let user interactively place window ( see below )
 * 2) ConfigureRequest :
 *      - if new position is requested, or new border width is requested -
 *        we recalculate the anchor point.
 *      - if new width is requested - we move/resize window around anchor point.
 * AfterStep actions :
 * 3) Maximize-ing :
 *      - we have to resize/move frame to requested size, and save original position
 *        for the future possible restartion to the original size.
 * 4) UnMaximize-ing
 *      - we have to restore window size/placement from previously saved data.
 * 4) Moving :
 *      - we are moving frame to the requested position.
 * 5) Resizing :
 *      - we are resizing frame to the specified size around the anchor point.
 * 6) look changed and decorartions size changed as well
 *      - we are resizing frame to the new size around the anchor point.
 */
Bool
place_window( ASWindow *t, ASStatusHints *status )
{

//#warning TODO: implement window placement
	return True ;
}



