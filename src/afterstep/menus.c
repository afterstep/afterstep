/*
 * Copyright (c) 2002 Sasha Vasko
 * module has been entirely rewritten to use ASCanvas from original menus.c
 * that was implemented by :
 *      Copyright (C) 1993 Rob Nation
 *      Copyright (C) 1995 Bo Yang
 *      Copyright (C) 1996 Frank Fejes
 *      Copyright (C) 1996 Alfredo Kojima
 *      Copyright (C) 1998 Guylhem Aznar
 *      Copyright (C) 1998, 1999 Ethan Fischer
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
 *
 */

/***********************************************************************
 * afterstep menu code
 ***********************************************************************/

#define LOCAL_DEBUG

#include "../../configure.h"

#include "asinternals.h"
#include "menus.h"


#include <X11/keysym.h>

#define START_LONG_DRAW_OPERATION   XGrabServer(dpy)
#define STOP_LONG_DRAW_OPERATION    XUngrabServer(dpy)


static ASMenu *ASTopmostMenu = NULL;


ASHints *make_menu_hints( ASMenu *menu );
/*************************************************************************/
/* low level ASMenu functionality :                                      */
/*************************************************************************/
static Window
make_menu_window( Window parent )
{
    Window w;
    XSetWindowAttributes attr ;
    if( parent == None )
        parent = Scr.Root ;

    attr.event_mask = AS_MENU_EVENT_MASK ;
    w = create_visual_window( Scr.asv, parent, -10, -10, 1, 1, 0, InputOutput, CWEventMask, &attr );

    if( w && parent != Scr.Root )
        XMapRaised( dpy, w );
    return w;
}


ASMenu *
create_asmenu( const char *name)
{
    ASMenu *menu = safecalloc( 1, sizeof(ASMenu) );
    Window w ;
    menu->magic = MAGIC_ASMENU ;
    w = make_menu_window( None );
    menu->main_canvas = create_ascanvas( w );
    menu->name = mystrdup(name);
	menu->scroll_up_bar = create_astbar();
	menu->scroll_down_bar = create_astbar();
    return menu;
}

static void
free_asmenu_item( ASMenuItem *item )
{
    if( item->bar )
        destroy_astbar(&(item->bar) );
    item->submenu = NULL ;
}

void destroy_asmenu(ASMenu **pmenu);

static inline void
close_asmenu_submenu( ASMenu *menu)
{
LOCAL_DEBUG_CALLER_OUT( "top(%p)->supermenu(%p)->menu(%p)->submenu(%p)", ASTopmostMenu, menu->supermenu, menu, menu->submenu );
    if( menu->submenu )
    {
		if( menu->submenu->supermenu == menu )
			menu->submenu->supermenu = NULL ;
        if( menu->submenu->owner )
        {
            /* cannot use Destroy directly - must go through the normal channel: */
            unmap_canvas_window( menu->submenu->main_canvas );
            ASWIN_CLEAR_FLAGS(menu->submenu->owner, AS_Visible);
            ASWIN_SET_FLAGS(menu->submenu->owner, AS_UnMapPending);
			menu->submenu = NULL ;
        }else
            destroy_asmenu( &(menu->submenu));
    }
}

void
destroy_asmenu(ASMenu **pmenu)
{
    if( pmenu )
    {
        ASMenu *menu = *pmenu;
        if( menu && menu->magic == MAGIC_ASMENU )
        {
            Window w = menu->main_canvas->w ;
LOCAL_DEBUG_CALLER_OUT( "top(%p)->supermenu(%p)->menu(%p)->submenu(%p)", ASTopmostMenu, menu->supermenu, menu, menu->submenu );

            if( menu->supermenu && menu->supermenu->submenu == menu )
                menu->supermenu->submenu = NULL ;
            else if( ASTopmostMenu == menu )
                ASTopmostMenu = NULL ;

            close_asmenu_submenu( menu);

            if( menu->main_canvas )
                destroy_ascanvas( &(menu->main_canvas) );
            if( menu->owner )
            {
                ASWIN_SET_FLAGS(menu->owner, AS_Dead );
                ASWIN_CLEAR_FLAGS(menu->owner, AS_Visible);
                ASWIN_SET_FLAGS(menu->owner, AS_UnMapPending);
            }
            destroy_registered_window( w );

            if( menu->items )
            {
                register int i = menu->items_num;
                while ( --i >= 0 )
                    free_asmenu_item( &(menu->items[i]));
                free( menu->items );
                menu->items = NULL ;
            }

			if( menu->scroll_up_bar )
				destroy_astbar(&(menu->scroll_up_bar ));
			if( menu->scroll_down_bar )
				destroy_astbar( &(menu->scroll_down_bar));

            if( menu->name )
                free( menu->name );
            if( menu->title )
                free( menu->title );
            menu->magic = 0 ;
            free( menu );
            *pmenu = NULL ;
        }
    }
}

void
close_asmenu( ASMenu **pmenu)
{
    if( pmenu )
    {
        ASMenu *menu = *pmenu ;
        if( menu )
        {
LOCAL_DEBUG_CALLER_OUT( "top(%p)->supermenu(%p)->menu(%p)->submenu(%p)", ASTopmostMenu, menu->supermenu, menu, menu->submenu );
		    if( menu->submenu )
			{
				if( menu->submenu->supermenu == menu )
					menu->submenu->supermenu = NULL ;
				close_asmenu( &(menu->submenu) );
  			}
            if( menu->owner )
            {
                /* cannot use Destroy directly - must go through the normal channel: */
                unmap_canvas_window( menu->main_canvas );
                ASWIN_CLEAR_FLAGS(menu->owner, AS_Visible);
                ASWIN_SET_FLAGS(menu->owner, AS_UnMapPending);
            }else
                destroy_asmenu( &(menu));
            *pmenu = NULL ;
        }
    }
}

static void
set_asmenu_item_data( ASMenuItem *item, MenuDataItem *mdi )
{
    ASImage *icon_im = NULL ;

    item->source = mdi ;

    if( item->bar == NULL )
        item->bar = create_astbar();
	else
		delete_astbar_tile( item->bar, -1 );  /* delete all tiles */


    if( item->icon )
    {
        safe_asimage_destroy( item->icon );
        item->icon = NULL ;
    }
    if( mdi->minipixmap_image )
        icon_im = mdi->minipixmap_image ;
    else if( mdi->minipixmap )
        icon_im = GetASImageFromFile( mdi->minipixmap );
    if( icon_im )
    {
        int w = icon_im->width ;
        int h = icon_im->height ;
        if( w > h )
        {
            if( w > MAX_MENU_ITEM_HEIGHT )
            {
                w = MAX_MENU_ITEM_HEIGHT ;
                h = (h * w)/icon_im->width ;
                if( h == 0 )
                    h = 1 ;
            }
        }else if( h > MAX_MENU_ITEM_HEIGHT )
        {
            h = MAX_MENU_ITEM_HEIGHT ;
            w = (w * h)/icon_im->height ;
            if( w == 0 )
                w = 1 ;
        }
        if( w != icon_im->width || h != icon_im->height )
        {
            item->icon = scale_asimage( Scr.asv, icon_im, w, h, ASA_ASImage, 100, ASIMAGE_QUALITY_DEFAULT );
            if( icon_im != mdi->minipixmap_image )
                safe_asimage_destroy( icon_im );
        }else if( icon_im == mdi->minipixmap_image )
            item->icon = dup_asimage( icon_im );
        else
            item->icon = icon_im ;
    }
LOCAL_DEBUG_OUT( "item(\"%s\")->minipixmap(\"%s\")->icon(%p)", mdi->item, mdi->minipixmap?mdi->minipixmap:NULL, item->icon );

    /* reserve space for minipixmap */
#define MI_LEFT_SPACER_IDX  0
    add_astbar_spacer( item->bar, 0, 0, 0, NO_ALIGN, 1, 1 );              /*0*/
#define MI_LEFT_ARROW_IDX   1
    add_astbar_spacer( item->bar, 1, 0, 0, NO_ALIGN, 1, 1 );              /*1*/
#define MI_LEFT_ICON_IDX    2
    add_astbar_spacer( item->bar, 2, 0, 0, NO_ALIGN, 1, 1 );              /*2*/
    /* reserve space for popup icon :*/
#define MI_POPUP_IDX        3
    add_astbar_spacer( item->bar, 7, 0, 0, NO_ALIGN, 1, 1 );              /*3*/

    /* optional menu items : */
    /* add label */
    if( mdi->item )
        add_astbar_label( item->bar, 3, 0, 0, ALIGN_LEFT|ALIGN_VCENTER, 0, 0, mdi->item, AS_Text_ASCII );
    /* add hotkey */
    if( mdi->item2 )
        add_astbar_label( item->bar, 4, 0, 0, ALIGN_RIGHT|ALIGN_VCENTER, 0, 0, mdi->item2, AS_Text_ASCII );

    item->flags = 0 ;
    if( mdi->fdata->func == F_POPUP )
    {
        if( (item->submenu = FindPopup (mdi->fdata->text, True)) == NULL )
            set_flags( item->flags, AS_MenuItemDisabled );
    }else if( get_flags( mdi->flags, MD_Disabled ) || mdi->fdata->func == F_NOP )
        set_flags( item->flags, AS_MenuItemDisabled );

    item->fdata = mdi->fdata ;
}

static Bool
set_asmenu_item_look( ASMenuItem *item, MyLook *look, unsigned int icon_space, unsigned int arrow_space )
{
    ASFlagType hilite = NO_HILITE, fhilite = NO_HILITE ;
    int subitem_offset = 1 ;
LOCAL_DEBUG_OUT( "item.bar(%p)->look(%p)", item->bar, look );

    if( item->bar == NULL )
        return False;

    item->bar->h_spacing = DEFAULT_MENU_SPACING ;
    item->bar->h_border = DEFAULT_MENU_ITEM_HBORDER ;
    item->bar->v_border = DEFAULT_MENU_ITEM_VBORDER ;

    delete_astbar_tile( item->bar, MI_LEFT_SPACER_IDX );
    if(get_flags( item->flags, AS_MenuItemSubitem ))
    {
        subitem_offset = icon_space+2+item->bar->h_spacing+item->bar->h_spacing;
        add_astbar_spacer( item->bar, 0, 0, 0, NO_ALIGN, subitem_offset, 1 );
    }
    delete_astbar_tile( item->bar, MI_LEFT_ARROW_IDX );
#if 1
    if(get_flags( item->flags, AS_MenuItemSubitem ))
    {
        if( look->MenuArrow )
    	    add_astbar_icon( item->bar, 1, 0, 0, ALIGN_VCENTER, look->MenuArrow->image );
        else
            add_astbar_spacer( item->bar, 1, 0, 0, NO_ALIGN, arrow_space, 1 );
    }
#endif
//        add_astbar_spacer( item->bar, 1, 0, 0, NO_ALIGN, 1, 1 );

    if( get_flags( look->flags, MenuMiniPixmaps ) && icon_space > 0 )
    {
        //set_astbar_tile_size( item->bar, MI_LEFT_SPACER_IDX, icon_space, 1 );
        delete_astbar_tile( item->bar, MI_LEFT_ICON_IDX );
        /* now readd it as minipixmap :*/
        if( item->icon )
            add_astbar_icon( item->bar, 2, 0, 0, ALIGN_VCENTER, item->icon );
        else
            add_astbar_spacer( item->bar, 2, 0, 0, NO_ALIGN, icon_space, 1 );
    }

    /* delete tile from Popup arrow cell : */
    delete_astbar_tile( item->bar, MI_POPUP_IDX );
    /* now readd it as proper type : */
    if( look->MenuArrow && item->fdata->func == F_POPUP )
        add_astbar_icon( item->bar, 7, 0, 0, ALIGN_VCENTER, look->MenuArrow->image );
    else
        add_astbar_spacer( item->bar, 7, 0, 0, NO_ALIGN, arrow_space, 1 );


    if( get_flags( item->flags, AS_MenuItemDisabled ) )
    {
        set_astbar_style_ptr( item->bar, BAR_STATE_UNFOCUSED, look->MSMenu[MENU_BACK_STIPPLE]);
        set_astbar_style_ptr( item->bar, BAR_STATE_FOCUSED, look->MSMenu[MENU_BACK_STIPPLE] );
    }else
    {
        set_astbar_style_ptr( item->bar, BAR_STATE_UNFOCUSED,
                              look->MSMenu[get_flags( item->flags, AS_MenuItemSubitem )?MENU_BACK_SUBITEM:
                                                                                        MENU_BACK_ITEM] );
        set_astbar_style_ptr( item->bar, BAR_STATE_FOCUSED, look->MSMenu[MENU_BACK_HILITE] );
    }
    if( look->DrawMenuBorders == DRAW_MENU_BORDERS_ITEM )
        fhilite = hilite = DEFAULT_MENU_HILITE;
    else if ( Scr.Look.DrawMenuBorders == DRAW_MENU_BORDERS_OVERALL )
    {
        hilite = NO_HILITE_OUTLINE|LEFT_HILITE|RIGHT_HILITE ;
        if( get_flags( item->flags, AS_MenuItemFirst ) )
            hilite |= TOP_HILITE ;
        if( get_flags( item->flags, AS_MenuItemLast ) )
            hilite |= BOTTOM_HILITE ;
        fhilite = hilite ;
    }else if( look->DrawMenuBorders == DRAW_MENU_BORDERS_FOCUSED_ITEM )
        fhilite = DEFAULT_MENU_HILITE;
    else if ( Scr.Look.DrawMenuBorders == DRAW_MENU_BORDERS_O_AND_F )
    {
        hilite = NO_HILITE_OUTLINE|LEFT_HILITE|RIGHT_HILITE ;
        if( get_flags( item->flags, AS_MenuItemFirst ) )
            hilite |= TOP_HILITE ;
        if( get_flags( item->flags, AS_MenuItemLast ) )
            hilite |= BOTTOM_HILITE ;
        fhilite = DEFAULT_MENU_HILITE ;
    }

    set_astbar_hilite( item->bar, BAR_STATE_UNFOCUSED, hilite );
    set_astbar_hilite( item->bar, BAR_STATE_FOCUSED, fhilite );

    if( get_flags( item->flags, AS_MenuItemDisabled ) )
    {
        set_astbar_composition_method( item->bar, BAR_STATE_UNFOCUSED, Scr.Look.menu_scm );
        set_astbar_composition_method( item->bar, BAR_STATE_FOCUSED, Scr.Look.menu_scm );
    }else
    {
        set_astbar_composition_method( item->bar, BAR_STATE_UNFOCUSED, Scr.Look.menu_icm );
        set_astbar_composition_method( item->bar, BAR_STATE_FOCUSED, Scr.Look.menu_hcm );
    }


    return True;
}

static void
render_asmenu_bars( ASMenu *menu, Bool force )
{
    int i = menu->items_num ;
    Bool rendered = False;
    if( menu->main_canvas->height > 1 && menu->main_canvas->width > 1 )
    {
        START_LONG_DRAW_OPERATION;
        for( i = 0 ; i < menu->items_num ; ++i )
        {
            int prev_y = -10000 ;
            register ASTBarData *bar = menu->items[i].bar ;
            if( bar->win_y >= (int)(menu->main_canvas->height) )
                continue;

/*			update_astbar_transparency (bar, menu->main_canvas, False); */

			LOCAL_DEBUG_OUT( "bar %p needs_rendering? %s, visible? %s, lower ? %s", bar,
							 DoesBarNeedsRendering(bar)?"yes":"no",
							 (bar->win_y + (int)(bar->height) > 0)?"yes":"no",
							 ((int)(bar->win_y) > prev_y)?"yes":"no");
            if( (force || DoesBarNeedsRendering(bar)) &&
                bar->win_y + (int)(bar->height) > 0 &&
                (int)(bar->win_y) > prev_y )
            {
                if( render_astbar( bar, menu->main_canvas ) )
                    rendered = True ;
            }
            prev_y = bar->win_y ;
        }
		if( menu->visible_items_num < menu->items_num )
		{
            if( render_astbar( menu->scroll_up_bar, menu->main_canvas ) )
                rendered = True ;
            if( render_astbar( menu->scroll_down_bar, menu->main_canvas ) )
                rendered = True ;
		}
        STOP_LONG_DRAW_OPERATION;
		LOCAL_DEBUG_OUT( "%s menu items rendered!", rendered?"some":"none" );
        if( rendered )
        {
            update_canvas_display( menu->main_canvas );
            ASSync(False);
			/* yield to let some time for menu redrawing to happen : */
			//sleep_a_millisec(10);
			menu->rendered = True ;
        }
    }
}

/*************************************************************************/
/* midium level ASMenu functionality :                                      */
/*************************************************************************/
static unsigned int
extract_recent_subitems( MenuData *md, MenuDataItem **subitems, unsigned int max_subitems )
{
    int used = 0 ;
    MenuDataItem *smdi = md->first ;
    while( smdi != NULL )
    {
        if( smdi->last_used_time > 0 )
        {
            if( used < max_subitems )
            {
                subitems[used] = smdi ;
                ++used ;
            }else
            {
                register int i = used ;
                while( --i >= 0 )
                    if( subitems[i]->last_used_time  > smdi->last_used_time )
                    {
                        subitems[i] = smdi ;
                        break;
                    }
            }
        }
        smdi = smdi->next ;
    }/* while smdi */
    return used;
}

void
set_asmenu_data( ASMenu *menu, MenuData *md, Bool first_time )
{
    int items_num = md->items_num ;
    int i = 0 ;
    int real_items_num = 0;
    int max_icon_size  = 0;
    MenuDataItem **subitems = NULL ;


    if( menu->items_num < items_num )
    {
        menu->items = realloc( menu->items, items_num*(sizeof(ASMenuItem)));
        memset( &(menu->items[menu->items_num]), 0x00, (items_num-menu->items_num)*sizeof(ASMenuItem));
    }

    if( menu->title )
    {
        free( menu->title );
        menu->title = NULL;
    }

    if( items_num > 0 )
    {
        MenuDataItem *mdi = md->first ;

        if( Scr.Feel.recent_submenu_items > 0 )
            subitems = safecalloc( Scr.Feel.recent_submenu_items, sizeof(MenuDataItem*));

        for(mdi = md->first; real_items_num < items_num && mdi != NULL ; mdi = mdi->next )
            if( mdi->fdata->func == F_TITLE && menu->title == NULL )
            {
                menu->title = mystrdup( mdi->item );
            }else
            {
                ASMenuItem *item = &(menu->items[real_items_num]);
                set_asmenu_item_data( item, mdi );

                ++real_items_num;
                if( mdi->fdata->func == F_POPUP && item->submenu != NULL && subitems != NULL )
                {
                    int used = extract_recent_subitems( item->submenu, subitems, Scr.Feel.recent_submenu_items );
                    if( used > 0 )
                    {
                        items_num += used ;
                        if( menu->items_num < items_num )
                        {
                            int to_zero = max(real_items_num,menu->items_num);
                            menu->items = realloc( menu->items, items_num*(sizeof(ASMenuItem)));
                            memset( &(menu->items[to_zero]), 0x00, (items_num-to_zero)*sizeof(ASMenuItem));
                        }
                        for( i = 0 ; i < used ; ++i )
                        {
    	            	    set_asmenu_item_data( &(menu->items[real_items_num]), subitems[i] );
                            subitems[i] = NULL ;
                            set_flags( menu->items[real_items_num].flags, AS_MenuItemSubitem );
                            ++real_items_num;
                        }
                    }
                }
            }
        if( real_items_num > 0 )
        {
            for( i = 0 ; i < real_items_num ; ++i )
            {
                register ASMenuItem *item = &(menu->items[i]);
                if( item->icon )
                    if( item->icon->width > max_icon_size )
                        max_icon_size = item->icon->width ;
            }
            set_flags( menu->items[0].flags, AS_MenuItemFirst );
            set_flags( menu->items[real_items_num-1].flags, AS_MenuItemLast );
        }
        if( subitems )
            free( subitems );
    }
    menu->icon_space = MIN(max_icon_size,(Scr.MyDisplayWidth>>3));
    /* if we had more then needed tbars - destroy the rest : */
    if( menu->items )
    {
        i = menu->items_num ;
        while( --i >= real_items_num )
            free_asmenu_item(&(menu->items[i]));
    }
    menu->items_num = real_items_num ;
    menu->top_item = 0 ;
	if( first_time )
	{
	    menu->selected_item = 0 ;
    	if( menu->items && menu->items[0].bar )
        	set_astbar_focused( menu->items[0].bar, menu->main_canvas, True );
	}else
		menu->selected_item = -1 ;
    menu->pressed_item = -1;
}

void
set_menu_scroll_bar_look( ASTBarData *bar, MyLook *look, Bool up )
{
	ASFlagType hilite = 0, fhilite = 0;

	bar->h_spacing = DEFAULT_MENU_SPACING ;
    bar->h_border = DEFAULT_MENU_ITEM_HBORDER ;
    bar->v_border = DEFAULT_MENU_ITEM_VBORDER ;

    delete_astbar_tile( bar, -1 );
    /* now readd it as proper type : */
    if( look->MenuArrow )
        add_astbar_icon( bar, 7, 0, up?FLIP_VERTICAL:FLIP_VERTICAL|FLIP_UPSIDEDOWN,
		                 ALIGN_CENTER|RESIZE_H|RESIZE_H_SCALE, look->MenuArrow->image );
    else
        add_astbar_label( bar, 7, 0, 0, ALIGN_CENTER, 5, 5, "...", AS_Text_ASCII );

	set_astbar_style_ptr( bar, BAR_STATE_UNFOCUSED, look->MSMenu[MENU_BACK_ITEM] );
    set_astbar_style_ptr( bar, BAR_STATE_FOCUSED,   look->MSMenu[MENU_BACK_HILITE] );

	if( look->DrawMenuBorders == DRAW_MENU_BORDERS_ITEM )
        fhilite = hilite = DEFAULT_MENU_HILITE;
    else if ( Scr.Look.DrawMenuBorders == DRAW_MENU_BORDERS_OVERALL )
    {
        hilite = NO_HILITE_OUTLINE|LEFT_HILITE|RIGHT_HILITE|(up?TOP_HILITE:BOTTOM_HILITE) ;
        fhilite = hilite ;
    }else if( look->DrawMenuBorders == DRAW_MENU_BORDERS_FOCUSED_ITEM )
        fhilite = DEFAULT_MENU_HILITE;
    else if ( Scr.Look.DrawMenuBorders == DRAW_MENU_BORDERS_O_AND_F )
    {
        hilite = NO_HILITE_OUTLINE|LEFT_HILITE|RIGHT_HILITE|(up?TOP_HILITE:BOTTOM_HILITE) ;
        fhilite = DEFAULT_MENU_HILITE ;
    }

    set_astbar_hilite( bar, BAR_STATE_UNFOCUSED, hilite );
    set_astbar_hilite( bar, BAR_STATE_FOCUSED, fhilite );

    set_astbar_composition_method( bar, BAR_STATE_UNFOCUSED, Scr.Look.menu_icm );
    set_astbar_composition_method( bar, BAR_STATE_FOCUSED, Scr.Look.menu_hcm );
}

void
set_asmenu_look( ASMenu *menu, MyLook *look )
{
    int i ;
    unsigned int max_width = 0, max_height = 0;
    int display_size ;

    menu->arrow_space = look->MenuArrow?look->MenuArrow->width:DEFAULT_ARROW_SIZE ;

	set_menu_scroll_bar_look( menu->scroll_up_bar, look, True );
	set_menu_scroll_bar_look( menu->scroll_down_bar, look, False );

    i = menu->items_num ;
    while ( --i >= 0 )
    {
        unsigned int width, height ;
        register ASTBarData *bar;
        set_asmenu_item_look( &(menu->items[i]), look, menu->icon_space, menu->arrow_space );
        if( (bar= menu->items[i].bar) != NULL )
        {
            width = calculate_astbar_width( bar );
            height = calculate_astbar_height( bar );
LOCAL_DEBUG_OUT( "i(%d)->bar(%p)->size(%ux%u)", i, bar, width, height );
            if( width > max_width )
                max_width = width ;
            if( height > max_height )
                max_height = height ;
        }
    }
    /* some sanity checks : */

    if( max_height > MAX_MENU_ITEM_HEIGHT )
        max_height = MAX_MENU_ITEM_HEIGHT ;
    if( max_height == 0 )
        max_height = 1 ;
	/* we want height to be even at all times */
	max_height = ((max_height+1)/2)*2;

    if( max_width > MAX_MENU_WIDTH )
        max_width = MAX_MENU_WIDTH ;
    if( max_width == 0 )
        max_width = 1 ;
    menu->item_width = max_width ;
    menu->item_height = max_height ;

    display_size = max_height * menu->items_num ;
	menu->scroll_bar_size = max_height/2 ;

    if( display_size > MAX_MENU_HEIGHT )
    {
        menu->visible_items_num = MAX_MENU_HEIGHT/max_height;
        display_size = menu->visible_items_num* max_height ;  /* important! */
		display_size += 2*menu->scroll_bar_size ;  /* we'll need to render two more scroll bars */
    }else
        menu->visible_items_num = display_size / max_height ;

    /* setting up desired size  - may or maynot be overriden by user actions -
     * so we'll use ConfigureNotify events later on to keep ourselves up to date*/
    if( menu->owner != NULL )
        resize_canvas( menu->main_canvas, max_width, display_size );
    menu->optimal_width  = max_width ;
    menu->optimal_height = display_size ;

    if( menu->top_item > menu->items_num - menu->visible_items_num )
        menu->top_item = menu->items_num - menu->visible_items_num ;

   set_astbar_size( menu->scroll_up_bar, max_width, menu->scroll_bar_size );
   set_astbar_size( menu->scroll_down_bar, max_width, menu->scroll_bar_size );

	i = menu->items_num ;
    while ( --i >= 0 )
        set_astbar_size( menu->items[i].bar, max_width, max_height );
    ASSync(False);
}

static void
set_menu_item_used( ASMenu *menu, MenuDataItem *mdi )
{
    MenuData *md = FindPopup( menu->name, False );
    if( md && md->magic == MAGIC_MENU_DATA )
    {
        register MenuDataItem *i = md->first ;
        while( i != NULL )
        {
            if( i == mdi )
            {
                i->last_used_time = time(NULL);
                break;
            }
            i = i->next ;
        }
    }

}



void set_asmenu_scroll_position( ASMenu *menu, int pos );

void
select_menu_item( ASMenu *menu, int selection )
{
LOCAL_DEBUG_CALLER_OUT( "%p,%d", menu, selection );
    if( AS_ASSERT(menu) || menu->items_num == 0 )
        return;
    if( selection < 0 )
        selection = 0 ;
    else if( selection >= (int)menu->items_num )
        selection = menu->items_num - 1 ;

    if( selection != menu->selected_item )
    {
        close_asmenu_submenu( menu );
		if( menu->selected_item >= 0 )
        	set_astbar_focused( menu->items[menu->selected_item].bar, menu->main_canvas, False );
    }
    set_astbar_focused( menu->items[selection].bar, menu->main_canvas, True );
    menu->selected_item = selection ;

    if( selection < menu->top_item )
        set_asmenu_scroll_position( menu, MAX(selection-1, 0) );
    else if( selection >= menu->top_item + menu->visible_items_num )
        set_asmenu_scroll_position( menu, (selection-menu->visible_items_num)+1);
    render_asmenu_bars(menu, False);
}

void
set_asmenu_scroll_position( ASMenu *menu, int pos )
{
    int curr_y ;
    int i ;
	int first_item, last_item ;

LOCAL_DEBUG_CALLER_OUT( "%p,%d", menu, pos );
    if( AS_ASSERT(menu) || menu->items_num == 0 )
        return;
	first_item = (pos < 0)?0:pos ;

	last_item =first_item + menu->visible_items_num-1 ;
	if( last_item >= menu->items_num )
	{
		last_item = menu->items_num-1 ;
		first_item = max(0, menu->items_num - menu->visible_items_num);
	}

	for( i = 0 ; i < first_item ; ++i )
		move_astbar( menu->items[i].bar, menu->main_canvas, 0, -menu->item_height );
	for( i = last_item+1 ; i < menu->items_num ; ++i )
		move_astbar( menu->items[i].bar, menu->main_canvas, 0, -menu->item_height );

	curr_y = 0 ;
	if( first_item > 0 || last_item < menu->items_num-1 )
	{
		move_astbar( menu->scroll_up_bar, menu->main_canvas, 0, curr_y );
		curr_y += menu->scroll_bar_size ;
	}else
	{
		move_astbar( menu->scroll_up_bar, menu->main_canvas, 0, -menu->scroll_bar_size );
		move_astbar( menu->scroll_up_bar, menu->main_canvas, 0, -menu->scroll_bar_size );
	}

	for( i = first_item ; i <= last_item ; ++i )
	{
		move_astbar( menu->items[i].bar, menu->main_canvas, 0, curr_y );
        curr_y += menu->item_height ;
	}
	if( first_item > 0 || last_item < menu->items_num-1 )
		move_astbar( menu->scroll_down_bar, menu->main_canvas, 0, curr_y );

LOCAL_DEBUG_OUT("adj_pos(%d)->curr_y(%d)->items_num(%d)->vis_items_num(%d)->sel_item(%d)", first_item, curr_y, menu->items_num, menu->visible_items_num, menu->selected_item);

	menu->top_item = first_item ;

	if( menu->selected_item < menu->top_item )
        select_menu_item( menu, menu->top_item );
    else if( menu->selected_item >= 0 )
	{
		if( menu->visible_items_num > 0 && menu->selected_item >= (int)menu->top_item + (int)menu->visible_items_num )
        select_menu_item( menu, menu->top_item + menu->visible_items_num - 1 );
	}
    render_asmenu_bars(menu, False);
}

static inline void
run_item_submenu( ASMenu *menu, int item )
{
LOCAL_DEBUG_CALLER_OUT( "%p, %d, submenu(%p)", menu, item, menu->items[item].submenu );
    if( menu->items[item].submenu )
    {
        int x = menu->main_canvas->root_x ;
        int y ;
        ASQueryPointerRootXY( &x, &y );
        x -= 5 ;
#if 0
        if( x  < menu->main_canvas->root_x + (menu->item_width/3) )
        {
            int max_dx = Scr.MyDisplayWidth / 40 ;
            if( menu->main_canvas->root_x + (menu->item_width/3) - x  < max_dx )
                x = menu->main_canvas->root_x + (menu->item_width/3) ;
            else
                x += max_dx ;
        }
#endif
        y = menu->main_canvas->root_y+(menu->item_height*(item+1-(int)menu->top_item)) - 5 ;
/*	if( x > menu->main_canvas->root_x+menu->item_width-5 )
	    x = menu->main_canvas->root_x+menu->item_width-5 ;
*/      close_asmenu_submenu( menu );
        menu->submenu = run_submenu( menu, menu->items[item].submenu, x, y);
    }
}

void
press_menu_item( ASMenu *menu, int pressed )
{
LOCAL_DEBUG_CALLER_OUT( "%p,%d", menu, pressed );

    if( AS_ASSERT(menu) || menu->items_num == 0 )
        return;

    if( pressed >= (int)menu->items_num )
        pressed = menu->items_num - 1 ;

    if( menu->pressed_item >= 0 )
        set_astbar_pressed( menu->items[menu->pressed_item].bar, menu->main_canvas, False );

    if( pressed >= 0 )
    {
        if( get_flags(menu->items[pressed].flags, AS_MenuItemDisabled) )
            pressed = -1 ;
        else
        {
            if( pressed != menu->selected_item )
            {
                set_astbar_pressed( menu->items[pressed].bar, NULL, True );/* don't redraw yet */
                select_menu_item( menu, pressed );  /* this one updates display already */
            }else
                set_astbar_pressed( menu->items[pressed].bar, menu->main_canvas, True );
            set_menu_item_used( menu, menu->items[pressed].source );
            run_item_submenu( menu, pressed );
        }
    }
    render_asmenu_bars(menu, False);
LOCAL_DEBUG_OUT( "pressed(%d)->old_pressed(%d)->focused(%d)", pressed, menu->pressed_item, menu->focused );
    if( pressed < 0 && menu->pressed_item >= 0 && menu->focused )
    {
        ASMenuItem *item = &(menu->items[menu->pressed_item]);
        if( !get_flags(item->flags, AS_MenuItemDisabled) &&
            item->fdata->func != F_POPUP )
        {
            set_menu_item_used( menu, item->source );
            ExecuteFunction( item->fdata, NULL, -1 );
            if( !menu->pinned )
                close_asmenu( &ASTopmostMenu );
        }
    }
    menu->pressed_item = pressed ;
}
/*************************************************************************/
/* Menu event handlers  - ASInternalWindow interface :                   */
/*************************************************************************/
void
menu_register_subwindows( struct ASInternalWindow *asiw )
{
    ASMenu   *menu = (ASMenu*)(asiw->data) ;
    if( menu != NULL && menu->magic == MAGIC_ASMENU )
    {
        register_aswindow( menu->main_canvas->w, asiw->owner );
    }
}

void
on_menu_moveresize( ASInternalWindow *asiw, Window w )
{
    ASMenu   *menu = (ASMenu*)(asiw->data) ;
    if( menu != NULL && menu->magic == MAGIC_ASMENU )
    {   /* handle config change */
        ASFlagType changed = handle_canvas_config( menu->main_canvas );
        register int i = menu->items_num ;
LOCAL_DEBUG_OUT( "changed(%lX)->main_width(%d)->main_height(%d)->item_height(%d)", changed, menu->main_canvas->width, menu->main_canvas->height, menu->item_height);
        if( get_flags( changed, CANVAS_RESIZED) )
        {
            if( get_flags( changed, CANVAS_WIDTH_CHANGED) )
            {
                while ( --i >= 0 )
                    set_astbar_size(menu->items[i].bar, menu->main_canvas->width, menu->item_height);
				set_astbar_size(menu->scroll_up_bar, menu->main_canvas->width, menu->scroll_bar_size);
				set_astbar_size(menu->scroll_down_bar, menu->main_canvas->width, menu->scroll_bar_size);
            }
            if( get_flags( changed, CANVAS_HEIGHT_CHANGED) )
            {
                menu->visible_items_num = menu->main_canvas->height / menu->item_height ;
				if( menu->visible_items_num < menu->items_num )
					menu->visible_items_num = (menu->main_canvas->height - (menu->scroll_bar_size*2) )/ menu->item_height ;
LOCAL_DEBUG_OUT( "update_canvas_display via set_asmenu_scroll_position from move_resize %s", "");
                set_asmenu_scroll_position( menu, menu->top_item );
            }
        }else if( get_flags( changed, CANVAS_MOVED) )
        {
            while ( --i >= 0 )
                update_astbar_transparency(menu->items[i].bar, menu->main_canvas, False);
			update_astbar_transparency(menu->scroll_up_bar, menu->main_canvas, False);
			update_astbar_transparency(menu->scroll_down_bar, menu->main_canvas, False);
        }
        if( changed != 0 )
            render_asmenu_bars(menu, get_flags( changed, CANVAS_RESIZED));
    }
}


/* fwindow looses/gains focus : */
void
on_menu_hilite_changed( ASInternalWindow *asiw, ASMagic *data, Bool focused )
{
    ASMenu   *menu = (ASMenu*)(asiw->data) ;
LOCAL_DEBUG_CALLER_OUT( "%p, %p, %d", asiw, data, focused );
    if( menu != NULL && menu->magic == MAGIC_ASMENU )
    {
        /* TODO : hilite/unhilite selected item, and
         * withdraw non-pinned menu if it has no submenus */
        menu->focused = focused ;
    }
}

/* ButtonPress/Release event on one of the contexts : */
void
on_menu_pressure_changed( ASInternalWindow *asiw, int pressed_context )
{
    ASMenu   *menu = (ASMenu*)(asiw->data) ;
LOCAL_DEBUG_CALLER_OUT( "%p,%p,0x%X", asiw, menu, pressed_context );
    if( menu != NULL && menu->magic == MAGIC_ASMENU )
    {
        /* press/depress menu item, possibly change the selection,
         * and run the function when item is depressed */
        if( pressed_context )
        {
            int px = 0, py = 0 ;
            ASQueryPointerWinXY( menu->main_canvas->w, &px, &py );
LOCAL_DEBUG_OUT( "pointer(%d,%d)", px, py );
            if( px >= 0 && px < menu->main_canvas->width &&  py >= 0 && py < menu->main_canvas->height )
            {
				int pressed = -1 ;
				if( menu->visible_items_num < menu->items_num )
				{
					if( py < menu->scroll_bar_size )
					{
						set_asmenu_scroll_position( menu, menu->top_item-1 );
					}else if( py > menu->scroll_down_bar->win_y )
					{
						set_asmenu_scroll_position( menu, menu->top_item+1 );
					}else
					{
						pressed = (py - menu->scroll_bar_size)/menu->item_height ;
					}
					if( pressed < 0 )
						return ;
				}else
                	pressed = py/menu->item_height ;
				pressed += menu->top_item ;
                if( pressed != menu->pressed_item )
                    press_menu_item( menu, pressed );
            }
        }else if( menu->pressed_item >= 0 )
        {
            press_menu_item(menu, -1 );
        }
    }
}

/* Motion notify : */
void
on_menu_pointer_event( ASInternalWindow *asiw, ASEvent *event )
{
    ASMenu   *menu = (ASMenu*)(asiw->data) ;
    if( menu != NULL && menu->magic == MAGIC_ASMENU && event)
    {
        /* change selection and maybe pop a submenu */
        XMotionEvent *xmev = &(event->x.xmotion);
        ASCanvas *canvas = menu->main_canvas ;
		int pointer_x = event->x.xmotion.x_root, pointer_y = event->x.xmotion.y_root;
		int px, py ;
		XEvent tmp_e ;
		if( ASCheckTypedWindowEvent(canvas->w,LeaveNotify,&tmp_e) )
		{
			XPutBackEvent( dpy, &tmp_e );
			return ;    /* pointer has moved into other window - ignore this event! */
		}
		/* must get current pointer position as MotionNotify events
		   tend to accumulate while we are drawing and we start getting
		   late with menu selection, creating an illusion of slowness */
		ASQueryPointerRootXY( &pointer_x, &pointer_y );
        px = pointer_x - canvas->root_x;
		py = pointer_y - canvas->root_y;

        if( px >= 0 && px < canvas->width &&  py >= 0 && py < canvas->height )
        {
			int selection = -1 ;
			if( menu->items_num > menu->visible_items_num )
			{
				Bool render = False ;
				if( py < menu->scroll_bar_size )
				{
					if( set_astbar_focused( menu->scroll_up_bar, menu->main_canvas, True ) )
						render = True ;
					if( set_astbar_focused( menu->scroll_down_bar, menu->main_canvas, False ) )
						render = True ;
				}else
				{
			   		if( set_astbar_focused( menu->scroll_up_bar, menu->main_canvas, False ) )
						render = True ;
					if( py > menu->scroll_down_bar->win_y )
					{
						if( set_astbar_focused( menu->scroll_down_bar, menu->main_canvas, True ) )
							render = True ;
					}else
					{
						if( set_astbar_focused( menu->scroll_down_bar, menu->main_canvas, False ) )
							render = True ;
						else
							selection = (py-menu->scroll_bar_size)/menu->item_height ;
					}
				}
				if( selection < 0 )
				{
					if( render )
						render_asmenu_bars(menu, False);
					return;
				}
			}else
				selection = py/menu->item_height ;
			selection += menu->top_item ;

            if( selection != menu->selected_item )
            {
                if( xmev->state & ButtonAnyMask )
                    press_menu_item( menu, selection );
                else
                    select_menu_item( menu, selection );
            }
            if( px > canvas->width - ( menu->arrow_space + DEFAULT_MENU_SPACING ) &&
                menu->submenu == NULL )
                run_item_submenu( menu, selection );
        }
    }
}

/* KeyPress/Release : */
void
on_menu_keyboard_event( ASInternalWindow *asiw, ASEvent *event )
{
    ASMenu   *menu = (ASMenu*)(asiw->data) ;
    if( menu != NULL && menu->magic == MAGIC_ASMENU )
    {
        /* TODO : goto to menu item using the shortcut key */
    }
}

/* reconfiguration : */
void menu_destroy( ASInternalWindow *asiw );
void
on_menu_look_feel_changed( ASInternalWindow *asiw, ASFeel *feel, MyLook *look, ASFlagType what )
{
    ASMenu   *menu = (ASMenu*)(asiw->data) ;
    if( menu != NULL && menu->magic == MAGIC_ASMENU )
    {
		ASHints *hints ;
		unsigned int tbar_width ;

        if( menu->items )
        {
            register int i = menu->items_num;
            while ( --i >= 0 )
                free_asmenu_item( &(menu->items[i]));
            free( menu->items );
            menu->items = NULL ;
			menu->items_num = 0 ;
        }

        if( get_flags( what, FEEL_CONFIG_CHANGED ) )
        {
            MenuData *md = FindPopup (menu->name, False);
            if( md == NULL )
            {
                menu_destroy( asiw );
                return ;
            }
            set_asmenu_data( menu, md, False );
        }
        set_asmenu_look( menu, look );
		hints = make_menu_hints( menu );
	    estimate_titlebar_size( hints, &tbar_width, NULL );
   	    destroy_hints( hints, False );

    	if( tbar_width > MAX_MENU_WIDTH )
        	tbar_width = MAX_MENU_WIDTH ;
    	if( tbar_width > menu->optimal_width )
		{
			int i = menu->items_num ;
        	menu->optimal_width = tbar_width ;
			LOCAL_DEBUG_OUT( "menu_tbar_width = %d - resizing items!", tbar_width );
	        resize_canvas( menu->main_canvas, tbar_width, menu->optimal_height );
		    while ( --i >= 0 )
        		set_astbar_size( menu->items[i].bar, tbar_width, menu->item_height );
		}
	    if( menu->items && menu->items[0].bar )
    	    set_astbar_focused( menu->items[0].bar, menu->main_canvas, False );
		menu->selected_item = -1 ;
		set_asmenu_scroll_position( menu, 0 );

//        render_asmenu_bars(menu);
    }
}

void
on_menu_root_background_changed( ASInternalWindow *asiw )
{
    ASMenu   *menu = (ASMenu*)(asiw->data) ;
    if( menu != NULL && menu->magic == MAGIC_ASMENU )
    {
    /* TODO : update transparency here */
    }
}

/* destruction */
void
menu_destroy( ASInternalWindow *asiw )
{
    destroy_asmenu( (ASMenu**)&(asiw->data) );
}

ASHints *
make_menu_hints( ASMenu *menu )
{

    ASHints *hints = NULL ;

	if( menu == NULL )
		return NULL ;

	hints = safecalloc( 1, sizeof(ASHints) );

	/* normal hints : */
    hints->names[0] = mystrdup(menu->title?menu->title:menu->name);
    hints->names[1] = mystrdup(ASMENU_RES_CLASS);
    /* these are merely shortcuts to the above list DON'T FREE THEM !!! */
    hints->res_name  = hints->names[1];
    hints->res_class = hints->names[1];
    hints->icon_name = hints->names[0];

    hints->flags = AS_DontCirculate|
                   AS_SkipWinList|
                   AS_Titlebar|
                   AS_Border|
                   AS_Handles|
                   AS_AcceptsFocus|
                   AS_Gravity|
                   AS_MinSize|
                   AS_MaxSize|
                   AS_SizeInc;//|AS_VerticalTitle ;
    hints->protocols = AS_DoesWmTakeFocus ;
    hints->function_mask = ~(AS_FuncPopup|     /* everything else is allowed ! */
                             AS_FuncMinimize|
                             AS_FuncMaximize);

    hints->max_width  = MAX_MENU_WIDTH ;
    hints->max_height = MIN(MAX_MENU_HEIGHT,menu->items_num*menu->item_height);
    hints->width_inc  = 0 ;
    hints->height_inc = menu->item_height;
    hints->gravity = NorthWestGravity ;
    hints->border_width = BW ;
    hints->handle_width = BOUNDARY_WIDTH;

    hints->frame_name = mystrdup("ASMenuFrame");
    hints->mystyle_names[BACK_FOCUSED] = mystrdup(Scr.Look.MSMenu[MENU_BACK_HITITLE]->name);
    hints->mystyle_names[BACK_UNFOCUSED] = mystrdup(Scr.Look.MSMenu[MENU_BACK_TITLE]->name);
    hints->mystyle_names[BACK_STICKY] = mystrdup(Scr.Look.MSMenu[MENU_BACK_TITLE]->name);

    hints->disabled_buttons = 0;

    hints->min_width  = menu->optimal_width ;
    hints->min_height = menu->item_height ;

	return hints;
}

/*************************************************************************/
/* End of Menu event handlers:                                           */
/*************************************************************************/
void
show_asmenu( ASMenu *menu, int x, int y )
{
    ASStatusHints status ;
    ASHints *hints = make_menu_hints( menu );
	ASInternalWindow *asiw = safecalloc( 1, sizeof(ASInternalWindow));
    int gravity = NorthWestGravity ;
    unsigned int tbar_width = 0, tbar_height = 0 ;
    ASRawHints raw ;
    static char *ASMenuStyleNames[2] = {"ASMenu",NULL} ;
    ASDatabaseRecord db_rec;

    asiw->data = (ASMagic*)menu;

    asiw->register_subwindows = menu_register_subwindows;
    asiw->on_moveresize = on_menu_moveresize;
    asiw->on_hilite_changed = on_menu_hilite_changed ;
    asiw->on_pressure_changed = on_menu_pressure_changed;
    asiw->on_pointer_event = on_menu_pointer_event;
    asiw->on_keyboard_event = on_menu_keyboard_event;
    asiw->on_look_feel_changed = on_menu_look_feel_changed;
    asiw->destroy = menu_destroy;

    estimate_titlebar_size( hints, &tbar_width, &tbar_height );
    if( tbar_width > MAX_MENU_WIDTH )
        tbar_width = MAX_MENU_WIDTH ;

    if( tbar_width > menu->optimal_width )
    {
        menu->optimal_width = tbar_width ;
        hints->min_width  = tbar_width ;
    }

    if( x <= MIN_MENU_X )
        x = MIN_MENU_X ;
    else if( x + menu->optimal_width + tbar_height> MAX_MENU_X )
    {
        x = MAX_MENU_X - menu->optimal_width ;
        gravity = NorthEastGravity ;
    }else if( x + menu->optimal_width + tbar_height + menu->optimal_width> MAX_MENU_X )
		gravity = NorthEastGravity ;

    if( y <= MIN_MENU_Y )
        y = MIN_MENU_Y ;
    else if( y + menu->optimal_height + tbar_height> MAX_MENU_Y )
    {
        y = MAX_MENU_Y - menu->optimal_height;
        gravity = (gravity == NorthWestGravity)?SouthWestGravity:SouthEastGravity ;
    }else if( y + menu->optimal_height + tbar_height + menu->optimal_height> MAX_MENU_Y )
        gravity = (gravity == NorthWestGravity)?SouthWestGravity:SouthEastGravity ;

    hints->gravity = gravity ;


    /* status hints : */
    memset( &status, 0x00, sizeof( ASStatusHints ) );
    status.flags = AS_StartPosition|
                   AS_StartPositionUser|
                   AS_StartSize|
                   AS_StartSizeUser|
                   AS_StartViewportX|
                   AS_StartViewportY|
                   AS_StartDesktop|
                   AS_StartLayer|
                   AS_StartsSticky;

    status.x = x;
    status.y = y;
    status.width = menu->optimal_width;
    status.height = menu->optimal_height;
    status.viewport_x = Scr.Vx;
    status.viewport_y = Scr.Vy;
    status.desktop = Scr.CurrentDesk;
    status.layer = AS_LayerMenu;

    /* now lets merge it with database record for ASMenu style - if it exists : */
    if( fill_asdb_record (Database, &(ASMenuStyleNames[0]), &db_rec, False) )
    {
        memset( &raw, 0x00, sizeof(raw) );
        raw.placement.x = status.x ;
        raw.placement.y = status.y ;
        raw.placement.width = menu->optimal_width ;
        raw.placement.height = menu->optimal_height ;
        raw.scr = &Scr ;
        raw.border_width = 0 ;

        merge_asdb_hints (hints, &raw, &db_rec, &status, ASFLAGS_EVERYTHING);
    }
    /* lets make sure we got everything right : */
    check_hints_sanity (&Scr, hints );
    check_status_sanity (&Scr, &status);

#if 0
    {
        int pointer_x = 0, pointer_y  = 0;
        if( ASQueryPointerRootXY(&pointer_x,&pointer_y) )
        {
            if( pointer_x< status.x || pointer_y < status.y ||
                pointer_x > status.x + status.width ||
                pointer_y > status.y + status.height  )
            {/* not likely to happen:  */
                XWarpPointer(dpy, Scr.Root, Scr.Root, pointer_x, pointer_y, 0, 0, status.x+5, status.y+5);
            }
        }
    }
#endif
//    move_canvas( menu->main_canvas, status.x, status.y );
    menu->owner = AddInternalWindow( menu->main_canvas->w, &asiw, &hints, &status );

    /* need to cleanup if we failed : */
    if( asiw )
    {
        if( asiw->data )
            destroy_asmenu( &menu );
        free( asiw );
    }
    if( hints )
        destroy_hints( hints, False );
}

/*************************************************************************/
/* high level ASMenu functionality :                                     */
/*************************************************************************/
ASMenu *
run_submenu( ASMenu *supermenu, MenuData *md, int x, int y )
{
    ASMenu *menu = NULL ;
    if( md )
    {
        menu = create_asmenu(md->name);
		menu->rendered = False ;
        set_asmenu_data( menu, md, True );
        set_asmenu_look( menu, &Scr.Look );
        /* will set scroll position when ConfigureNotify arrives */
        menu->supermenu = supermenu;
        show_asmenu(menu, x, y );
		MapConfigureNotifyLoop();
    }
    return menu;
}


void
run_menu_data( MenuData *md )
{
    int x = 0, y = 0;
	Bool persistent = (get_flags( Scr.Feel.flags, PersistentMenus ) || ASTopmostMenu == NULL );

    close_asmenu(&ASTopmostMenu);

	if( persistent )
	{
        if( md == NULL )
    	    return;

    	if( !ASQueryPointerRootXY(&x,&y) )
    	{
        	x = (Scr.MyDisplayWidth*2)/3;
        	y = (Scr.MyDisplayHeight*3)/ 4;
    	}
    	ASTopmostMenu = run_submenu(NULL, md, x, y );
	}
}

void
run_menu( const char *name )
{
    MenuData *md = FindPopup (name, False);
    run_menu_data( md );
}


ASMenu *
find_asmenu( const char *name )
{
    if( name )
    {
        ASMenu *menu = ASTopmostMenu ;
        while( menu )
        {
            if( mystrcasecmp(menu->name, name) == 0 ||
                mystrcasecmp(menu->title, name) == 0)
                return menu;
            menu = menu->submenu ;
        }
    }
    return NULL;
}

ASMenu *
find_topmost_transient_menu( ASMenu *menu )
{
    if( menu && menu->supermenu && menu->supermenu->magic == MAGIC_ASMENU )
        if( !menu->supermenu->pinned )
            return find_topmost_transient_menu( menu->supermenu );
    return menu;
}

void
pin_asmenu( ASMenu *menu )
{
    if( menu )
    {
        ASMenu *menu_to_close = find_topmost_transient_menu( menu );

        close_asmenu_submenu( menu );
        if( menu == ASTopmostMenu )
            ASTopmostMenu = NULL ;
        else if( menu->supermenu &&
                 menu->supermenu->magic == MAGIC_ASMENU &&
                 menu->supermenu->submenu == menu )
            menu->supermenu->submenu = NULL ;
        if( menu_to_close != menu )
            close_asmenu( &menu_to_close );

        menu->pinned = True ;
        if( menu->owner )
        {
            clear_flags( menu->owner->hints->function_mask, AS_FuncPinMenu);
            redecorate_window( menu->owner, False );
            on_window_status_changed( menu->owner, True, True );
            if( Scr.Windows->hilited == menu->owner )
                on_window_hilite_changed (menu->owner, True);
        }
        close_asmenu_submenu( menu );
    }
}

Bool
is_menu_pinnable( ASMenu *menu )
{
    if( menu && menu->magic == MAGIC_ASMENU )
        return !(menu->pinned);
    return False;
}


