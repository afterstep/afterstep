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

#define LOCAL_DEBUG

#include "../../include/asapp.h"
#include <X11/keysym.h>

#include "../../include/afterstep.h"
#include "../../include/screen.h"
#include "../../include/mystyle.h"
#include "../../include/decor.h"
#include "../../include/hints.h"
#include "../../include/event.h"
#include "asinternals.h"
#include "menus.h"


static ASMenu *ASTopmostMenu = NULL;

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
create_asmenu( const char *name)
{
    ASMenu *menu = safecalloc( 1, sizeof(ASMenu) );
    Window w ;
    menu->magic = MAGIC_ASMENU ;
    w = make_menu_window( None );
    menu->main_canvas = create_ascanvas( w );
    menu->name = mystrdup(name);
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
        if( menu->submenu->owner )
        {
            /* cannot use Destroy directly - must go through the normal channel: */
            unmap_canvas_window( menu->submenu->main_canvas );
        }else
            destroy_asmenu( &(menu->submenu));
        menu->submenu = NULL ;
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
            destroy_registered_window( w );

            if( menu->items )
            {
                register int i = menu->items_num;
                while ( --i >= 0 )
                    free_asmenu_item( &(menu->items[i]));
                free( menu->items );
                menu->items = NULL ;
            }

            if( menu->name )
                free( menu->name );
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
            if( menu->owner )
            {
                /* cannot use Destroy directly - must go through the normal channel: */
                unmap_canvas_window( menu->main_canvas );
            }else
                destroy_asmenu( &(menu));
            *pmenu = NULL ;
        }
    }
}

static void
set_asmenu_item_data( ASMenuItem *item, MenuDataItem *mdi )
{
    if( item->bar == NULL )
        item->bar = create_astbar();
	/* TODO:add minipixmap */
	/* add label */
    add_astbar_label( item->bar, 1, 0, 0, mdi->item );
	/* TODO:add hotkey */
	/* TODO:add popup icon */
	
    item->flags = 0 ;
    if( mdi->fdata->func == F_POPUP )
    {
        if( (item->submenu = FindPopup (mdi->fdata->text, True)) == NULL )
            set_flags( item->flags, AS_MenuItemDisabled );
    }else if( get_flags( mdi->flags, MD_Disabled ) )
        set_flags( item->flags, AS_MenuItemDisabled );
}

static Bool
set_asmenu_item_look( ASMenuItem *item, MyLook *look )
{
    if( item->bar == NULL )
        return False;

    if( get_flags( item->flags, AS_MenuItemDisabled ) )
    {
        set_astbar_style( item->bar, BAR_STATE_UNFOCUSED, look->MSMenu[MENU_BACK_STIPPLE]->name );
        set_astbar_style( item->bar, BAR_STATE_FOCUSED, look->MSMenu[MENU_BACK_STIPPLE]->name );
    }else
    {
        set_astbar_style( item->bar, BAR_STATE_UNFOCUSED, look->MSMenu[MENU_BACK_ITEM]->name );
        set_astbar_style( item->bar, BAR_STATE_FOCUSED, look->MSMenu[MENU_BACK_HILITE]->name );
    }
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
        for(mdi = md->first; real_items_num < items_num && mdi != NULL ; mdi = mdi->next )
            if( mdi->fdata->func == F_TITLE && menu->title == NULL )
            {
                menu->title = mystrdup( mdi->item );
            }else
            {
                set_asmenu_item_data( &(menu->items[real_items_num]), mdi );
                ++real_items_num;
            }
    }
    /* if we had more then needed tbars - destroy the rest : */
    if( menu->items )
    {
        i = menu->items_num ;
        while( --i >= real_items_num )
            free_asmenu_item(&(menu->items[i]));
    }
    menu->items_num = real_items_num ;
    menu->top_item = 0 ;
    menu->selected_item = 0 ;
    if( menu->items && menu->items[0].bar )
        set_astbar_focused( menu->items[0].bar, menu->main_canvas, True );
    menu->pressed_item = -1;
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
        register ASTBarData *bar;
        set_asmenu_item_look( &(menu->items[i]), look );
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

    if( max_width > MAX_MENU_WIDTH )
        max_width = MAX_MENU_WIDTH ;
    if( max_width == 0 )
        max_width = 1 ;
    menu->item_width = max_width ;
    menu->item_height = max_height ;

    display_size = max_height * menu->items_num ;

    if( display_size > MAX_MENU_HEIGHT )
    {
        menu->visible_items_num = MAX_MENU_HEIGHT/max_height;
        display_size = menu->visible_items_num* max_height ;  /* important! */
    }else
        menu->visible_items_num = display_size / max_height ;

    /* setting up desired size  - may or maynot be overriden by user actions -
     * so we'll use ConfigureNotify events later on to keep ourselves up to date*/
    resize_canvas( menu->main_canvas, max_width, display_size );
    menu->optimal_width  = max_width ;
    menu->optimal_height = display_size ;

    if( menu->top_item > menu->items_num - menu->visible_items_num )
        menu->top_item = menu->items_num - menu->visible_items_num ;

    i = menu->items_num ;
    while ( --i >= 0 )
        set_astbar_size( menu->items[i].bar, max_width, max_height );
    ASSync(False);
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
        close_asmenu_submenu( menu );

    set_astbar_focused( menu->items[menu->selected_item].bar, menu->main_canvas, False );
    set_astbar_focused( menu->items[selection].bar, menu->main_canvas, True );
    menu->selected_item = selection ;

    if( selection < menu->top_item )
        set_asmenu_scroll_position( menu, MAX(selection-1, 0) );
    else if( selection >= menu->top_item + menu->visible_items_num )
        set_asmenu_scroll_position( menu, (selection-menu->visible_items_num)+1);
    else
        update_canvas_display( menu->main_canvas );
}

void
set_asmenu_scroll_position( ASMenu *menu, int pos )
{
    int curr_y = 0 ;
    int i ;

LOCAL_DEBUG_CALLER_OUT( "%p,%d", menu, pos );
    if( AS_ASSERT(menu) || menu->items_num == 0 )
        return;
    if( pos < 0 )
        pos = 0 ;
    else if( pos > (int)(menu->items_num) - (int)(menu->visible_items_num) )
        pos = (int)(menu->items_num) - (int)(menu->visible_items_num) ;

    curr_y =  ((int)(menu->items_num) - pos) * menu->item_height ;
LOCAL_DEBUG_OUT("adj_pos(%d)->curr_y(%d)->items_num(%d)->vis_items_num(%d)->sel_item(%d)", pos, curr_y, menu->items_num, menu->visible_items_num, menu->selected_item);
    i = menu->items_num ;
    while( --i >= 0 )
    {
        curr_y -= menu->item_height ;
        move_astbar( menu->items[i].bar, menu->main_canvas, 0, curr_y );
    }

    menu->top_item = pos ;
    if( menu->selected_item < menu->top_item )
        select_menu_item( menu, menu->top_item );
    else if( menu->selected_item >= menu->top_item + menu->visible_items_num )
        select_menu_item( menu, menu->top_item + menu->visible_items_num - 1 );
    else /* selection update canvas display just as well */
        update_canvas_display( menu->main_canvas );
    ASSync(False);
}

void
press_menu_item( ASMenu *menu, int pressed )
{
    Bool update_display = False ;
LOCAL_DEBUG_CALLER_OUT( "%p,%d", menu, pressed );

    if( AS_ASSERT(menu) || menu->items_num == 0 )
        return;

    if( pressed >= (int)menu->items_num )
        pressed = menu->items_num - 1 ;

    if( menu->pressed_item >= 0 )
    {
        set_astbar_pressed( menu->items[menu->pressed_item].bar, menu->main_canvas, False );
        update_display = True ;
    }
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
                update_display = False ;
            }else
            {
                set_astbar_pressed( menu->items[pressed].bar, menu->main_canvas, True );
                update_display = True ;
            }
            if( menu->items[pressed].submenu )
            {
                close_asmenu_submenu( menu );
                menu->submenu = run_submenu( menu, menu->items[pressed].submenu,
                                             menu->main_canvas->root_x+menu->item_width-5,
                                             menu->main_canvas->root_y+(menu->item_height*(pressed-(int)menu->top_item))-5 );
            }
        }
    }
    menu->pressed_item = pressed ;
    if( update_display )
        update_canvas_display( menu->main_canvas );
    ASSync(False);
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
                {
                    set_astbar_size(menu->items[i].bar, menu->main_canvas->width, menu->item_height);
                    render_astbar( menu->items[i].bar, menu->main_canvas );
                }
            }
            if( get_flags( changed, CANVAS_HEIGHT_CHANGED) )
            {
                menu->visible_items_num = menu->main_canvas->height / menu->item_height ;
                set_asmenu_scroll_position( menu, menu->top_item );
            }else
                update_canvas_display( menu->main_canvas );
        }else if( get_flags( changed, CANVAS_MOVED) )
        {
            while ( --i >= 0 )
            {
                update_astbar_root_pos(menu->items[i].bar, menu->main_canvas);
                render_astbar( menu->items[i].bar, menu->main_canvas );
            }
            update_canvas_display( menu->main_canvas );
            /* optionally update transparency */
        }
        ASSync(False);
    }
}


/* fwindow looses/gains focus : */
void
on_menu_hilite_changed( ASInternalWindow *asiw, ASMagic *data, Bool focused )
{
    ASMenu   *menu = (ASMenu*)(asiw->data) ;
    if( menu != NULL && menu->magic == MAGIC_ASMENU )
    {
        /* TODO : hilite/unhilite selected item, and
         * withdraw non-pinned menu if it has no submenus */
    }
}

/* ButtonPress/Release event on one of the contexts : */
void
on_menu_pressure_changed( ASInternalWindow *asiw, int pressed_context )
{
    ASMenu   *menu = (ASMenu*)(asiw->data) ;
LOCAL_DEBUG_CALLER_OUT( "%p,0x%X", asiw, pressed_context );
    if( menu != NULL && menu->magic == MAGIC_ASMENU )
    {
        /* press/depress menu item, possibly change the selection,
         * and run the function when item is depressed */
        if( pressed_context )
        {
            int px = 0, py = 0 ;
            GrabEm(&Scr, Scr.Feel.cursors[SELECT]);
            ASQueryPointerWinXY( menu->main_canvas->w, &px, &py );
LOCAL_DEBUG_OUT( "pointer(%d,%d)", px, py );
            if( px >= 0 && px < menu->main_canvas->width &&  py >= 0 && py < menu->main_canvas->height )
            {
                int pressed = py/menu->item_height ;
                if( pressed != menu->pressed_item )
                    press_menu_item( menu, pressed );
            }
        }else if( menu->pressed_item >= 0 )
        {
            UngrabEm(&Scr);
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
        int px = xmev->x_root - canvas->root_x, py = xmev->y_root - canvas->root_y;

        if( px >= 0 && px < canvas->width &&  py >= 0 && py < canvas->height )
        {
            int selection = py/menu->item_height ;
            if( selection != menu->selected_item )
            {
                if( xmev->state & ButtonAnyMask )
                    press_menu_item( menu, selection );
                else
                    select_menu_item( menu, selection );
            }
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
        if( get_flags( what, FEEL_CONFIG_CHANGED ) )
        {
            MenuData *md = FindPopup (menu->name, False);
            if( md == NULL )
            {
                menu_destroy( asiw );
                return ;
            }
            set_asmenu_data( menu, md );
        }
        set_asmenu_look( menu, look );
        set_asmenu_scroll_position( menu, 0 );
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

/*************************************************************************/
/* End of Menu event handlers:                                           */
/*************************************************************************/
void
show_asmenu(ASMenu *menu, int x, int y)
{
    ASStatusHints status ;
    ASHints *hints = safecalloc( 1, sizeof(ASHints) );
    ASInternalWindow *asiw = safecalloc( 1, sizeof(ASInternalWindow));

    asiw->data = (ASMagic*)menu;

    asiw->register_subwindows = menu_register_subwindows;
    asiw->on_moveresize = on_menu_moveresize;
    asiw->on_hilite_changed = on_menu_hilite_changed ;
    asiw->on_pressure_changed = on_menu_pressure_changed;
    asiw->on_pointer_event = on_menu_pointer_event;
    asiw->on_keyboard_event = on_menu_keyboard_event;
    asiw->on_look_feel_changed = on_menu_look_feel_changed;
    asiw->on_look_feel_changed = on_menu_look_feel_changed;
    asiw->destroy = menu_destroy;

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

    if( x <= MIN_MENU_X )
        x = MIN_MENU_X ;
    else if( x + menu->optimal_width > MAX_MENU_X )
        x = MAX_MENU_X - menu->optimal_width ;
    if( y <= MIN_MENU_Y )
        y = MIN_MENU_Y ;
    else if( y + menu->optimal_height > MAX_MENU_Y )
        y = MAX_MENU_Y - menu->optimal_height ;

    status.x = x;
    status.y = y;
    status.width = menu->optimal_width;
    status.height = menu->optimal_height;
    status.viewport_x = Scr.Vy;
    status.viewport_y = Scr.Vx;
    status.desktop = Scr.CurrentDesk;
    status.layer = AS_LayerMenu;

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
                   AS_SizeInc ;
    hints->protocols = AS_DoesWmTakeFocus ;
    hints->function_mask = ~(AS_FuncPopup|     /* everything else is allowed ! */
                             AS_FuncMinimize|
                             AS_FuncMaximize);

    hints->min_width  = menu->optimal_width ;
    hints->min_height = menu->item_height ;
    hints->max_width  = MAX_MENU_WIDTH ;
    hints->max_height = MIN(MAX_MENU_HEIGHT,menu->items_num*menu->item_height);
    hints->width_inc  = 0 ;
    hints->height_inc = menu->item_height;
    hints->gravity = NorthWestGravity ;/* for now - really should depend on where we were created at */
    hints->border_width = BW ;
    hints->handle_width = BOUNDARY_WIDTH;

    hints->frame_name = mystrdup("ASMenuFrame");
    hints->mystyle_names[BACK_FOCUSED] = mystrdup(Scr.Look.MSMenu[MENU_BACK_HILITE]->name);
    hints->mystyle_names[BACK_UNFOCUSED] = mystrdup(Scr.Look.MSMenu[MENU_BACK_TITLE]->name);
    hints->mystyle_names[BACK_STICKY] = mystrdup(Scr.Look.MSMenu[MENU_BACK_TITLE]->name);

    hints->disabled_buttons = 0;

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
        set_asmenu_data( menu, md );
        set_asmenu_look( menu, &Scr.Look );
        set_asmenu_scroll_position( menu, 0 );
        menu->supermenu = supermenu;
        show_asmenu(menu, x, y );
    }
    return menu;
}


void
run_menu( const char *name )
{
    MenuData *md;
    int x = 0, y = 0;

    close_asmenu(&ASTopmostMenu);

    if( (md = FindPopup (name, False)) == NULL )
        return;

    if( !ASQueryPointerRootXY(&x,&y) )
    {
        x = (Scr.MyDisplayWidth*2)/3;
        y = (Scr.MyDisplayHeight*3)/ 4;
    }
    ASTopmostMenu = run_submenu(NULL, md, x, y );
}

