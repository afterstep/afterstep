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
#include "../../configure.h"

#include "../../include/asapp.h"
#include <X11/keysym.h>

#include "../../include/afterstep.h"
#include "../../include/screen.h"
#include "../../include/mystyle.h"
#include "../../include/decor.h"
#include "asinternals.h"
#include "menus.h"

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
    w = create_visual_window( Scr.asv, parent, -10, -10, 5, 5, 0, InputOutput, CWEventMask, &attr );

    if( w && parent != Scr.Root )
        XMapRaised( dpy, w );
    return w;
}


ASMenu *
create_asmenu()
{
    ASMenu *menu = safecalloc( 1, sizeof(ASMenu) );
    Window w ;
    menu->magic = MAGIC_ASMENU ;
    w = make_menu_window( None );
    menu->main_canvas = create_ascanvas( w );
    return menu;
}

void
destroy_asmenu(ASMenu **pmenu)
{
    if( pmenu )
    {
        ASMenu *menu = *pmenu;
        if( menu && menu->magic == MAGIC_ASMENU )
        {
            if( menu->main_canvas )
                destroy_ascanvas( &(menu->main_canvas) );

            if( menu->item_bar )
            {
                register int i = menu->items_num;
                while ( --i >= 0 )
                    destroy_astbar( &(menu->item_bar[i]));
                free( menu->item_bar );
                menu->item_bar = NULL ;
            }

            menu->magic = 0 ;
            free( menu );
            *pmenu = NULL ;
        }
    }
}

static void
set_asmenu_item_data( ASTBarData **pbar, MenuDataItem *mdi )
{
    ASTBarData *bar = *pbar ;

    if( bar == NULL )
        bar = create_astbar();
    set_astbar_label( bar, mdi->item );
}

static Bool
set_asmenu_item_look( ASTBarData *bar, MyLook *look )
{
    if( bar == NULL )
        return False;

    set_astbar_style( bar, BAR_STATE_UNFOCUSED, look->MSMenu[MENU_BACK_ITEM]->name );
    set_astbar_style( bar, BAR_STATE_FOCUSED, look->MSMenu[MENU_BACK_HILITE]->name );
    return True;
}

/*************************************************************************/
/* midium level ASMenu functionality :                                      */
/*************************************************************************/
void
set_asmenu_data( ASMenu *menu, MenuData *md )
{
    int items_num = md->items_num ;
    int i = 0 ;
    int real_items_num = 0;
    if( menu->items_num < items_num )
    {
        menu->item_bar = realloc( menu->item_bar, items_num*(sizeof(ASTBarData*)));
        for( i = menu->items_num ; i < items_num ; ++i )
            menu->item_bar[i] = NULL ;
    }

    if( items_num > 0 )
    {
        MenuDataItem *mdi = md->first ;
        for(i = 0; i < items_num && mdi != NULL ; ++i )
        {
            set_asmenu_item_data( &(menu->item_bar[i]), mdi );
            mdi = mdi->next ;
        }
        real_items_num = i;
    }
    /* if we had more then needed tbars - destroy the rest : */
    if( menu->item_bar )
    {
        i = menu->items_num ;
        while( --i >= real_items_num )
            if( menu->item_bar[i] )
                destroy_astbar( &(menu->item_bar[i]) );
    }
    menu->items_num = real_items_num ;
    menu->top_item = 0 ;
    menu->selected_item = 0 ;
}

void
set_asmenu_look( ASMenu *menu, MyLook *look )
{
    int i ;
    unsigned int max_width = 0, max_height = 0;
    int display_size ;

    i = menu->items_num ;
    while ( --i >= 0 )
    {
        unsigned int width, height ;
        register ASTBarData *bar = menu->item_bar[i];
        set_asmenu_item_look( bar, look );
        width = calculate_astbar_width( bar );
        height = calculate_astbar_height( bar );
        if( width > max_width )
            max_width = width ;
        if( height > max_height )
            max_height = height ;
    }
    /* some sanity checks : */
    if( max_height > (Scr.MyDisplayHeight>>4) )
        max_height = Scr.MyDisplayHeight>>4 ;
    if( max_width > (Scr.MyDisplayWidth>>1) )
        max_width = Scr.MyDisplayWidth>>1 ;
    menu->item_width = max_width ;
    menu->item_height = max_height ;

    display_size = max_height * menu->items_num ;
    if( display_size > (Scr.MyDisplayHeight*3)/4 )
    {
        menu->visible_items_num = ((Scr.MyDisplayHeight*3)/4)/max_height;
        display_size = menu->visible_items_num* max_height ;  /* important! */
    }else
        menu->visible_items_num = display_size / max_height ;

    resize_canvas( menu->main_canvas, max_width, display_size );

    if( menu->top_item > menu->items_num - menu->visible_items_num )
        menu->top_item = menu->items_num - menu->visible_items_num ;

    i = menu->items_num ;
    while ( --i >= 0 )
        set_astbar_size( menu->item_bar[i], max_width, max_height );
}

void set_asmenu_scroll_position( ASMenu *menu, int pos );

void
select_menu_item( ASMenu *menu, int selection )
{
    if( AS_ASSERT(menu) || menu->items_num == 0 )
        return;

}

void
set_asmenu_scroll_position( ASMenu *menu, int pos )
{
    int curr_y = 0 ;
    int i ;

    if( AS_ASSERT(menu) || menu->items_num == 0 )
        return;

    if( pos < 0 )
        pos = 0 ;
    else if( pos > (int)(menu->items_num) - (int)(menu->visible_items_num) )
        pos = (int)(menu->items_num) - (int)(menu->visible_items_num) ;

    curr_y =  ((int)(menu->items_num) - pos) * menu->item_height ;
    i = menu->items_num ;
    while( --i >= 0 )
    {
        curr_y -= menu->item_height ;
        move_astbar( menu->item_bar[i], menu->main_canvas, 0, curr_y );
    }

    menu->top_item = pos ;
    if( menu->selected_item < menu->top_item )
        select_menu_item( menu, menu->top_item );
    else if( menu->selected_item >= menu->top_item + menu->visible_items_num )
        select_menu_item( menu, menu->top_item + menu->visible_items_num - 1 );
    else /* selection update canvas display just as well */
        update_canvas_display( menu->main_canvas );
}

void
show_asmenu(ASMenu *menu)
{
    // TODO:
}

/*************************************************************************/
/* high level ASMenu functionality :                                     */
/*************************************************************************/
void
run_menu( const char *name )
{
    MenuData *md = FindPopup (name, False);
    ASMenu   *menu = NULL ;

    if( md == NULL )
        return;

    menu = create_asmenu();
    set_asmenu_data( menu, md );
    set_asmenu_look( menu, &Scr.Look );
    set_asmenu_scroll_position( menu, 0 );
    show_asmenu(menu);
}
