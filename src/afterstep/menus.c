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

#define START_LONG_DRAW_OPERATION   grab_server()
#define STOP_LONG_DRAW_OPERATION    ungrab_server()



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

	attr.cursor = Scr.Feel.cursors[ASCUR_Menu] ;
    attr.event_mask = AS_MENU_EVENT_MASK ;
    w = create_visual_window( Scr.asv, parent, -10, -10, 1, 1, 0, InputOutput, CWEventMask|CWCursor, &attr );

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
	if( item->icon )
	    safe_asimage_destroy( item->icon );
	free_func_data (&(item->fdata));
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
            ASWIN_SET_FLAGS(menu->submenu->owner, AS_Hidden);
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

	        while (timer_remove_by_data (menu));

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
                ASWIN_SET_FLAGS(menu->owner, AS_Hidden);
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

			destroy_asballoon( &(menu->comment_balloon) );
			destroy_asballoon( &(menu->item_balloon) );
			
			if( menu->volitile_menu_data != NULL ) 
				destroy_menu_data( &(menu->volitile_menu_data) );

			
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
                ASWIN_SET_FLAGS(menu->owner, AS_Hidden );
                ASWIN_SET_FLAGS(menu->owner, AS_UnMapPending );
            }else
                destroy_asmenu( &(menu));
            *pmenu = NULL ;
        }
    }
}

#define FDataPopupName(fdata)   ((fdata).text)

static void
set_asmenu_item_data( ASMenuItem *item, MenuDataItem *mdi )
{
    ASImage *icon_im = NULL ;

    item->source = mdi ;

    if( item->bar == NULL )
	{
        item->bar = create_astbar();
	}else
		delete_astbar_tile( item->bar, -1 );  /* delete all tiles */


    if( item->icon )
    {
        safe_asimage_destroy( item->icon );
        item->icon = NULL ;
    }
	/* we can only use images that are reference counted */
    if( mdi->minipixmap_image )
        icon_im = mdi->minipixmap_image ;
    else if( mdi->minipixmap )
        icon_im = GetASImageFromFile( mdi->minipixmap );
	else if( mdi->fdata->func == F_CHANGE_BACKGROUND_FOREIGN ) 
		icon_im = GetASImageFromFile( mdi->fdata->text );

    if( icon_im )
	{
		item->icon = check_scale_menu_pmap( icon_im, mdi->flags );
		if( item->icon != icon_im && icon_im != mdi->minipixmap_image )
			safe_asimage_destroy( icon_im );
		if( item->icon && item->icon == mdi->minipixmap_image )
		{	
			if( item->icon->imageman != NULL )
            	item->icon = dup_asimage( item->icon );
			else
		    	item->icon = clone_asimage( item->icon, 0xFFFFFFFF );
		}
    }
LOCAL_DEBUG_OUT( "item(\"%s\")->minipixmap(\"%s\")->icon(%p)", mdi->item?mdi->item:"NULL", mdi->minipixmap?mdi->minipixmap:"NULL", item->icon );

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
	{
		int encoding = get_flags( mdi->flags, MD_NameIsUTF8)? AS_Text_UTF8: mdi->fdata->name_encoding ;
    	if( mdi->item )
		{	
        	add_astbar_label( item->bar, 3, 0, 0, ALIGN_LEFT|ALIGN_VCENTER, 0, 0, mdi->item, encoding );
			item->first_sym = mdi->item[0] ;
		}
    	/* add hotkey */
    	if( mdi->item2 )
        	add_astbar_label( item->bar, 4, 0, 0, ALIGN_RIGHT|ALIGN_VCENTER, 0, 0, mdi->item2, encoding );
	}
    item->flags = 0 ;

	if( IsDynamicPopup(mdi->fdata->func) )
	{	
		set_flags( item->flags, AS_MenuItemHasSubmenu );
	}else if( mdi->fdata->func == F_POPUP )
    {
		MenuData *submenu = FindPopup (FDataPopupName(*(mdi->fdata)), True);
        if( submenu == NULL )
            set_flags( item->flags, AS_MenuItemDisabled );
		else
		{
			MenuDataItem *smdi = submenu->first ;

			set_flags( item->flags, AS_MenuItemHasSubmenu );
			while( smdi ) 
			{
				if( smdi->fdata->func != F_TITLE )
					break;
				smdi = smdi->next ;
			}	 
			if( smdi == NULL ) 
				set_flags( item->flags, AS_MenuItemDisabled );	
		}	 
    }else if( get_flags( mdi->flags, MD_Disabled ) || mdi->fdata->func == F_NOP )
        set_flags( item->flags, AS_MenuItemDisabled );

	dup_func_data (&(item->fdata), mdi->fdata);
}

static Bool
set_asmenu_item_look( ASMenuItem *item, MyLook *look, unsigned int icon_space, unsigned int arrow_space )
{
    ASFlagType hilite = NO_HILITE, fhilite = NO_HILITE ;
    int subitem_offset = 1 ;
LOCAL_DEBUG_OUT( "item.bar(%p)->look(%p)", item->bar, look );

    if( item->bar == NULL )
        return False;

	if( get_flags( look->flags, TxtrMenuItmInd ) )
		clear_flags( item->bar->state, BAR_FLAGS_CROP_BACK );
	else
		set_flags( item->bar->state, BAR_FLAGS_CROP_UNFOCUSED_BACK );


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
/*        add_astbar_spacer( item->bar, 1, 0, 0, NO_ALIGN, 1, 1 ); */

    if( get_flags( look->flags, MenuMiniPixmaps ) && icon_space > 0 )
    {
        /*set_astbar_tile_size( item->bar, MI_LEFT_SPACER_IDX, icon_space, 1 ); */
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
    if( look->MenuArrow && get_flags( item->flags, AS_MenuItemHasSubmenu ) )
        add_astbar_icon( item->bar, 7, 0, 0, ALIGN_VCENTER, look->MenuArrow->image );
    else
        add_astbar_spacer( item->bar, 7, 0, 0, NO_ALIGN, arrow_space, 1 );


    if( get_flags( item->flags, AS_MenuItemDisabled ) )
    {
        set_astbar_style_ptr( item->bar, -1, look->MSMenu[MENU_BACK_STIPPLE]);
        set_astbar_style_ptr( item->bar, BAR_STATE_FOCUSED, look->MSMenu[MENU_BACK_STIPPLE] );
    }else
    {
        set_astbar_style_ptr( item->bar, -1,
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
		ASImage *cache = NULL ; 
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
                if( render_astbar_cached_back( bar, menu->main_canvas, &cache, NULL ) )
                    rendered = True ;
            }
            prev_y = bar->win_y ;
        }
		if( menu->visible_items_num < menu->items_num )
		{
			if( force || DoesBarNeedsRendering(menu->scroll_up_bar))
            	if( render_astbar_cached_back( menu->scroll_up_bar, menu->main_canvas, &cache, NULL ) )
                	rendered = True ;
			if( force || DoesBarNeedsRendering(menu->scroll_down_bar))
            	if( render_astbar_cached_back( menu->scroll_down_bar, menu->main_canvas, &cache, NULL ) )
                	rendered = True ;
		}
        STOP_LONG_DRAW_OPERATION;
		LOCAL_DEBUG_OUT( "%s menu items rendered!", rendered?"some":"none" );
        if( rendered )
        {
            update_canvas_display( menu->main_canvas );
            ASSync(False);
			set_flags(	menu->state, AS_MenuRendered);
        }
		if( cache ) 
			safe_asimage_destroy( cache );
    }
}

/*************************************************************************/
/* midium level ASMenu functionality :                                      */
/*************************************************************************/
static unsigned int
extract_recent_subitems( char *submenu_name, MenuDataItem **subitems, unsigned int max_subitems )
{
	MenuData *md = FindPopup (submenu_name, True);
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
set_asmenu_data( ASMenu *menu, MenuData *md, Bool first_time, Bool show_unavailable, int recent_items )
{
    int items_num = md->items_num ;
    int i = 0 ;
    int real_items_num = 0;
    int max_icon_size  = 0;
    MenuDataItem **subitems = NULL ;
	MenuDataItem *title_mdi = NULL; 


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
	clear_flags( menu->state, AS_MenuTitleIsUTF8|AS_MenuNameIsUTF8 );
	
	if( get_flags( md->flags, MD_NameIsUTF8 ) )
		set_flags( menu->state, AS_MenuNameIsUTF8 );

    if( items_num > 0 )
    {
        MenuDataItem *mdi = md->first ;
		if( md->recent_items >= 0 )
			recent_items = md->recent_items ;
        if( recent_items > 0 )
            subitems = safecalloc( recent_items, sizeof(MenuDataItem*));

		LOCAL_DEBUG_OUT( "show_unavailable = %d", show_unavailable );

        for(mdi = md->first; real_items_num < items_num && mdi != NULL ; mdi = mdi->next )
            if( mdi->fdata->func == F_TITLE && menu->title == NULL )
            {
				title_mdi = mdi ; 
                menu->title = mystrdup( mdi->item );
				if( get_flags( mdi->flags, MD_NameIsUTF8 ) )
					set_flags( menu->state, AS_MenuTitleIsUTF8 );
            }else
            {
                ASMenuItem *item = &(menu->items[real_items_num]);
				
				if( !show_unavailable ) 
				{
					if( get_flags( mdi->flags, MD_Disabled ) )
						continue;
					if( mdi->fdata->func == F_NOP  )
					{	
						int len = mdi->item?strlen(mdi->item):0 ;
						if(  mdi->item2 )
							len += strlen(mdi->item2);
						if( len > 0 )
							continue;
					}
				}

                set_asmenu_item_data( item, mdi );
                ++real_items_num;

                if( item->fdata.func == F_POPUP && !get_flags( item->flags, AS_MenuItemDisabled ) && subitems != NULL )
                {
                    int used = extract_recent_subitems( FDataPopupName(item->fdata), subitems, recent_items );
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
	
	{
		char *comment = md->comment ; 
		int encoding = get_flags( md->flags, MD_CommentIsUTF8)? AS_Text_UTF8 : AS_Text_ASCII ;
		if( title_mdi != NULL && title_mdi->comment != NULL ) 
		{
			encoding = get_flags( title_mdi->flags, MD_CommentIsUTF8)? AS_Text_UTF8 : AS_Text_ASCII ;
			comment = title_mdi->comment ; 
		}
		if( comment ) 
		{
			if( menu->comment_balloon == NULL )
				menu->comment_balloon = create_asballoon_with_text_for_state ( MenuBalloons, NULL, comment, encoding);
			else
			{
				balloon_set_text (menu->comment_balloon, comment, encoding);
				clear_flags( menu->state, AS_MenuBalloonShown);
			}
		}else
			destroy_asballoon( &(menu->comment_balloon) );
	}
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

	set_astbar_style_ptr( bar, -1, look->MSMenu[MENU_BACK_ITEM] );
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
			if( get_flags( menu->items[i].flags, AS_MenuItemSubitem ) )
				height = max_height;
			else
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
static void menu_item_balloon_timer_handler (void *data);

void
select_menu_item( ASMenu *menu, int selection, Bool render )
{
	Bool needs_scrolling ;
LOCAL_DEBUG_CALLER_OUT( "%p,%d", menu, selection );
	
    if( AS_ASSERT(menu) || menu->items_num == 0 )
        return;
    if( selection < 0 )
        selection = 0 ;
    else if( selection >= (int)menu->items_num )
        selection = menu->items_num - 1 ;

	needs_scrolling = ( selection < menu->top_item || selection >= menu->top_item + menu->visible_items_num ) ;
    if( selection != menu->selected_item )
    {
		if( menu->item_balloon )
		{
	        while (timer_remove_by_data (menu));
			withdraw_balloon( menu->item_balloon );
		}
        close_asmenu_submenu( menu );
		if( menu->selected_item >= 0 )
        	set_astbar_focused( menu->items[menu->selected_item].bar, NULL/*needs_scrolling?NULL:menu->main_canvas*/, False );
    }
    set_astbar_focused( menu->items[selection].bar, NULL/*needs_scrolling?NULL:menu->main_canvas*/, True );
    menu->selected_item = selection ;

    if( selection < menu->top_item )
        set_asmenu_scroll_position( menu, MAX(selection, 0) );
    else if( selection >= menu->top_item + menu->visible_items_num )
        set_asmenu_scroll_position( menu, (selection-menu->visible_items_num)+1);
	else if( render )
	    render_asmenu_bars(menu, False);

	if( menu->items[selection].source )
	{
		LOCAL_DEBUG_OUT( "selection func = %ld", menu->items[selection].fdata.func );
		if( menu->items[selection].source->comment ) 
		{
			int	encoding = get_flags( menu->items[selection].source->flags, MD_CommentIsUTF8)? AS_Text_UTF8 : AS_Text_ASCII ;
			if( menu->item_balloon == NULL )
				menu->item_balloon = create_asballoon_for_state ( MenuBalloons, NULL);

	        while (timer_remove_by_data (menu));
			balloon_set_text (menu->item_balloon, menu->items[selection].source->comment, encoding);
        	timer_new (MenuBalloons->look->Delay, &menu_item_balloon_timer_handler, (void *)menu);
		}else if( menu->items[selection].fdata.func == F_CHANGE_BACKGROUND_FOREIGN || 
				  menu->items[selection].fdata.func == F_CHANGE_BACKGROUND ) 
		{
			int delay = MenuBalloons->look->Delay ; 
			LOCAL_DEBUG_OUT( "change background func = \"%s\"", menu->items[selection].fdata.text );

			if( menu->item_balloon == NULL )
				menu->item_balloon = create_asballoon_for_state ( MenuBalloons, NULL);
	        while (timer_remove_by_data (menu));
			balloon_set_image_from_file (menu->item_balloon, menu->items[selection].fdata.text );
        	timer_new (delay>1000?delay-1000:delay, &menu_item_balloon_timer_handler, (void *)menu);
		}
	}
}

static void
menu_item_balloon_timer_handler (void *data)
{
    ASMenu      *menu = (ASMenu *) data;

	if( menu && menu->item_balloon ) 
		display_balloon_nodelay( menu->item_balloon );	
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
        select_menu_item( menu, menu->top_item, False );
    else if( menu->selected_item >= 0 )
	{
		if( menu->visible_items_num > 0 && menu->selected_item >= (int)menu->top_item + (int)menu->visible_items_num )
        select_menu_item( menu, menu->top_item + menu->visible_items_num - 1, False );
	}
    render_asmenu_bars(menu, False);
}

static inline void
run_item_submenu( ASMenu *menu, int item_no )
{
	ASMenuItem *item = &(menu->items[item_no]);
LOCAL_DEBUG_CALLER_OUT( "%p, %d", menu, item_no );
    if( !get_flags( item->flags, AS_MenuItemDisabled ) )
	{
		MenuData *submenu = NULL ;
		if( item->fdata.func == F_POPUP )
			submenu = FindPopup( FDataPopupName(item->fdata), True );
		else if( item->fdata.func == F_STOPMODULELIST ) 
			submenu = make_stop_module_menu(  Scr.Feel.winlist_sort_order );
		else if( item->fdata.func == F_RESTARTMODULELIST ) 
			submenu = make_restart_module_menu(  Scr.Feel.winlist_sort_order );
#ifndef NO_WINDOWLIST
		else if( item->fdata.func == F_WINDOWLIST ) 
		{	
			submenu = make_desk_winlist_menu( Scr.Windows, 
											  (item->fdata.text == NULL) ? 
											  Scr.CurrentDesk: item->fdata.func_val[0], 
											  Scr.Feel.winlist_sort_order, False );
		}
#endif		
		if( submenu )
    	{
        	int x = menu->main_canvas->root_x+(int)menu->main_canvas->bw ;
        	int y ;
        	ASQueryPointerRootXY( &x, &y );
#if 0
        	if( x  < menu->main_canvas->root_x + (int)(menu->item_width/3) )
        	{
            	int max_dx = Scr.MyDisplayWidth / 40 ;
            	if( menu->main_canvas->root_x + (int)(menu->item_width/3) - x  < max_dx )
                	x = menu->main_canvas->root_x + (int)(menu->item_width/3) ;
            	else
                	x += max_dx ;
        	}
#endif
        	y = menu->main_canvas->root_y+(menu->item_height*(item_no-(int)menu->top_item)) ;
	/*	if( x > menu->main_canvas->root_x+menu->item_width-5 )
	    	x = menu->main_canvas->root_x+menu->item_width-5 ;
	*/      close_asmenu_submenu( menu );
        	menu->submenu = run_submenu( menu, submenu, x, y);
			if( item->fdata.func != F_POPUP )
			{
				if( menu->submenu == NULL ) 
	    			destroy_menu_data( &submenu );
				else
					menu->submenu->volitile_menu_data = submenu ;
			}
		}
	}
}

void
press_menu_item( ASMenu *menu, int pressed, Bool keyboard_event )
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
			if( keyboard_event ) 
			{   /* don't press if keyboard selection */
				if( pressed != menu->selected_item )
					select_menu_item( menu, pressed, False );

			}else if( pressed != menu->selected_item )
            {
                set_astbar_pressed( menu->items[pressed].bar, NULL, True );/* don't redraw yet */
                select_menu_item( menu, pressed, False );  /* this one updates display already */
            }else
                set_astbar_pressed( menu->items[pressed].bar, menu->main_canvas, True );
            set_menu_item_used( menu, menu->items[pressed].source );
			
			if( get_flags( menu->items[pressed].flags, AS_MenuItemHasSubmenu) ) 
			{
				if( keyboard_event ) 
				{
					int x = menu->main_canvas->root_x+(int)menu->main_canvas->bw+menu->main_canvas->width-(menu->arrow_space + DEFAULT_MENU_SPACING) ;
					int y = menu->main_canvas->root_y+(int)menu->main_canvas->bw ;
					y += menu->items[pressed].bar->win_y + menu->items[pressed].bar->height - 5 ;
					XWarpPointer (dpy, None, Scr.Root, 0, 0, 0, 0, x, y);
				}		  
            	run_item_submenu( menu, pressed );
			}
        }
    }
    render_asmenu_bars(menu, False);
LOCAL_DEBUG_OUT( "pressed(%d)->old_pressed(%d)->focused(%d)", pressed, menu->pressed_item, get_flags(menu->state, AS_MenuFocused)?1:0 );
    if( get_flags(menu->state, AS_MenuFocused) )
	{	
		int item_no = keyboard_event?pressed:menu->pressed_item ;
    	
		/* if keyboard event we exec item on press event, and on release event otherwise */ 
		if( item_no >= 0 && (pressed < 0 || keyboard_event))   
    	{
        	ASMenuItem *item = &(menu->items[item_no]);
        	if( get_flags(item->flags, AS_MenuItemDisabled|AS_MenuItemHasSubmenu) == 0 )
        	{
            	set_menu_item_used( menu, item->source );
            	ExecuteFunctionForClient( &(item->fdata), menu->client_window );
            	if( !get_flags(menu->state, AS_MenuPinned) )
                	close_asmenu( &ASTopmostMenu );
        	}
    	}
	}
	if( keyboard_event )
		menu->pressed_item = -1 ;
	else
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
		if( focused ) 
		 	set_flags(menu->state, AS_MenuFocused);
		else
			clear_flags(menu->state, AS_MenuFocused);
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
                    press_menu_item( menu, pressed, False );
            }
        }else if( menu->pressed_item >= 0 )
        {
            press_menu_item(menu, -1, False );
        }
    }
}

static void 
handle_menu_selection( ASMenu *menu, int button_state, int px, int selection, Bool render)
{
    if( selection != menu->selected_item )
    {
        if( button_state & (Button1Mask|Button2Mask|Button3Mask) )
            press_menu_item( menu, selection, False );
        else
            select_menu_item( menu, selection, render );
    }
    if( px > menu->main_canvas->width - ( menu->arrow_space + DEFAULT_MENU_SPACING ) &&
        menu->submenu == NULL )
        run_item_submenu( menu, selection );

}

void
on_menu_scroll_event( ASInternalWindow *asiw, ASEvent *event )
{
    ASMenu   *menu = (ASMenu*)(asiw->data) ;
    if( menu != NULL && menu->magic == MAGIC_ASMENU && event)
    {
		if(  menu->items_num >= menu->visible_items_num )
		{
			XButtonEvent *xbtn = &(event->x.xbutton);
			int selection = menu->selected_item ; 
			int top = menu->top_item ;
			int px_offset = menu->main_canvas->root_x+(int)menu->main_canvas->bw ;
			int px, py ;
			Bool needs_scrolling ;
			
			ASQueryPointerRootXY( &px, &py );
			if( xbtn->button == Button4 )		
			{
				if( selection > 0 ) 
				{
					--selection ;
					py -= menu->item_height ;
				}
				if( selection < menu->top_item )
					--top ;
			}else
			{
				if( selection+1 < menu->items_num ) 
				{
					++selection ;
					py += menu->item_height ;
				}
				if( selection > menu->top_item + menu->visible_items_num - 1 )
					++top ;
			}
			needs_scrolling = (top != menu->top_item);
			if( selection != menu->selected_item )
			{
				handle_menu_selection( menu, xbtn->state, px-px_offset, selection, !needs_scrolling);
				if( !needs_scrolling ) 
					XWarpPointer (dpy, None, Scr.Root, 0, 0, 0, 0, px, py);
			}
//			if( needs_scrolling ) 
//				set_asmenu_scroll_position( menu, top );
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
        	if( menu->comment_balloon )
			    withdraw_balloon( menu->comment_balloon );
			XPutBackEvent( dpy, &tmp_e );
			return ;    /* pointer has moved into other window - ignore this event! */
		}
		if( menu->comment_balloon && !get_flags( menu->state, AS_MenuBalloonShown)) 
		{
   			set_flags( menu->state, AS_MenuBalloonShown);
		    display_balloon( menu->comment_balloon );
		}
		/* must get current pointer position as MotionNotify events
		   tend to accumulate while we are drawing and we start getting
		   late with menu selection, creating an illusion of slowness */
		ASQueryPointerRootXY( &pointer_x, &pointer_y );
        px = pointer_x - (canvas->root_x+(int)canvas->bw);
		py = pointer_y - (canvas->root_y+(int)canvas->bw);

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

			handle_menu_selection( menu, xmev->state, px, selection, True);
        }
    }
}

void menu_destroy( ASInternalWindow *asiw );

/* KeyPress/Release : */
void
on_menu_keyboard_event( ASInternalWindow *asiw, ASEvent *event )
{
    ASMenu   *menu = (ASMenu*)(asiw->data) ;
    if( menu != NULL && menu->magic == MAGIC_ASMENU )
    {
		KeySym keysym = XLookupKeysym (&(event->x.xkey), 0);
	    
		if (keysym == XK_Escape || 
			keysym == XK_BackSpace ||
			keysym == XK_Delete || 
		    (keysym == XK_Left && menu->supermenu ) )
		{	
			if( menu->supermenu != NULL ) 
			{	
				ASMenu *sm = menu->supermenu ; 
				int x = sm->main_canvas->root_x + sm->main_canvas->bw; 
				int y = sm->main_canvas->root_y + sm->main_canvas->bw;
				if( sm->selected_item >= 0 && sm->selected_item < sm->items_num )
				{
					ASMenuItem *item = &(sm->items[sm->selected_item]);
					x += item->bar->win_x + item->bar->width - (menu->arrow_space + DEFAULT_MENU_SPACING);
					y += item->bar->win_y + item->bar->height/2 ;
				}	 
				XWarpPointer (dpy, None, Scr.Root, 0, 0, 0, 0, x, y);
			}
			close_asmenu(&menu); 
/*            menu_destroy( asiw ); */
            return ;
		}else if ((keysym >= XK_A && keysym <= XK_Z) ||	/* Only consider alphabetic */
				  (keysym >= XK_a && keysym <= XK_z) ||
				  (keysym >= XK_0 && keysym <= XK_9))		/* ...or numeric keys     */
		{
			int i ;
			if (islower (keysym))
				keysym = toupper (keysym);
			LOCAL_DEBUG_OUT( "processing keysym [%c]", (char)keysym );
			/* Search menu for matching hotkey */
			for( i = 0 ; i < menu->items_num ; i++ )
				if( menu->items[i].fdata.hotkey == keysym )
				{
					press_menu_item( menu, i, True );
					return ;
				}
			for( i = 0 ; i < menu->items_num ; i++ )
			{
				if( menu->items[i].first_sym == keysym )
				{
					press_menu_item( menu, i, True );
					return ;
				}
			}
		}else 
		{	 
			switch( keysym )
			{
			  	case XK_Page_Up:
					break;
				case XK_Page_Down:
					break;
				case XK_Up:
				case XK_k:
				case XK_p:
					if( menu->selected_item > 0 )
	                	select_menu_item( menu, menu->selected_item-1, True );
                    break;
				case XK_Tab :
				case XK_Down:
				case XK_n:
				case XK_j:
					if( menu->selected_item < menu->items_num )
	                	select_menu_item( menu, menu->selected_item+1, True );
                    break;
				case XK_Right:
				case XK_Return :
				case XK_space :
					if( menu->selected_item	>= 0 ) 
					{	
						if( get_flags( menu->items[menu->selected_item].flags, AS_MenuItemHasSubmenu) || keysym != XK_Right ) 
		 					press_menu_item( menu, menu->selected_item, True );		
					}
			}	 
		}	
    }
}

/* reconfiguration : */

void
on_menu_look_feel_changed( ASInternalWindow *asiw, ASFeel *feel, MyLook *look, ASFlagType what )
{
    ASMenu   *menu = (ASMenu*)(asiw->data) ;
    if( menu != NULL && menu->magic == MAGIC_ASMENU )
    {
		ASHints *hints ;
		unsigned int tbar_width ;
		MenuData *md ;

   /*      if( get_flags( what, FEEL_CONFIG ) )       */
        if( menu->items )
        {
            register int i = menu->items_num;
            while ( --i >= 0 )
                free_asmenu_item( &(menu->items[i]));
            free( menu->items );
            menu->items = NULL ;
			menu->items_num = 0 ;
        }

        md = FindPopup (menu->name, False);
        if( md == NULL )
        {
        	menu_destroy( asiw );
            return ;
        }
        set_asmenu_data( menu, md, False, get_flags(look->flags, MenuShowUnavailable), feel->recent_submenu_items );
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
    {	/* update transparency here */
        register int i = menu->items_num ;
        while ( --i >= 0 )
        	update_astbar_transparency(menu->items[i].bar, menu->main_canvas, True);
		update_astbar_transparency(menu->scroll_up_bar, menu->main_canvas, True);
		update_astbar_transparency(menu->scroll_down_bar, menu->main_canvas, True);
    	render_asmenu_bars(menu, False);
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
	if( menu->title ) 
	{
    	hints->names[0] = mystrdup(menu->title);
	    hints->names_encoding[0] = get_flags( menu->state, AS_MenuTitleIsUTF8 )? AS_Text_UTF8:AS_Text_ASCII;
	}else
	{
    	hints->names[0] = mystrdup(menu->name);
	    hints->names_encoding[0] = get_flags( menu->state, AS_MenuNameIsUTF8 )? AS_Text_UTF8:AS_Text_ASCII;
	}
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
                   AS_SizeInc;/*|AS_VerticalTitle ; */
    hints->protocols = AS_DoesWmTakeFocus ;
    hints->function_mask = ~(AS_FuncPopup|     /* everything else is allowed ! */
                             AS_FuncMinimize|
                             AS_FuncMaximize);

    hints->max_width  = MAX_MENU_WIDTH ;
    hints->max_height = MIN(MAX_MENU_HEIGHT,menu->items_num*menu->item_height);
    hints->width_inc  = 0 ;
    hints->height_inc = menu->item_height;
    hints->gravity = StaticGravity ;
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
    int gravity = StaticGravity ;
    unsigned int tbar_width = 0, tbar_height = 0 ;
    ASRawHints raw ;
    static char *ASMenuStyleNames[2] = {"ASMenu",NULL} ;
    ASDatabaseRecord *db_rec;
	int my_width, my_height ;

	LOCAL_DEBUG_OUT( "menu(%s) - encoding set in hints is %d", hints->names[0], hints->names_encoding[0] ); 

    asiw->data = (ASMagic*)menu;

    asiw->register_subwindows = menu_register_subwindows;
    asiw->on_moveresize = on_menu_moveresize;
    asiw->on_hilite_changed = on_menu_hilite_changed ;
    asiw->on_pressure_changed = on_menu_pressure_changed;
    asiw->on_pointer_event = on_menu_pointer_event;
	asiw->on_scroll_event = on_menu_scroll_event;
    asiw->on_keyboard_event = on_menu_keyboard_event;
    asiw->on_look_feel_changed = on_menu_look_feel_changed;
	asiw->on_root_background_changed = on_menu_root_background_changed ;
    asiw->destroy = menu_destroy;

	db_rec = fill_asdb_record (Database, &(ASMenuStyleNames[0]), NULL, False);
	merge_asdb_hints ( hints, NULL, db_rec, NULL, HINT_GENERAL);

    estimate_titlebar_size( hints, &tbar_width, &tbar_height );

    if( tbar_width > MAX_MENU_WIDTH )
        tbar_width = MAX_MENU_WIDTH ;

    if( tbar_width > menu->optimal_width )
    {
        menu->optimal_width = tbar_width ;
        hints->min_width  = tbar_width ;
    }

	my_width = menu->optimal_width + tbar_height ; 
	my_height = menu->optimal_height + tbar_height ;

	y -= tbar_height ;
	x -= menu->optimal_width / 3 ;
	
    if( menu->supermenu ) 
    {/* we need to make sure we would not overlay our parent completely ! */
		ASCanvas *pc = menu->supermenu->main_canvas ; 
		int requested_x = x, requested_y = y ; 
		if( x + my_width > Scr.MyDisplayWidth ) 
			x = Scr.MyDisplayWidth - my_width ; 
		if( y + my_height > Scr.MyDisplayHeight ) 
			y = Scr.MyDisplayHeight - my_height ; 
		if( requested_x != x || requested_y != y ) 
		{
			if(   x <= pc->root_x + 10 
			   && x + my_width >= pc->root_x + pc->width 
			   && y <= pc->root_y + 10
			   && y + my_height >= pc->root_y + pc->height 
			  )
			{
				if( requested_y - pc->root_y > tbar_height ) 
					y = (requested_y + pc->root_y)/2 ;
				else
					y = requested_y ;
				if( y + my_height >= Scr.MyDisplayHeight ) 
				{
					if( pc->root_y + pc->height - requested_y < menu->item_height ) 
						y = requested_y - my_height ; 
					else
						y = (pc->root_y + pc->height + requested_y)/2 - my_height ;
				}
			}  
			if( requested_y != y ) 
			{
				if( requested_x != x ) 
					gravity = SouthEastGravity ; 
				else
					gravity = SouthWestGravity ;
			}else if( requested_x != x ) 	
				gravity = NorthEastGravity ;
		}      
    }else
    {
		if( x <= MIN_MENU_X )
		  	x = MIN_MENU_X ;
		else if( x + menu->optimal_width + tbar_height> MAX_MENU_X )
		{ 
		  	x = MAX_MENU_X - menu->optimal_width ;
		  	gravity = EastGravity ;
		}else if( x + menu->optimal_width + tbar_height + menu->optimal_width> MAX_MENU_X )
		  	gravity = EastGravity ;

		if( y <= MIN_MENU_Y )
		  	y = MIN_MENU_Y ;
		else if( y + menu->optimal_height + tbar_height> MAX_MENU_Y )
		{
		  	y = MAX_MENU_Y - menu->optimal_height;
		  	gravity = (gravity == StaticGravity)?SouthGravity:SouthEastGravity ;
		}else if( y + menu->optimal_height + tbar_height + menu->optimal_height> MAX_MENU_Y )
		  	gravity = (gravity == StaticGravity)?SouthGravity:SouthEastGravity ;
	}
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
    if( db_rec )
    {
        memset( &raw, 0x00, sizeof(raw) );
        raw.placement.x = status.x ;
        raw.placement.y = status.y ;
        raw.placement.width = menu->optimal_width ;
        raw.placement.height = menu->optimal_height ;
        raw.scr = ASDefaultScr ;
        raw.border_width = 0 ;

/*		LOCAL_DEBUG_OUT( "printing db record %p for names %p and db %p", pdb_rec, clean->names, db );
		print_asdb_matched_rec (NULL, NULL, Database, db_rec);
  */	
        merge_asdb_hints (hints, &raw, db_rec, &status, ASFLAGS_EVERYTHING);
		destroy_asdb_record( db_rec, False );
    }
    /* lets make sure we got everything right : */
	LOCAL_DEBUG_OUT( "menu(%s) - encoding set in hints is %d", hints->names[0], hints->names_encoding[0] ); 
    check_hints_sanity (ASDefaultScr, hints, &status, menu->main_canvas->w );
    check_status_sanity (ASDefaultScr, &status);

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
	LOCAL_DEBUG_OUT( "menu(%s) - encoding set in hints is %d", hints->names[0], hints->names_encoding[0] );

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
		clear_flags( menu->state, AS_MenuRendered);
		set_asmenu_data( menu, md, True, get_flags(Scr.Look.flags, MenuShowUnavailable), Scr.Feel.recent_submenu_items );
        set_asmenu_look( menu, &Scr.Look );
        /* will set scroll position when ConfigureNotify arrives */
        menu->supermenu = supermenu;
        show_asmenu(menu, x, y );
		MapConfigureNotifyLoop();
    }
    return menu;
}


ASMenu *
run_menu_data( MenuData *md )
{
    ASMenu *menu = NULL ;
    int x = 0, y = 0;
	Bool persistent = (get_flags( Scr.Feel.flags, PersistentMenus ) || ASTopmostMenu == NULL );

    close_asmenu(&ASTopmostMenu);

	if( persistent && md != NULL )
	{
	    if( !ASQueryPointerRootXY(&x,&y) )
    	{
        	x = (Scr.MyDisplayWidth*2)/3;
        	y = (Scr.MyDisplayHeight*3)/ 4;
	    }
    	ASTopmostMenu = run_submenu(NULL, md, x, y );
	}
    return menu;
}

void
run_menu( const char *name, Window client_window )
{
    MenuData *md = FindPopup (name, False);
    run_menu_data( md );
	if( ASTopmostMenu ) 
		ASTopmostMenu->client_window = client_window ;
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
        if( !get_flags(menu->supermenu->state, AS_MenuPinned) )
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

		set_flags(menu->state, AS_MenuPinned);
        if( menu->owner )
        {
            clear_flags( menu->owner->hints->function_mask, AS_FuncPinMenu);
            redecorate_window( menu->owner, False );
            on_window_status_changed( menu->owner, True );
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
        return !get_flags(menu->state, AS_MenuPinned);
    return False;
}


