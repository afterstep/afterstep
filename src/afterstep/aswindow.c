/*
 * Copyright (c) 2000 Sasha Vasko <sashav@sprintmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
[21~ * (at your option) any later version.
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

#include "../../include/asapp.h"
#include "../../include/afterstep.h"
#include "../../include/hints.h"
#include "../../include/screen.h"
#include "../../include/decor.h"
#include "../../include/module.h"
#include "asinternals.h"

Bool on_dead_aswindow( Window w, struct ASWindowList *list );
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

    list->circulate_list = create_asvector( sizeof(ASWindow*) );
    list->sticky_list = create_asvector( sizeof(ASWindow*) );

//    list->root = safecalloc (0, sizeof (ASWindow ));
//    list->root->w = Scr.Root ;
//    add_hash_item( list->clients, (ASHashableValue)Scr.Root, list->root );

    list->on_dead_window = on_dead_aswindow ;

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

    swad.f = fopen (realfilename, "w+");
    free (realfilename);

    if ( swad.f == NULL)
        show_error( "Unable to save your session into the %s - cannot open file for writing!", file);
    else
    {
        iterate_asbidirlist( list->clients, make_aswindow_cmd_iter_func, &swad, NULL, False );
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


Bool on_dead_aswindow( Window w, struct ASWindowList *list )
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
        ASLayer  *dst_layer = get_aslayer( ASWIN_LAYER(asw), Scr.Windows );
        /* inserting window into the top of the new layer */
	if( !AS_ASSERT(dst_layer) )
	    vector_insert_elem( dst_layer->members, &asw, 1, NULL, False );
    }
}

void
remove_aswindow_from_layer( ASWindow *asw, int layer )
{
    if( !AS_ASSERT(asw) )
    {
        ASLayer  *src_layer = get_aslayer( layer, Scr.Windows );
	LOCAL_DEBUG_OUT( "removing window %p from layer %p", src_layer, asw );
        if( !AS_ASSERT(src_layer) )
	{
LOCAL_DEBUG_OUT( "can be found at index %d", vector_find_data(	src_layer->members, &asw ) );
	    while( vector_find_data( src_layer->members, &asw ) < src_layer->members->used )
	    {
    	        vector_remove_elem( src_layer->members, &asw );
		LOCAL_DEBUG_OUT( "after delition can be found at index %d", vector_find_data(	src_layer->members, &asw ) );	
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

    tie_aswindow( t );
    if (!get_flags(t->hints->flags, AS_SkipWinList))
        update_windowList ();
    return True;
}

void
delist_aswindow( ASWindow *t )
{
    Bool skip_winlist;
    if( Scr.Windows == NULL )
        return ;

    /* set desktop for window */
    if( t->w != Scr.Root )
        vector_remove_elem( Scr.Windows->circulate_list, &t );

    if( ASWIN_GET_FLAGS(t, AS_Sticky ) )
        vector_remove_elem( Scr.Windows->sticky_list, &t );

    remove_aswindow_from_layer( t, ASWIN_LAYER(t) );

    untie_aswindow( t );
    skip_winlist = get_flags(t->hints->flags, AS_SkipWinList);
    discard_bidirelem( Scr.Windows->clients, t );

    if (!skip_winlist)
        update_windowList ();

}

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
        int r ;

        if( ASWIN_GET_FLAGS( asws[i], AS_Iconic ) )
        {
            frame = client = asws[i]->icon_canvas ;
            if( frame == NULL )
                continue ;
        }else if( ASWIN_GET_FLAGS( asws[i], AS_Shaded ) )
            client = frame ;

        ASWIN_CLEAR_FLAGS( asws[0], AS_Visible );

        r = VECTOR_USED(*rects);
        while( --r >= 0 )
        {
            if( client->root_x+(int)client->width > vrect[r].x &&
                client->root_x < vrect[r].x+(int)vrect[r].width &&
                client->root_y+(int)client->height > vrect[r].y &&
                client->root_y < vrect[r].y+(int)vrect[r].height )
            {
                ASWIN_SET_FLAGS( asws[0], AS_Visible );

                break;
            }
        }
    }
}

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
    if( Scr.Windows->clients->count > ids->allocated )
        realloc_vector( ids, Scr.Windows->clients->count );

    if( (layers_in = sort_hash_items (Scr.Windows->layers, NULL, (void**)VECTOR_HEAD_RAW(*layers), 0)) == 0 )
        return ;

    l = VECTOR_HEAD(ASLayer*,*layers);
    windows = VECTOR_HEAD(Window,*ids);
    windows_num = 0 ;
    LOCAL_DEBUG_OUT( "filling up array with window IDs: layers_in = %ld, Total clients = %d", layers_in, Scr.Windows->clients->count );
    for( i = 0 ; i < layers_in ; i++ )
    {
        register int k, end_k = VECTOR_USED(*(l[i]->members)) ;
        ASWindow **members = VECTOR_HEAD(ASWindow*,*(l[i]->members));
        if( end_k > ids->allocated )
            end_k = ids->allocated ;
        LOCAL_DEBUG_OUT( "layer %ld, end_k = %d", i, end_k );
        for( k = 0 ; k < end_k ; k++ )
	{
	    register ASWindow *asw = members[k] ;
            if( ASWIN_DESK(asw) == desk )
		if( !ASWIN_GET_FLAGS(asw, AS_Dead) )
            	    windows[windows_num++] = asw->w;
	}
    }
    LOCAL_DEBUG_OUT( "Sending stacking order: windows_num = %ld, ", windows_num );
    VECTOR_USED(*ids) = windows_num ;
    SendStackingOrder (-1, M_STACKING_ORDER, desk, ids);

    if( !send_msg_only )
    {
        l = VECTOR_HEAD(ASLayer*,*layers);
        windows = VECTOR_HEAD(Window,*ids);
        windows_num = 0 ;
        LOCAL_DEBUG_OUT( "filling up array with window frame IDs: layers_in = %ld, ", layers_in );
        for( i = 0 ; i < layers_in ; i++ )
        {
            register int k, end_k = VECTOR_USED(*(l[i]->members)) ;
            register ASWindow **members = VECTOR_HEAD(ASWindow*,*(l[i]->members));
            if( end_k > ids->allocated )
                end_k = ids->allocated ;
            LOCAL_DEBUG_OUT( "layer %ld, end_k = %d", i, end_k );
            for( k = 0 ; k < end_k ; k++ )
                if( ASWIN_DESK(members[k]) == desk && !ASWIN_GET_FLAGS(members[k], AS_Dead))
                    windows[windows_num++] = get_window_frame(members[k]);
        }

        LOCAL_DEBUG_OUT( "Setting stacking order: windows_num = %ld, ", windows_num );
        if( windows_num > 0 )
        {
            XRaiseWindow( dpy, windows[0] );
            if( windows_num > 1 )
                XRestackWindows( dpy, windows, windows_num );
            XSync(dpy, False);
        }
        raise_scren_panframes (&Scr);
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

    return (above->root_x < below->root_x + below->width && above->root_x + above->width > below->root_x &&
            above->root_y < below->root_y + below->height && above->root_y + above->height > below->root_y);
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
				return False;
            else if( ASWIN_DESK(t) == ASWIN_DESK(below) )
                if (IS_OVERLAPING(t,below))
					return True;
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

    if( src_layer )
        vector_remove_elem( src_layer->members, &t );
    if( dst_layer )
        vector_insert_elem( dst_layer->members, &t, 1, sibling, above );

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
            !(ASWIN_DESK(clients[i]) != Scr.CurrentDesk && get_flags(Scr.Feel.flags,AutoTabThroughDesks )))
        {
            return clients[i];
        }
        i += dir ;
    }while(1);
    return NULL;
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



