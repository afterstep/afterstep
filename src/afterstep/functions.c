/****************************************************************************
 * Copyright (c) 2000,2001 Sasha Vasko <sasha at aftercode.net>
 * Copyright (c) 1999 Ethan Fisher <allanon@crystaltokyo.com>
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
 * This has been completely rewritten and as the result relicensed under GPL.
 * For historic purposes we keep original creators here :
 *
 * This module is based on Twm, but has been SIGNIFICANTLY modified
 * by Rob Nation
 * by Bo Yang
 * by Frank Fejes
 ****************************************************************************/
/***********************************************************************
 * afterstep function execution code
 ***********************************************************************/

#include "../../configure.h"

#define LOCAL_DEBUG

#include "../../include/asapp.h"
#include "../../libAfterImage/afterimage.h"

#include <limits.h>
#include <signal.h>

#include "../../include/afterstep.h"
#include "../../include/parse.h"
#include "../../include/parser.h"
#include "../../include/decor.h"
#include "../../include/event.h"
#include "../../include/screen.h"
#include "../../include/module.h"
#include "../../include/session.h"
#include "asinternals.h"
#include "moveresize.h"

static as_function_handler function_handlers[F_FUNCTIONS_NUM] ;

void ExecuteComplexFunction ( ASEvent *event, char *name );

/* list of available handlers : */
void beep_func_handler( FunctionData *data, ASEvent *event, int module );
void moveresize_func_handler( FunctionData *data, ASEvent *event, int module );
void scroll_func_handler( FunctionData *data, ASEvent *event, int module );
void movecursor_func_handler( FunctionData *data, ASEvent *event, int module );
void raiselower_func_handler( FunctionData *data, ASEvent *event, int module );
void setlayer_func_handler( FunctionData *data, ASEvent *event, int module );
void change_desk_func_handler( FunctionData *data, ASEvent *event, int module );
void toggle_status_func_handler( FunctionData *data, ASEvent *event, int module );
void iconify_func_handler( FunctionData *data, ASEvent *event, int module );
void focus_func_handler( FunctionData *data, ASEvent *event, int module );
void paste_selection_func_handler( FunctionData *data, ASEvent *event, int module );
void warp_func_handler( FunctionData *data, ASEvent *event, int module );
void goto_bookmark_func_handler( FunctionData *data, ASEvent *event, int module );
void bookmark_window_func_handler( FunctionData *data, ASEvent *event, int module );
void pin_menu_func_handler( FunctionData *data, ASEvent *event, int module );
void close_func_handler( FunctionData *data, ASEvent *event, int module );
void restart_func_handler( FunctionData *data, ASEvent *event, int module );
void exec_func_handler( FunctionData *data, ASEvent *event, int module );
void change_background_func_handler( FunctionData *data, ASEvent *event, int module );
void change_config_func_handler( FunctionData *data, ASEvent *event, int module );
void refresh_func_handler( FunctionData *data, ASEvent *event, int module );
void goto_page_func_handler( FunctionData *data, ASEvent *event, int module );
void toggle_page_func_handler( FunctionData *data, ASEvent *event, int module );
void gethelp_func_handler( FunctionData *data, ASEvent *event, int module );
void wait_func_handler( FunctionData *data, ASEvent *event, int module );
void raise_it_func_handler( FunctionData *data, ASEvent *event, int module );
void desk_func_handler( FunctionData *data, ASEvent *event, int module );
void module_func_handler( FunctionData *data, ASEvent *event, int module );
void killmodule_func_handler( FunctionData *data, ASEvent *event, int module );
void popup_func_handler( FunctionData *data, ASEvent *event, int module );
void quit_func_handler( FunctionData *data, ASEvent *event, int module );
void windowlist_func_handler( FunctionData *data, ASEvent *event, int module );
void quickrestart_func_handler( FunctionData *data, ASEvent *event, int module );
void send_window_list_func_handler( FunctionData *data, ASEvent *event, int module );
void test_func_handler( FunctionData *data, ASEvent *event, int module );

/* handlers initialization function : */
void SetupFunctionHandlers()
{
    memset( &(function_handlers[0]), 0x00, sizeof( function_handlers ) );
    function_handlers[F_BEEP] = beep_func_handler ;

    function_handlers[F_RESIZE] =
        function_handlers[F_MOVE] = moveresize_func_handler ;

#ifndef NO_VIRTUAL
    function_handlers[F_SCROLL] = scroll_func_handler ;
    function_handlers[F_MOVECURSOR] = movecursor_func_handler ;
#endif

    function_handlers[F_RAISE] =
        function_handlers[F_LOWER] =
        function_handlers[F_RAISELOWER]     = raiselower_func_handler;

    function_handlers[F_PUTONTOP] =
        function_handlers[F_PUTONBACK] =
        function_handlers[F_TOGGLELAYER]=
        function_handlers[F_SETLAYER]       = setlayer_func_handler   ;

    function_handlers[F_CHANGE_WINDOWS_DESK]= change_desk_func_handler;

    function_handlers[F_MAXIMIZE]           =
        function_handlers[F_SHADE]          =
        function_handlers[F_STICK]          = toggle_status_func_handler;

    function_handlers[F_ICONIFY]            = iconify_func_handler;

    function_handlers[F_FOCUS]              = focus_func_handler;
    function_handlers[F_CHANGEWINDOW_UP] =
        function_handlers[F_WARP_F] =
        function_handlers[F_CHANGEWINDOW_DOWN]  =
        function_handlers[F_WARP_B]         = warp_func_handler ;

    function_handlers[F_PASTE_SELECTION]    = paste_selection_func_handler ;
    function_handlers[F_GOTO_BOOKMARK]      = goto_bookmark_func_handler ;
    function_handlers[F_BOOKMARK_WINDOW]    = bookmark_window_func_handler ;
    function_handlers[F_PIN_MENU]           = pin_menu_func_handler ;
    function_handlers[F_DESTROY] =
        function_handlers[F_DELETE] =
        function_handlers[F_CLOSE]          = close_func_handler ;
    function_handlers[F_RESTART]            = restart_func_handler ;

    function_handlers[F_EXEC] =
        function_handlers[F_Swallow] =
        function_handlers[F_MaxSwallow] =
        function_handlers[F_DropExec]       = exec_func_handler ;

    function_handlers[F_CHANGE_BACKGROUND]  = change_background_func_handler;

    function_handlers[F_CHANGE_LOOK] =
        function_handlers[F_CHANGE_FEEL]    =
        function_handlers[F_CHANGE_THEME]   = change_config_func_handler ;

    function_handlers[F_REFRESH]            = refresh_func_handler ;
#ifndef NO_VIRTUAL
    function_handlers[F_GOTO_PAGE]          = goto_page_func_handler ;
    function_handlers[F_TOGGLE_PAGE]        = toggle_page_func_handler ;
#endif
    function_handlers[F_GETHELP]            = gethelp_func_handler ;
    function_handlers[F_WAIT]               = wait_func_handler ;
    function_handlers[F_RAISE_IT]           = raise_it_func_handler ;
    function_handlers[F_DESK]               = desk_func_handler ;

    function_handlers[F_MODULE] =
        function_handlers[F_SwallowModule] =
        function_handlers[F_MaxSwallowModule]= module_func_handler ;

    function_handlers[F_KILLMODULEBYNAME]   = killmodule_func_handler ;
    function_handlers[F_POPUP]              = popup_func_handler ;
    function_handlers[F_QUIT]               = quit_func_handler ;
#ifndef NO_WINDOWLIST
    function_handlers[F_WINDOWLIST]         = windowlist_func_handler ;
#endif /* ! NO_WINDOWLIST */
    function_handlers[F_QUICKRESTART]       = quickrestart_func_handler ;
    function_handlers[F_SEND_WINDOW_LIST]   = send_window_list_func_handler ;
    function_handlers[F_Test]               = test_func_handler ;
}

/* complex functions are stored in hash table ComplexFunctions */
ComplexFunction *
get_complex_function( char *name )
{
    return find_complex_func( Scr.Feel.ComplexFunctions, name );
}

void
print_func_data(const char *file, const char *func, int line, FunctionData *data)
{
    fprintf( stderr, "%s:%s:%s:%d>!!FUNC ", get_application_name(), file, func, line);
    if( data == NULL )
        fprintf( stderr, "NULL Function\n");
    else
    {
        TermDef      *term = func2fterm (data->func, True);
        if( term == NULL )
            fprintf( stderr, "Invalid Function %d\n", data->func);
        else
        {
            fprintf( stderr, "%s \"%s\" %s ", term->keyword, data->name?data->name:"", data->text?data->text:"" );
            if( data->text == NULL )
            {
                fprintf( stderr, "%ld%c ", data->func_val[0], (data->unit[0]=='\0')?' ':data->unit[0] );
                fprintf( stderr, "%ld%c ", data->func_val[0], (data->unit[0]=='\0')?' ':data->unit[0] );
            }
            fprintf( stderr, "(popup=%p)\n", data->popup );
        }
    }
}

/***********************************************************************
 *  Procedure:
 *	ExecuteFunction - execute a afterstep built in function
 *  Inputs:
 *      data    - the function to execute
 *      event   - the event that caused the function
 *      module  - number of the module we received function request from
 ***********************************************************************/
#undef ExecuteFunction
void
ExecuteFunction (FunctionData *data, ASEvent *event, int module)
#ifdef TRACE_ExecuteFunction
#define ExecuteFunction(d,e,m) trace_ExecuteFunction(d,e,m,__FILE__,__LINE__)
#endif
{
    register FunctionCode  func = data->func;
    ASEvent scratch_event ;
#ifndef LOCAL_DEBUG
    if( get_output_threshold() >= OUTPUT_LEVEL_DEBUG )
#endif
        print_func_data(__FILE__, "ExecuteFunction", __LINE__, data);
LOCAL_DEBUG_CALLER_OUT( "event(%d(%s))->window(%lX)->client(%p(%s))->module(%d)",
                         event?event->x.type:-1,
                         event?event_type2name(event->x.type):"n/a",
                         event?(unsigned long)event->w:0,
                         event?event->client:NULL,
                         event?(event->client?ASWIN_NAME(event->client):"none"):"none",
                         module );
    if( event == NULL )
    {
        memset( &scratch_event, 0x00, sizeof(ASEvent) );
        event = &scratch_event ;
        event->event_time = Scr.last_Timestamp ;
        event->scr = &Scr ;
    }
    /* Defer Execution may wish to alter this value */
    if (IsWindowFunc (func))
	{
		int           cursor, fin_event;

        if (func != F_RESIZE && func != F_MOVE)
		{
			fin_event = ButtonRelease;
            cursor = (func!=F_DESTROY && func!=F_DELETE && func!=F_CLOSE)?SELECT:DESTROY;
        } else
		{
			cursor = MOVE;
			fin_event = ButtonPress;
		}

        if (data->text != NULL && event->client == NULL)
            if (*(data->text) != '\0')
                if ((event->client = pattern2ASWindow (data->text)) != NULL)
                    event->w = get_window_frame(event->client);

        if (DeferExecution (event, cursor, fin_event))
            func = F_NOP;
        else if (event->client == NULL)
            func = F_NOP;
	}

    if( function_handlers[func] || func == F_FUNCTION )
    {
        data->func = func ;
        if( event->client )
            if( !check_allowed_function2( data->func, event->client->hints) )
            {
LOCAL_DEBUG_OUT( "function \"%s\" is not allowed for the specifyed window (mask 0x%lX)", COMPLEX_FUNCTION_NAME(data), ASWIN_FUNC_MASK(event->client));
                XBell (dpy, Scr.screen);
                return ;
            }

        if( get_flags( AfterStepState, ASS_WarpingMode ) &&
            function_handlers[func] != warp_func_handler )
            EndWarping();

		if( func == F_FUNCTION )
            ExecuteComplexFunction (event, COMPLEX_FUNCTION_NAME(data));
		else
            function_handlers[func]( data, event, module );
    }
    /* Only wait for an all-buttons-up condition after calls from
	 * regular built-ins, not from complex-functions or modules.
	 *
	 *   Don't really know why but what the hell - let's honor
	 *   heritage
	 *                  Sasha Vasko.
	 */
#if 0
    if (func != F_FUNCTION && func != F_POPUP &&
	    function_handlers[func] != module_func_handler )
		WaitForButtonsUpLoop ();
#endif
}

/***********************************************************************
 *  Procedure:
 *	DeferExecution - defer the execution of a function to the
 *	    next button press if the context is ASC_Root
 *
 *  Inputs:
 *      eventp  - pointer to ASEvent to patch up
 *      cursor  - the cursor to display while waiting
 *      finish_event - ButtonRelease or ButtonPress; tells what kind of event to
 *                     terminate on.
 * Returns:
 *      True    - if we should defer execution of the function
 *      False   - if we can continue as planned
 ***********************************************************************/
int
DeferExecution ( ASEvent *event, int cursor, int finish_event)
{
    Bool res = False ;
LOCAL_DEBUG_CALLER_OUT( "cursor %d, event %d, window 0x%lX, window_name \"%s\", finish event %d",
                        cursor, event?event->x.type:-1, event?(unsigned long)event->w:0, event->client?ASWIN_NAME(event->client):"none", finish_event );

    if (event->context != C_ROOT && event->context != C_NO_CONTEXT)
        if ( finish_event == ButtonPress ||
            (finish_event == ButtonRelease && event->x.type != ButtonPress))
			return False;

    if (!(res = !GrabEm (&Scr, Scr.Feel.cursors[cursor])))
	{
        WaitEventLoop( event, finish_event, -1 );
        if (event->client == NULL)
            res = True ;
        UngrabEm ();
    }
    if( res )
        XBell (dpy, event->scr->screen);

LOCAL_DEBUG_OUT( "result %d, event %d, window 0x%lX, window_name \"%s\"",
                  res, event?event->x.type:-1, event?(unsigned long)event->w:0, event->client?ASWIN_NAME(event->client):"none" );

    return res;
}

/*****************************************************************************
 *  Executes complex function defined with Function/EndFunction construct :
 ****************************************************************************/
void
ExecuteComplexFunction ( ASEvent *event, char *name )
{
    ComplexFunction *func ;
    int              clicks = 0 ;
    register int     i ;
    Bool             persist = False;
    Bool             need_window = False;
    char             c ;
    static char clicks_upper[MAX_CLICKS_HANDLED+1] = {CLICKS_TRIGGERS_UPPER};
    static char clicks_lower[MAX_CLICKS_HANDLED+1] = {CLICKS_TRIGGERS_LOWER};
    LOCAL_DEBUG_CALLER_OUT( "event %d, window 0x%lX, window_name \"%s\", function name \"%s\"",
                        event?event->x.type:-1, event?(unsigned long)event->w:0, event->client?ASWIN_NAME(event->client):"none", name);

    if( (func = get_complex_function( name ) ) == NULL )
        return ;
    LOCAL_DEBUG_OUT( "function data found  %p ...\n   proceeding to execution of immidiate items...", func );
    if( event->w == None && event->client != NULL )
        event->w = get_window_frame(event->client);
    /* first running all the Imediate actions : */
    for( i = 0 ; i < func->items_num ; i++ )
    {
        c = func->items[i].name ? *(func->items[i].name): 'i' ;
        if( c == IMMEDIATE || c == IMMEDIATE_UPPER )
        {
            ExecuteFunction (&(func->items[i]), event, -1);
        }else
        {
            persist = True ;
            if( IsWindowFunc( func->items[i].func ) )
                need_window = True ;
        }
    }
    /* now lets count clicks : */
    LOCAL_DEBUG_OUT( "done with immidiate items - persisting ?: %s", persist?"True":"False" );
    if( !persist )
        return ;

    if (need_window)
    {
        if (DeferExecution (event, SELECT, ButtonPress))
		{
//            WaitForButtonsUpLoop ();
			return;
		}
    }
    if (!GrabEm (&Scr, Scr.Feel.cursors[SELECT]))
	{
        show_warning( "failed to grab pointer while executing complex function \"%s\"", name );
        XBell (dpy, Scr.screen);
		return;
	}

    clicks = 0 ;
    while( IsClickLoop( event, ButtonReleaseMask, Scr.Feel.ClickTime ) )
    {
        clicks++ ;
        if( !IsClickLoop( event, ButtonPressMask, Scr.Feel.ClickTime ) )
            break;
        clicks++ ;
    }
    if( clicks <= MAX_CLICKS_HANDLED )
    {
        /* some functions operate on button release instead of
         * presses. These gets really weird for complex functions ... */
        if (event->x.type == ButtonPress)
            event->x.type = ButtonRelease;
        /* first running all the Imediate actions : */
        for( i = 0 ; i < func->items_num ; i++ )
            if( func->items[i].name )
            {
                c = func->items[i].name[0];
                if( c == clicks_upper[clicks] || c == clicks_lower[clicks] )
                    ExecuteFunction (&(func->items[i]), event, -1);
            }
    }
//    WaitForButtonsUpLoop ();
	UngrabEm ();
}


/***************************************************************************************
 *
 * All The Handlers Go Below, Go Below,
 *                              Go Below.
 * All The Handlers Go Below, Go Below, Go Below
 * Ta-da-da-da-da-da-da
 *
 ****************************************************************************************/

void beep_func_handler( FunctionData *data, ASEvent *event, int module )
{
    XBell (dpy, event->scr->screen);
}

void apply_aswindow_move(struct ASMoveResizeData *data)
{
    ASWindow *asw = window2ASWindow( AS_WIDGET_WINDOW(data->mr));
SHOW_CHECKPOINT;
LOCAL_DEBUG_OUT( "%dx%d%+d%+d", data->curr.x, data->curr.y, data->curr.width, data->curr.height);
    if( ASWIN_GET_FLAGS( asw, AS_Shaded ) )
        moveresize_aswindow_wm( asw,
                                data->curr.x, data->curr.y,
                                asw->status->width, asw->status->height);
    else
        moveresize_aswindow_wm( asw,
                                data->curr.x, data->curr.y,
                                data->curr.width, data->curr.height);
}


void complete_aswindow_move(struct ASMoveResizeData *data, Bool cancelled)
{
    ASWindow *asw = window2ASWindow( AS_WIDGET_WINDOW(data->mr));

	if( cancelled )
	{
SHOW_CHECKPOINT;
        LOCAL_DEBUG_OUT( "%dx%d%+d%+d", data->start.x, data->start.y, data->start.width, data->start.height);
        moveresize_aswindow_wm( asw, data->start.x, data->start.y, data->start.width, data->start.height );
	}else
	{
SHOW_CHECKPOINT;
        LOCAL_DEBUG_OUT( "%dx%d%+d%+d", data->curr.x, data->curr.y, data->curr.width, data->curr.height);
        moveresize_aswindow_wm( asw, data->curr.x, data->curr.y, data->curr.width, data->curr.height );
	}
    Scr.moveresize_in_progress = NULL ;
}

struct ASWindowGridAuxData{
    ASGrid *grid;
    long desk;
};

static void
add_canvas_grid( ASGrid *grid, ASCanvas *canvas, int outer_gravity, int inner_gravity )
{
    if( canvas )
    {
LOCAL_DEBUG_CALLER_OUT( "(%p,%ux%u%+d%+d)", canvas, canvas->width, canvas->height, canvas->root_x, canvas->root_y );
        add_gridline( &(grid->h_lines), canvas->root_y,                canvas->root_x, canvas->root_x+canvas->width,  outer_gravity, inner_gravity );
        add_gridline( &(grid->h_lines), canvas->root_y+canvas->height, canvas->root_x, canvas->width+canvas->root_x,  inner_gravity, outer_gravity );
        add_gridline( &(grid->v_lines), canvas->root_x,                canvas->root_y, canvas->height+canvas->root_y, outer_gravity, inner_gravity );
        add_gridline( &(grid->v_lines), canvas->root_x+canvas->width,  canvas->root_y, canvas->height+canvas->root_y, inner_gravity, outer_gravity );
    }
}

Bool
get_aswindow_grid_iter_func(void *data, void *aux_data)
{
    ASWindow *asw = (ASWindow*)data ;
    struct ASWindowGridAuxData *grid_data = (struct ASWindowGridAuxData*)aux_data;

    if( asw && ASWIN_DESK(asw) == grid_data->desk )
    {
        int outer_gravity = Scr.Feel.EdgeAttractionWindow ;
        int inner_gravity = Scr.Feel.EdgeAttractionWindow ;
        if( ASWIN_HFLAGS(asw, AS_AvoidCover) )
            inner_gravity = -1 ;
        else if( inner_gravity == 0 )
            return True;

        if( ASWIN_GET_FLAGS(asw, AS_Iconic ) )
        {
            add_canvas_grid( grid_data->grid, asw->icon_canvas, outer_gravity, inner_gravity );
            if( asw->icon_canvas != asw->icon_title_canvas )
                add_canvas_grid( grid_data->grid, asw->icon_title_canvas, outer_gravity, inner_gravity );
        }else
        {
            add_canvas_grid( grid_data->grid, asw->frame_canvas, outer_gravity, inner_gravity );
            add_canvas_grid( grid_data->grid, asw->client_canvas, outer_gravity/2, (inner_gravity*2)/3 );
        }
    }
    return True;
}

ASGrid*
make_desktop_grid(int desk)
{
    struct ASWindowGridAuxData grid_data ;
    int resist = Scr.Feel.EdgeResistanceMove ;
    int attract = Scr.Feel.EdgeAttractionScreen ;

    grid_data.desk = desk ;
    grid_data.grid = safecalloc( 1, sizeof(ASGrid));
    add_canvas_grid( grid_data.grid, Scr.RootCanvas, resist, attract );
    /* add all the window edges for this desktop : */
    iterate_asbidirlist( Scr.Windows->clients, get_aswindow_grid_iter_func, (void*)&grid_data, NULL, False );

#ifdef LOCAL_DEBUG
    print_asgrid( grid_data.grid );
#endif

    return grid_data.grid;
}


void moveresize_func_handler( FunctionData *data, ASEvent *event, int module )
{   /* gotta have a window */
    ASWindow *asw = event->client ;
    if (asw == NULL)
		return;

    if ( data->func_val[0] != INVALID_POSITION || data->func_val[1] != INVALID_POSITION)
    {
        int new_val1, new_val2 ;
        int x = asw->status->x ;
        int y = asw->status->y ;
        int width = asw->status->width ;
        int height = asw->status->height ;

        new_val1 = APPLY_VALUE_UNIT(Scr.MyDisplayWidth,data->func_val[0],data->unit_val[0]);
        new_val2 = APPLY_VALUE_UNIT(Scr.MyDisplayHeight,data->func_val[1],data->unit_val[1]);
        if( data->func == F_MOVE )
        {
            x = new_val1;
            y = new_val2;
        }else
        {
            width = new_val1;
            height = new_val2;
        }
        moveresize_aswindow_wm( asw, x, y, width, height );
    }else
    {
        ASMoveResizeData *mvrdata;
        release_pressure();
        if( data->func == F_MOVE )
            mvrdata = move_widget_interactively(Scr.RootCanvas,
                                                asw->frame_canvas,
                                                event,
                                                apply_aswindow_move,
                                                complete_aswindow_move );
        else
        {
            int side = 0 ;
            register unsigned long context = event->context;

            if( ASWIN_GET_FLAGS( asw, AS_Shaded ) )
            {
                XBell (dpy, Scr.screen);
                return;
            }

            while( (0x01&context) == 0 )
            {
                ++side ;
                context = context>>1 ;
            }
            mvrdata = resize_widget_interactively(  Scr.RootCanvas,
                                                    asw->frame_canvas,
                                                    event,
                                                    apply_aswindow_move,
                                                    complete_aswindow_move,
                                                    side );
        }
        if( mvrdata )
        {
            raise_scren_panframes( &Scr );
            mvrdata->below_sibling = get_lowest_panframe(&Scr);
            set_moveresize_restrains( mvrdata, asw->hints, asw->status);
//            mvrdata->subwindow_func = on_deskelem_move_subwindow ;
            mvrdata->grid = make_desktop_grid(Scr.CurrentDesk);
            Scr.moveresize_in_progress = mvrdata ;
        }
    }
}


static inline int
make_scroll_pos( int val, int unit, int curr, int max, int size )
{
    int pos = curr ;
    if ( val > -100000 && val < 100000 )
    {
        pos += APPLY_VALUE_UNIT(size,val,unit);
        if( pos < 0 )
            pos = 0 ;
        if( pos > max )
            pos = max ;
    }else
    {
        pos += APPLY_VALUE_UNIT(size,val/1000, unit);
        while( pos < 0 )
            pos += max ;
        while( pos > max )
            pos -= max ;
    }
    return pos;
}

static inline int
make_edge_scroll( int curr_pos, int curr_view, int view_size, int max_view, int edge_scroll )
{
    int new_view = curr_view;
	edge_scroll = (edge_scroll*view_size)/100 ;
    if( curr_pos >= new_view+view_size - 2 )
    {
        while ( curr_pos >= new_view+view_size - 2)
            new_view += edge_scroll ;
        if( new_view > max_view )
            new_view = max_view ;
    }else
    {
        while ( curr_pos < new_view+2)
            new_view -= edge_scroll ;
        if( new_view < 0 )
            new_view = 0 ;
    }
    return new_view;
}

void scroll_func_handler( FunctionData *data, ASEvent *event, int module )
{
#ifndef NO_VIRTUAL
    int x, y ;
	register ScreenInfo *scr = event->scr;

    x = make_scroll_pos( data->func_val[0], data->unit_val[0], scr->Vx, scr->VxMax, scr->MyDisplayWidth );
    y = make_scroll_pos( data->func_val[1], data->unit_val[1], scr->Vy, scr->VyMax, scr->MyDisplayHeight );

    MoveViewport ( x, y, True);
#endif
}

void movecursor_func_handler( FunctionData *data, ASEvent *event, int module )
{
    int curr_x, curr_y ;
    int x, y ;
    register ScreenInfo *scr = &Scr;

    ASQueryPointerRootXY(&curr_x, &curr_y);
    x = make_scroll_pos( data->func_val[0], data->unit_val[0], scr->Vx+curr_x, scr->VxMax+scr->MyDisplayWidth ,scr->MyDisplayWidth );
    y = make_scroll_pos( data->func_val[1], data->unit_val[1], scr->Vy+curr_y, scr->VyMax+scr->MyDisplayHeight,scr->MyDisplayHeight);

#ifndef NO_VIRTUAL
    {
        int new_vx = 0, new_vy = 0;

        new_vx = make_edge_scroll( x, scr->Vx, scr->MyDisplayWidth,  scr->VxMax, scr->Feel.EdgeScrollX );
        new_vy = make_edge_scroll( y, scr->Vy, scr->MyDisplayHeight, scr->VyMax, scr->Feel.EdgeScrollY );
        if( new_vx != scr->Vx || new_vy != scr->Vy )
            MoveViewport (new_vx, new_vy, True);
    }
#endif
    x -= scr->Vx ;
    y -= scr->Vy ;
    if( x != curr_x || y != curr_y )
        XWarpPointer (dpy, scr->Root, scr->Root, 0, 0, scr->MyDisplayWidth,
                      scr->MyDisplayHeight, x, y);
}

void iconify_func_handler( FunctionData *data, ASEvent *event, int module )
{
LOCAL_DEBUG_CALLER_OUT( "function %d (val0 = %ld), event %d, window 0x%lX, window_name \"%s\", module %d",
                        data?data->func:0, data?data->func_val[0]:0, event?event->x.type:-1, event?(unsigned long)event->w:0, event->client?ASWIN_NAME(event->client):"none", module );
    if (ASWIN_GET_FLAGS(event->client, AS_Iconic) )
    {
        if (data->func_val[0] <= 0)
            iconify_window( event->client, False );
    }else if (data->func_val[0] >= 0)
        iconify_window( event->client, True );
}

void raiselower_func_handler( FunctionData *data, ASEvent *event, int module )
{
    restack_window (event->client,None,(data->func==F_RAISE)?Above:
                                       ((data->func==F_RAISELOWER)?Opposite:Below));
}

void raise_it_func_handler( FunctionData *data, ASEvent *event, int module )
{
    activate_aswindow( (ASWindow *) (data->func_val[0]), True, True );
}

void setlayer_func_handler( FunctionData *data, ASEvent *event, int module )
{
    register int func = data->func ;
    int layer = 0 ;

    if( func == F_PUTONTOP )
        layer = AS_LayerTop ;
    else if( func == F_PUTONBACK )
        layer = AS_LayerBack ;
    else
        layer = (func == F_TOGGLELAYER && ASWIN_LAYER(event->client))?data->func_val[1]: data->func_val[0] ;
    change_aswindow_layer( event->client, layer );
}

void change_desk_func_handler( FunctionData *data, ASEvent *event, int module )
{
    if( event->client )
        change_aswindow_desktop( event->client, data->func_val[0] );
}

void toggle_status_func_handler( FunctionData *data, ASEvent *event, int module )
{
    ASFlagType toggle_flags = 0;
    if( data->func == F_STICK )
        toggle_flags = AS_Sticky ;
    else if( data->func == F_MAXIMIZE )
        toggle_flags = AS_MaximizedX|AS_MaximizedY ;
    else if( data->func == F_SHADE )
        toggle_flags = AS_Shaded ;
    else
        return ;
    toggle_aswindow_status( event->client, toggle_flags );
}

void focus_func_handler( FunctionData *data, ASEvent *event, int module )
{
    activate_aswindow( event->client, True, False );
}

void warp_func_handler( FunctionData *data, ASEvent *event, int module )
{
    register ASWindow *t = NULL;

    if (data->text != NULL )
        if (*(data->text) != '\0')
            t = pattern2ASWindow (data->text);
    if( t == NULL )
        t = warp_aswindow_list (Scr.Windows, (data->func == F_CHANGEWINDOW_DOWN ||
                                       data->func == F_WARP_B));
    if ( t != NULL)
    {
        event->client = t;
        event->w = get_window_frame(t);
        StartWarping(&Scr, Scr.Feel.cursors[SELECT]);
        warp_to_aswindow(t, (data->func == F_WARP_F || data->func == F_WARP_B));
    }
}

void paste_selection_func_handler( FunctionData *data, ASEvent *event, int module )
{
    PasteSelection (event->scr);
}

//#warning implement window bookmarking.
void goto_bookmark_func_handler( FunctionData *data, ASEvent *event, int module )
{
}

void pin_menu_func_handler( FunctionData *data, ASEvent *event, int module )
{
    ASMenu *menu = NULL;
    if( data->name )
        menu = find_asmenu( data->name );
    else if( event->client && event->client->internal )
    {
        ASMagic *data = event->client->internal->data ;
        if( data->magic == MAGIC_ASMENU )
            menu = (ASMenu*)data;
    }
    if( menu == NULL )
        XBell (dpy, event->scr->screen);
    else
        pin_asmenu( menu );
}


void bookmark_window_func_handler( FunctionData *data, ASEvent *event, int module )
{
}

void close_func_handler( FunctionData *data, ASEvent *event, int module )
{
	Window w = event->client->w ;

	if ( get_flags(event->client->hints->protocols, AS_DoesWmDeleteWindow) &&
		 data->func != F_DESTROY)
		send_wm_protocol_request(w, _XA_WM_DELETE_WINDOW, CurrentTime);
	else
	{
        if( event->client->internal != NULL || validate_drawable(w, NULL, NULL) == None)
            Destroy (event->client, True);
        else if (data->func == F_DELETE )
            XBell (dpy, event->scr->screen);
        else
			XKillClient (dpy, w);
		XSync (dpy, 0);
	}
}

void restart_func_handler( FunctionData *data, ASEvent *event, int module )
{
    Done (True, data->text);
}

void exec_func_handler( FunctionData *data, ASEvent *event, int module )
{
    XGrabPointer( dpy, Scr.Root, True,
	   		      ButtonPressMask | ButtonReleaseMask,
                  GrabModeAsync, GrabModeAsync, Scr.Root, Scr.Feel.cursors[WAIT], CurrentTime);
    XSync (dpy, 0);
    spawn_child( data->text, -1, -1, None, C_NO_CONTEXT, True, False, NULL );
    XUngrabPointer (dpy, CurrentTime);
    XSync (dpy, 0);
}

void change_background_func_handler( FunctionData *data, ASEvent *event, int module )
{
    char tmpfile[256], *realfilename ;

    XGrabPointer (dpy, Scr.Root, True, ButtonPressMask | ButtonReleaseMask,
                  GrabModeAsync, GrabModeAsync, Scr.Root, Scr.Feel.cursors[WAIT], CurrentTime);
    XSync (dpy, 0);

    if (Scr.screen == 0)
        sprintf (tmpfile, BACK_FILE, Scr.CurrentDesk);
    else
        sprintf (tmpfile, BACK_FILE ".scr%ld", Scr.CurrentDesk, Scr.screen);

    realfilename = make_session_data_file(Session, False, 0, tmpfile, NULL );
    if (CopyFile (data->text, realfilename) == 0)
        SendPacket( -1, M_NEW_BACKGROUND, 1, 1);

    free (realfilename);
    XUngrabPointer (dpy, CurrentTime);
    XSync (dpy, 0);
}

void change_config_func_handler( FunctionData *data, ASEvent *event, int module )
{
    char *file_template ;
    char tmpfile[256], *realfilename = NULL ;
    int desk = 0 ;
#ifdef DIFFERENTLOOKNFEELFOREACHDESKTOP
    desk = Scr.CurrentDesk ;
#endif
    if (Scr.screen == 0)
    {
        file_template = (data->func == F_CHANGE_LOOK)? LOOK_FILE : ((data->func == F_CHANGE_THEME)?THEME_FILE:FEEL_FILE) ;
        sprintf (tmpfile, file_template, desk, Scr.d_depth);
    }else
    {
        file_template =  (data->func == F_CHANGE_LOOK )? LOOK_FILE  ".scr%ld" :
                        ((data->func == F_CHANGE_THEME)? THEME_FILE ".scr%ld" :
                                                         FEEL_FILE  ".scr%ld" );
        sprintf (tmpfile, file_template, desk, Scr.d_depth, Scr.screen);
    }

    realfilename = make_session_data_file(Session, False, 0, tmpfile, NULL );
    if (CopyFile (data->text, realfilename) == 0)
    {
        if( data->func == F_CHANGE_THEME )
            QuickRestart ("look&feel");
        else
            QuickRestart ((data->func == F_CHANGE_LOOK)?"look":"feel");
    }
}

void refresh_func_handler( FunctionData *data, ASEvent *event, int module )
{
    XSetWindowAttributes attributes;
	unsigned long valuemask;
	Window        w;

	valuemask = (CWBackPixmap | CWBackingStore | CWOverrideRedirect);
	attributes.background_pixmap = None;
	attributes.backing_store = NotUseful;
	attributes.override_redirect = True;

    w = create_visual_window(Scr.asv, Scr.Root, 0, 0,
                               Scr.MyDisplayWidth, Scr.MyDisplayHeight,
                               0, InputOutput, valuemask, &attributes);

	XMapRaised (dpy, w);
    XSync (dpy, False);
    XDestroyWindow (dpy, w);
    XFlush (dpy);
}

void goto_page_func_handler( FunctionData *data, ASEvent *event, int module )
{
#ifndef NO_VIRTUAL
    MoveViewport ( data->func_val[0]*event->scr->MyDisplayWidth, data->func_val[1] * event->scr->MyDisplayHeight, True);
#endif
}

void toggle_page_func_handler( FunctionData *data, ASEvent *event, int module )
{
#ifndef NO_VIRTUAL
    if( get_flags( Scr.Feel.flags, DoHandlePageing ) )
        clear_flags( Scr.Feel.flags, DoHandlePageing );
	else
        set_flags( Scr.Feel.flags, DoHandlePageing );

    SendPacket( -1, M_TOGGLE_PAGING, 1, get_flags( Scr.Feel.flags, DoHandlePageing ));
    check_screen_panframes(&Scr);
#endif
}

void gethelp_func_handler( FunctionData *data, ASEvent *event, int module )
{
	if (event->client != NULL)
  		if (ASWIN_RES_NAME(event->client)!= NULL)
		{
      		char         *realfilename = PutHome(HELPCOMMAND);
            XGrabPointer (dpy, Scr.Root, True,
                          ButtonPressMask | ButtonReleaseMask,
                          GrabModeAsync, GrabModeAsync, Scr.Root, Scr.Feel.cursors[WAIT], CurrentTime);
            XSync (dpy, 0);
            spawn_child( realfilename, -1, -1, None, C_NO_CONTEXT, True, False, ASWIN_RES_NAME(event->client), NULL);
            free (realfilename);
            XUngrabPointer (dpy, CurrentTime);
            XSync (dpy, 0);
        }
}

void wait_func_handler( FunctionData *data, ASEvent *event, int module )
{
	WaitWindowLoop( data->text, -1 );
	XSync (dpy, 0);
}

void desk_func_handler( FunctionData *data, ASEvent *event, int module )
{
    long new_desk ;

    if ( data->func_val[0] != 0 && !IsValidDesk (data->func_val[0]))
        new_desk = Scr.CurrentDesk + data->func_val[0];
	else
        new_desk = data->func_val[1];

    ChangeDesks (new_desk);
}

void module_func_handler( FunctionData *data, ASEvent *event, int module )
{
    UngrabEm ();
    ExecModule (data->text, event->client ? event->client->w : None, event->context);
}

void killmodule_func_handler( FunctionData *data, ASEvent *event, int module )
{
    KillModuleByName (data->text);
}

void popup_func_handler( FunctionData *data, ASEvent *event, int module )
{
    run_menu( data->name );
    /* TODO : implement menus : */
}

void quit_func_handler( FunctionData *data, ASEvent *event, int module )
{
    Done (0, NULL);
}

void windowlist_func_handler( FunctionData *data, ASEvent *event, int module )
{
#ifndef NO_WINDOWLIST
    /* TODO : implement menus : */
    /*do_menu_new( event->scr, make_window_list_name(data->text == NULL ? event->scr->CurrentDesk: data->func_val[0]), event->client, event->context, windowlist_menu);*/
#endif /* ! NO_WINDOWLIST */
}

void quickrestart_func_handler( FunctionData *data, ASEvent *event, int module )
{
    QuickRestart (data->text);
}

Bool
send_aswindow_data_iter_func(void *data, void *aux_data)
{
    int module = (int)aux_data ;
    ASWindow *asw = (ASWindow *)data ;

    SendConfig (module, M_CONFIGURE_WINDOW, asw);
    SendString (module, M_WINDOW_NAME, asw->w,asw->frame,(unsigned long)asw, asw->hints->names[0]);
    SendString (module, M_ICON_NAME, asw->w, asw->frame, (unsigned long)asw, asw->hints->icon_name);
    SendString (module, M_RES_CLASS, asw->w, asw->frame, (unsigned long)asw, asw->hints->res_class);
    SendString (module, M_RES_NAME,  asw->w, asw->frame, (unsigned long)asw, asw->hints->res_name);

    if (ASWIN_GET_FLAGS(asw,AS_Iconic))
    {
        ASCanvas *ic = asw->icon_canvas?asw->icon_canvas:asw->icon_title_canvas ;
        if( ic != NULL )
            SendPacket (module, M_ICONIFY, 7, asw->w, asw->frame,
                        (unsigned long)asw, ic->root_x, ic->root_y, ic->width, ic->height);
        else
            SendPacket (module, M_ICONIFY, 7, asw->w, asw->frame, (unsigned long)asw, 0, 0, 0, 0);
    }
    return True;
}

void send_window_list_func_handler( FunctionData *data, ASEvent *event, int module )
{
    if (module >= 0)
    {
        SendPacket (module, M_TOGGLE_PAGING, 1, DoHandlePageing);
        SendPacket (module, M_NEW_DESK, 1, Scr.CurrentDesk);
        SendPacket (module, M_NEW_PAGE, 3, Scr.Vx, Scr.Vy, Scr.CurrentDesk);
        iterate_asbidirlist( Scr.Windows->clients, send_aswindow_data_iter_func, (void*)module, NULL, False );
        SendPacket (module, M_END_WINDOWLIST, 0);
    }
}

void test_func_handler( FunctionData *data, ASEvent *event, int module )
{
         /* add test command processing here : */
/*         fprintf( stderr, "Testing <do_menu_new( \"Looks\", NULL ) ...\n" ); */
/*         do_menu_new( "Look", NULL, NULL ); */
		 fprintf( stderr, "Testing completed\n" );
}

void
QuickRestart (char *what)
{
    Bool          what_flags = 0;
    Bool          update_background = False;

	if (what == NULL)
		return;

    if (strcasecmp (what, "all") == 0)
        what_flags = PARSE_EVERYTHING;
    else if (strcasecmp (what, "look&feel") == 0)
        what_flags = PARSE_LOOK_CONFIG|PARSE_FEEL_CONFIG;
    else if (strcasecmp (what, "startmenu") == 0 || strcasecmp (what, "feel") == 0)
        what_flags = PARSE_FEEL_CONFIG;
    else if (strcasecmp (what, "look") == 0)
        what_flags = PARSE_LOOK_CONFIG;
    else if (strcasecmp (what, "base") == 0)
        what_flags = PARSE_BASE_CONFIG;
    else if (strcasecmp (what, "database") == 0)
        what_flags = PARSE_DATABASE_CONFIG;
    else if (strcasecmp (what, "background") == 0)
		update_background = True;

	/* Force reinstall */
    if (what)
	{
        InstallRootColormap();
        GrabEm (&Scr, Scr.Feel.cursors[WAIT]);
        LoadASConfig (Scr.CurrentDesk, what_flags);
        UngrabEm ();
	}

	if (update_background)
        SendPacket( -1, M_NEW_BACKGROUND, 1, 1);
}


