/****************************************************************************
 * Copyright (c) 2000,2001,2003 Sasha Vasko <sasha at aftercode.net>
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

#define LOCAL_DEBUG

#include "../../configure.h"

#include "asinternals.h"

#include <limits.h>
#include <signal.h>
#include <unistd.h>

#include "../../libAfterStep/session.h"
#include "../../libAfterStep/moveresize.h"
#include "../../libAfterStep/mylook.h"
#include "../../libAfterStep/desktop_category.h"
#include "../../libAfterStep/wmprops.h"
#include "../../libAfterConf/afterconf.h"
#include "../../libAfterStep/kde.h"

static as_function_handler function_handlers[F_FUNCTIONS_NUM];

void ExecuteComplexFunction (ASEvent * event, char *name);
Bool desktop_category2complex_function (const char *name,
																				const char *category_name);

/* list of available handlers : */
void beep_func_handler (FunctionData * data, ASEvent * event, int module);
void moveresize_func_handler (FunctionData * data, ASEvent * event,
															int module);
void scroll_func_handler (FunctionData * data, ASEvent * event,
													int module);
void movecursor_func_handler (FunctionData * data, ASEvent * event,
															int module);
void raiselower_func_handler (FunctionData * data, ASEvent * event,
															int module);
void setlayer_func_handler (FunctionData * data, ASEvent * event,
														int module);
void change_desk_func_handler (FunctionData * data, ASEvent * event,
															 int module);
void toggle_status_func_handler (FunctionData * data, ASEvent * event,
																 int module);
void iconify_func_handler (FunctionData * data, ASEvent * event,
													 int module);
void focus_func_handler (FunctionData * data, ASEvent * event, int module);
void paste_selection_func_handler (FunctionData * data, ASEvent * event,
																	 int module);
void warp_func_handler (FunctionData * data, ASEvent * event, int module);
void goto_bookmark_func_handler (FunctionData * data, ASEvent * event,
																 int module);
void bookmark_window_func_handler (FunctionData * data, ASEvent * event,
																	 int module);
void pin_menu_func_handler (FunctionData * data, ASEvent * event,
														int module);
void close_func_handler (FunctionData * data, ASEvent * event, int module);
void restart_func_handler (FunctionData * data, ASEvent * event,
													 int module);
void exec_func_handler (FunctionData * data, ASEvent * event, int module);
void exec_in_dir_func_handler (FunctionData * data, ASEvent * event,
															 int module);
void desktop_entry_func_handler (FunctionData * data, ASEvent * event,
																 int module);
void exec_in_term_func_handler (FunctionData * data, ASEvent * event,
																int module);
void exec_tool_func_handler (FunctionData * data, ASEvent * event,
														 int module);
void change_background_func_handler (FunctionData * data, ASEvent * event,
																		 int module);
void change_back_foreign_func_handler (FunctionData * data,
																			 ASEvent * event, int module);
void change_config_func_handler (FunctionData * data, ASEvent * event,
																 int module);
void change_theme_func_handler (FunctionData * data, ASEvent * event,
																int module);
void install_file_func_handler (FunctionData * data, ASEvent * event,
																int module);
void refresh_func_handler (FunctionData * data, ASEvent * event,
													 int module);
void goto_page_func_handler (FunctionData * data, ASEvent * event,
														 int module);
void toggle_page_func_handler (FunctionData * data, ASEvent * event,
															 int module);
void gethelp_func_handler (FunctionData * data, ASEvent * event,
													 int module);
void wait_func_handler (FunctionData * data, ASEvent * event, int module);
void raise_it_func_handler (FunctionData * data, ASEvent * event,
														int module);
void desk_func_handler (FunctionData * data, ASEvent * event, int module);
void deskviewport_func_handler (FunctionData * data, ASEvent * event,
																int module);
void module_func_handler (FunctionData * data, ASEvent * event,
													int module);
void killmodule_func_handler (FunctionData * data, ASEvent * event,
															int module);
void killallmodules_func_handler (FunctionData * data, ASEvent * event,
																	int module);
void restartmodule_func_handler (FunctionData * data, ASEvent * event,
																 int module);
void popup_func_handler (FunctionData * data, ASEvent * event, int module);
void quit_func_handler (FunctionData * data, ASEvent * event, int module);
void windowlist_func_handler (FunctionData * data, ASEvent * event,
															int module);
void quickrestart_func_handler (FunctionData * data, ASEvent * event,
																int module);
void send_window_list_func_handler (FunctionData * data, ASEvent * event,
																		int module);
void save_workspace_func_handler (FunctionData * data, ASEvent * event,
																	int module);
void signal_reload_GTKRC_file_handler (FunctionData * data,
																			 ASEvent * event, int module);
void KIPC_send_message_all_handler (FunctionData * data, ASEvent * event,
																		int module);
void set_func_handler (FunctionData * data, ASEvent * event, int module);
void test_func_handler (FunctionData * data, ASEvent * event, int module);
void screenshot_func_handler (FunctionData * data, ASEvent * event,
															int module);
void swallow_window_func_handler (FunctionData * data, ASEvent * event,
																	int module);
void modulelist_func_handler (FunctionData * data, ASEvent * event,
															int module);


/* handlers initialization function : */
void SetupFunctionHandlers ()
{
	memset (&(function_handlers[0]), 0x00, sizeof (function_handlers));
	function_handlers[F_BEEP] = beep_func_handler;

	function_handlers[F_RESIZE] =
			function_handlers[F_MOVE] = moveresize_func_handler;

#ifndef NO_VIRTUAL
	function_handlers[F_SCROLL] = scroll_func_handler;
	function_handlers[F_MOVECURSOR] = movecursor_func_handler;
#endif

	function_handlers[F_RAISE] =
			function_handlers[F_LOWER] =
			function_handlers[F_RAISELOWER] = raiselower_func_handler;

	function_handlers[F_PUTONTOP] =
			function_handlers[F_PUTONBACK] =
			function_handlers[F_TOGGLELAYER] =
			function_handlers[F_SETLAYER] = setlayer_func_handler;

	function_handlers[F_CHANGE_WINDOWS_DESK] = change_desk_func_handler;

	function_handlers[F_MAXIMIZE] =
			function_handlers[F_FULLSCREEN] =
			function_handlers[F_SHADE] =
			function_handlers[F_STICK] = toggle_status_func_handler;

	function_handlers[F_ICONIFY] = iconify_func_handler;

	function_handlers[F_FOCUS] = focus_func_handler;
	function_handlers[F_CHANGEWINDOW_UP] =
			function_handlers[F_WARP_F] =
			function_handlers[F_CHANGEWINDOW_DOWN] =
			function_handlers[F_WARP_B] = warp_func_handler;

	function_handlers[F_PASTE_SELECTION] = paste_selection_func_handler;
	function_handlers[F_GOTO_BOOKMARK] = goto_bookmark_func_handler;
	function_handlers[F_BOOKMARK_WINDOW] = bookmark_window_func_handler;
	function_handlers[F_PIN_MENU] = pin_menu_func_handler;
	function_handlers[F_DESTROY] =
			function_handlers[F_DELETE] =
			function_handlers[F_CLOSE] = close_func_handler;
	function_handlers[F_RESTART] = restart_func_handler;

	function_handlers[F_DesktopEntry] = desktop_entry_func_handler;

	function_handlers[F_EXEC] =
			function_handlers[F_Swallow] =
			function_handlers[F_MaxSwallow] = exec_func_handler;

	function_handlers[F_ExecInDir] = exec_in_dir_func_handler;

	function_handlers[F_ExecInTerm] = exec_in_term_func_handler;

	function_handlers[F_ExecBrowser] =
			function_handlers[F_ExecEditor] = exec_tool_func_handler;

	function_handlers[F_CHANGE_BACKGROUND] = change_background_func_handler;
	function_handlers[F_CHANGE_BACKGROUND_FOREIGN] =
			change_back_foreign_func_handler;

	function_handlers[F_CHANGE_LOOK] =
			function_handlers[F_CHANGE_FEEL] =
			function_handlers[F_CHANGE_COLORSCHEME] =
			function_handlers[F_CHANGE_THEME_FILE] = change_config_func_handler;

	function_handlers[F_CHANGE_THEME] = change_theme_func_handler;

	function_handlers[F_INSTALL_LOOK] =
			function_handlers[F_INSTALL_FEEL] =
			function_handlers[F_INSTALL_BACKGROUND] =
			function_handlers[F_INSTALL_FONT] =
			function_handlers[F_INSTALL_ICON] =
			function_handlers[F_INSTALL_TILE] =
			function_handlers[F_INSTALL_THEME_FILE] =
			function_handlers[F_INSTALL_COLORSCHEME] = install_file_func_handler;



	function_handlers[F_SAVE_WORKSPACE] = save_workspace_func_handler;
	function_handlers[F_SIGNAL_RELOAD_GTK_RCFILE] =
			signal_reload_GTKRC_file_handler;
	function_handlers[F_KIPC_SEND_MESSAGE_ALL] =
			KIPC_send_message_all_handler;

	function_handlers[F_REFRESH] = refresh_func_handler;
#ifndef NO_VIRTUAL
	function_handlers[F_GOTO_PAGE] = goto_page_func_handler;
	function_handlers[F_TOGGLE_PAGE] = toggle_page_func_handler;
#endif
	function_handlers[F_GETHELP] = gethelp_func_handler;
	function_handlers[F_WAIT] = wait_func_handler;
	function_handlers[F_RAISE_IT] = raise_it_func_handler;
	function_handlers[F_DESK] = desk_func_handler;
	function_handlers[F_GOTO_DESKVIEWPORT] = deskviewport_func_handler;

	function_handlers[F_MODULE] =
			function_handlers[F_SwallowModule] =
			function_handlers[F_MaxSwallowModule] = module_func_handler;

	function_handlers[F_KILLMODULEBYNAME] = killmodule_func_handler;
	function_handlers[F_RESTARTMODULEBYNAME] = restartmodule_func_handler;
	function_handlers[F_KILLALLMODULESBYNAME] = killallmodules_func_handler;
	function_handlers[F_POPUP] = popup_func_handler;
	function_handlers[F_QUIT] = quit_func_handler;
#ifndef NO_WINDOWLIST
	function_handlers[F_WINDOWLIST] = windowlist_func_handler;
#endif													/* ! NO_WINDOWLIST */
	function_handlers[F_STOPMODULELIST] =
			function_handlers[F_RESTARTMODULELIST] = modulelist_func_handler;

	function_handlers[F_QUICKRESTART] = quickrestart_func_handler;
	function_handlers[F_SEND_WINDOW_LIST] = send_window_list_func_handler;
	function_handlers[F_SET] = set_func_handler;
	function_handlers[F_Test] = test_func_handler;

	function_handlers[F_TAKE_WINDOWSHOT] =
			function_handlers[F_TAKE_FRAMESHOT] =
			function_handlers[F_TAKE_SCREENSHOT] = screenshot_func_handler;
	function_handlers[F_SWALLOW_WINDOW] = swallow_window_func_handler;
}

/* complex functions are stored in hash table ComplexFunctions */
ComplexFunction *get_complex_function (char *name)
{
	return find_complex_func (Scr.Feel.ComplexFunctions, name);
}

/* WE need to implement functions queue, so that ExecuteFunction
 * only places function to run into that queue, and queue gets processed
 * at a later time from the main event loop.
 * This is to prevent nasty recursions when functions are called from
 * functions, etc.
 */
typedef struct ASScheduledFunction {
	FunctionData fdata;
	int module;
	ASEvent event;
	/* we do not want to keep pointers to data structures here,
	 * since they may change by the time function is run : */
	Window client;
	Window canvas;

	Bool defered;
} ASScheduledFunction;

ASBiDirList *FunctionQueue = NULL;

void destroy_scheduled_function_handler (void *data)
{
	ASScheduledFunction *sf = data;
	if (sf) {
		free_func_data (&(sf->fdata));
		free (sf);
	}
}

#define destroy_scheduled_function(sf)  destroy_scheduled_function_handler((void*)sf)

static void DoExecuteFunction (ASScheduledFunction * sf);

/***********************************************************************
 *  Procedure:
 *  ExecuteFunction - schedule execution a afterstep built in function
 *  Inputs:
 *      data    - the function to execute
 *      event   - the event that caused the function
 *      module  - number of the module we received function request from
 ***********************************************************************/
void
ExecuteFunctionExt (FunctionData * data, ASEvent * event, int module,
										Bool defered)
{
	ASScheduledFunction *sf = NULL;

	if (data == NULL)
		return;
	if (FunctionQueue == NULL)
		FunctionQueue =
				create_asbidirlist (destroy_scheduled_function_handler);

	sf = safecalloc (1, sizeof (ASScheduledFunction));
	dup_func_data (&(sf->fdata), data);
	sf->module = module;

	sf->event.event_time = Scr.last_Timestamp;
	sf->event.scr = ASDefaultScr;
	sf->defered = defered;
	if (event) {
		sf->event.mask = event->mask;
		sf->event.eclass = event->eclass;
		sf->event.last_time = event->last_time;
		sf->event.typed_last_time = event->typed_last_time;
		sf->event.event_time = event->event_time;

		sf->event.w = event->w;
		sf->event.context = event->context;
		sf->event.x = event->x;
		/* may become freed when it comes to running the function - can't keep a pointer ! */
		if (event->client && event->client->magic == MAGIC_ASWINDOW)
			sf->client = event->client->w;
		if (event->widget)
			sf->canvas = event->widget->w;
	}
	/* we may end up deadlocked if we wait for modules who wait for window list : */
	if (data->func == F_SEND_WINDOW_LIST)
		DoExecuteFunction (sf);
	else
		append_bidirelem (FunctionQueue, sf);
}

void ExecuteFunctionForClient (FunctionData * data, Window client)
{
	ASEvent dummy;
	LOCAL_DEBUG_CALLER_OUT ("client_window(%lX)", client);
	if (data == NULL)
		return;
	memset (&dummy, 0x00, sizeof (dummy));
	dummy.client = window2ASWindow (client);
	dummy.x.type = ButtonRelease;
	dummy.x.xany.window = client;
	ExecuteFunctionExt (data, &dummy, -1, (client != None) /* deffered */ );
}


#undef ExecuteFunction
void ExecuteFunction (FunctionData * data, ASEvent * event, int module)
#ifdef TRACE_ExecuteFunction
#define ExecuteFunction(d,e,m) trace_ExecuteFunction(d,e,m,__FILE__,__LINE__)
#endif
{
	LOCAL_DEBUG_CALLER_OUT
			("event(%d(%s))->window(%lX)->client(%p(%s))->module(%d)",
			 event ? event->x.type : -1,
			 event ? event_type2name (event->x.type) : "n/a",
			 event ? (unsigned long)event->w : 0, event ? event->client : NULL,
			 event ? (event->
								client ? ASWIN_NAME (event->client) : "none") : "none",
			 module);
	if (data == NULL)
		return;
	ExecuteFunctionExt (data, event, module, False);
}

/***********************************************************************
 *  Procedure:
 *  DoExecuteFunction - execute an afterstep built in function
 ***********************************************************************/
static Bool is_interactive_action (FunctionData * data)
{
	if (data->func == F_MOVE || data->func == F_RESIZE)
		return (data->func_val[0] == INVALID_POSITION
						&& data->func_val[1] == INVALID_POSITION);
	return False;
}

static void DoExecuteFunction (ASScheduledFunction * sf)
{
	FunctionData *data = &(sf->fdata);
	ASEvent *event = &(sf->event);
	register FunctionCode func = data->func;

#if !defined(LOCAL_DEBUG) || defined(NO_DEBUG_OUTPUT)
	if (get_output_threshold () >= OUTPUT_LEVEL_DEBUG)
#endif
	{
		print_func_data (__FILE__, "DoExecuteFunction", __LINE__, data);
	}

	if (sf->client != None) {
		ASWindow *asw = window2ASWindow (sf->client);
		if (asw == NULL) {					/* window had died by now - let go on with our lives */
			destroy_scheduled_function (sf);
			return;
		}

		event->client = asw;

		if (sf->canvas) {
			ASCanvas *canvas = NULL;
			if (sf->canvas == asw->client_canvas->w)
				canvas = asw->client_canvas;
			else if (sf->canvas == asw->frame_canvas->w)
				canvas = asw->frame_canvas;
			else if (asw->icon_canvas && sf->canvas == asw->icon_canvas->w)
				canvas = asw->icon_canvas;
			else if (asw->icon_title_canvas
							 && sf->canvas == asw->icon_title_canvas->w)
				canvas = asw->icon_title_canvas;
			else {
				int i = FRAME_SIDES;
				while (--i >= 0)
					if (asw->frame_sides[i] != NULL &&
							asw->frame_sides[i]->w == sf->canvas) {
						canvas = asw->frame_sides[i];
						break;
					}
			}
			event->widget = canvas;
		}
	}

	/* Defer Execution may wish to alter this value */
	LOCAL_DEBUG_OUT ("client = %p, IsWindowFunc() = %s", event->client,
									 IsWindowFunc (func) ? "Yes" : "No");
	if (IsWindowFunc (func)) {
		int do_defer = !(sf->defered), fin_event;

		if (event->x.type == ButtonPress)
			fin_event = ButtonRelease;
		else if (event->x.type == MotionNotify)
			fin_event =
					(event->x.xmotion.state & AllButtonMask) !=
					0 ? ButtonRelease : ButtonPress;
		else
			fin_event = ButtonPress;

		if (data->text != NULL && event->client == NULL && func != F_SWALLOW_WINDOW)	/* SWallowWindow has module name as its text ! */
			if (*(data->text) != '\0')
				if ((event->client = pattern2ASWindow (data->text)) != NULL) {
					event->w = get_window_frame (event->client);
					do_defer = False;
				}

		if (event->x.type == KeyPress || event->x.type == KeyRelease) {	/* keyboard events should never be deferred,
																																		 * and if no client is selected for window specific function - then it should be ignored */
			if (event->client == NULL)
				func = F_NOP;
			do_defer = False;
		}

		if (do_defer
				&& (fin_event != ButtonRelease || !is_interactive_action (data))) {
			int cursor = ASCUR_Move;
			if (func != F_RESIZE && func != F_MOVE)
				cursor = (func != F_DESTROY && func != F_DELETE
									&& func != F_CLOSE) ? ASCUR_Select : ASCUR_Destroy;

			if (DeferExecution (event, cursor, fin_event))
				func = F_NOP;
		}

		if (event->client == NULL)
			func = F_NOP;

	}

	if (function_handlers[func] || func == F_FUNCTION || func == F_CATEGORY) {
		char *complex_func_name = COMPLEX_FUNCTION_NAME (data);

		data->func = func;
		if (event->client) {
			if (get_flags (event->client->status->flags, AS_Fullscreen) &&
					(data->func == F_MOVE ||
					 data->func == F_RESIZE || data->func == F_MAXIMIZE)) {
				LOCAL_DEBUG_OUT
						("function \"%s\" is not allowed for the Fullscreen window",
						 COMPLEX_FUNCTION_NAME (data));
				func = data->func = F_BEEP;
			} else
					if (!check_allowed_function2 (data->func, event->client->hints))
			{
				LOCAL_DEBUG_OUT
						("function \"%s\" is not allowed for the specifyed window (mask 0x%lX)",
						 COMPLEX_FUNCTION_NAME (data),
						 ASWIN_FUNC_MASK (event->client));
				func = data->func = F_BEEP;
			}
		}

		if (get_flags (AfterStepState, ASS_WarpingMode) &&
				function_handlers[func] != warp_func_handler)
			EndWarping ();

		if (func == F_CATEGORY) {
			char *cat_name = data->text ? data->text : data->name;
			if (cat_name == NULL)
				func = F_BEEP;
			else {
				complex_func_name =
						safemalloc (sizeof ("CATEGORY()") + strlen (cat_name) + 1);
				sprintf (complex_func_name, "CATEGORY(%s)", cat_name);
				if (get_complex_function (complex_func_name) != NULL) {
					func = F_FUNCTION;
				} else {
					if (desktop_category2complex_function
							(complex_func_name, cat_name))
						func = F_FUNCTION;
					else
						func = F_BEEP;
				}
			}
		}

		if (func == F_FUNCTION)
			ExecuteComplexFunction (event, complex_func_name);
		else
			function_handlers[func] (data, event, sf->module);
		if (complex_func_name && complex_func_name != data->name
				&& complex_func_name != data->text)
			free (complex_func_name);
	}
	destroy_scheduled_function (sf);
}

void ExecutePendingFunctions ()
{
	ASScheduledFunction *sf;
	if (FunctionQueue)
		while ((sf = extract_first_bidirelem (FunctionQueue)) != NULL)
			DoExecuteFunction (sf);
}

void DestroyPendingFunctionsQueue ()
{
	if (FunctionQueue)
		destroy_asbidirlist (&FunctionQueue);
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
int DeferExecution (ASEvent * event, int cursor, int finish_event)
{
	Bool res = False;
	LOCAL_DEBUG_CALLER_OUT
			("cursor %d, event %d, window 0x%lX, window_name \"%s\", finish event %d",
			 cursor, event ? event->x.type : -1,
			 event ? (unsigned long)event->w : 0,
			 event->client ? ASWIN_NAME (event->client) : "none", finish_event);

/*    if (event->context != C_ROOT && event->context != C_NO_CONTEXT)
	if ( finish_event == ButtonPress ||
		(finish_event == ButtonRelease && event->x.type != ButtonPress))
		return False;
*/
	if (!(res = !GrabEm (ASDefaultScr, Scr.Feel.cursors[cursor]))) {
		ASHintWindow *hint =
				create_ashint_window (ASDefaultScr, &(Scr.Look),
															"Please select target window");
		WaitEventLoop (event, finish_event, -1, hint);
		destroy_ashint_window (&hint);

		LOCAL_DEBUG_OUT ("window(%lX)->root(%lX)->subwindow(%lX)",
										 event->x.xbutton.window, event->x.xbutton.root,
										 event->x.xbutton.subwindow);
		if (event->client == NULL) {
			res = True;
			/* since we grabbed cursor we may get clicks over client windows as reported
			 * relative to the root window, in which case we have to check subwindow to
			 * see what client was clicked */
			if (event->x.xbutton.subwindow != event->w) {
				event->client = window2ASWindow (event->x.xbutton.subwindow);
				if (event->client != NULL) {
					res = False;
					event->w = event->x.xbutton.subwindow;
				}
			}
		}
		UngrabEm ();
	}
	if (res)
		XBell (dpy, event->scr->screen);

	LOCAL_DEBUG_OUT ("result %d, event %d, window 0x%lX, window_name \"%s\"",
									 res, event ? event->x.type : -1,
									 event ? (unsigned long)event->w : 0,
									 event->client ? ASWIN_NAME (event->client) : "none");

	return res;
}

/*****************************************************************************
 *  Executes complex function defined with Function/EndFunction construct :
 ****************************************************************************/
void ExecuteComplexFunction (ASEvent * event, char *name)
{
	ComplexFunction *func = NULL;
	int clicks = 0;
	register int i;
	Bool persist = False;
	Bool need_window = False;
	char c;
	static char clicks_upper[MAX_CLICKS_HANDLED + 1] =
			{ CLICKS_TRIGGERS_UPPER };
	static char clicks_lower[MAX_CLICKS_HANDLED + 1] =
			{ CLICKS_TRIGGERS_LOWER };
	LOCAL_DEBUG_CALLER_OUT
			("event %d, window 0x%lX, asw(%p), window_name \"%s\", function name \"%s\"",
			 event ? event->x.type : -1, event ? (unsigned long)event->w : 0,
			 event->client, event->client ? ASWIN_NAME (event->client) : "none",
			 name);


	if (name && (name[0] == IMMEDIATE || name[0] == IMMEDIATE_UPPER))
		if (name[1] == ':')
			if ((func = get_complex_function (&(name[2]))) == NULL)
				return;
	if (func == NULL)
		if ((func = get_complex_function (name)) == NULL)
			return;

	LOCAL_DEBUG_OUT
			("function data found  %p ...\n   proceeding to execution of immidiate items...",
			 func);
	if (event->w == None && event->client != NULL)
		event->w = get_window_frame (event->client);
	/* first running all the Imediate actions : */
	for (i = 0; i < func->items_num; i++) {
		c = func->items[i].name ? *(func->items[i].name) : 'i';
		if (c == IMMEDIATE || c == IMMEDIATE_UPPER) {
			Bool skip = False;
			if (func->items[i].name &&
					IsExecFunc (func->items[i].func) &&
					func->items[i].name[1] == ':') {
				int k = i;
				while (++k < func->items_num)
					if (func->items[k].func == F_WAIT && func->items[k].name)
						if (strcmp (func->items[k].name, func->items[i].name) == 0) {
							skip = True;
							break;
						}
				if (skip) {
					/* since function names in complex functions have i or I as the first
					 * character to signify that they are immidiate items - we allow semicolon
					 * to separate that special character from the actuall window's name for
					 * readability sake
					 */
					char *pattern_start = &(func->items[i].name[2]);
					if (complex_pattern2ASWindow (pattern_start) == NULL)
						skip = False;				/* window is not up yet - can't skip */
				}
			}
			if (!skip)
				ExecuteFunctionExt (&(func->items[i]), event, -1, True);
		} else {
			persist = True;
			if (IsWindowFunc (func->items[i].func))
				need_window = True;
		}
	}
	/* now lets count clicks : */
	LOCAL_DEBUG_OUT ("done with immidiate items - persisting ?: %s",
									 persist ? "True" : "False");
	if (!persist)
		return;

	if (need_window && event->client == NULL) {
		if (DeferExecution (event, ASCUR_Select, ButtonPress)) {
/*            WaitForButtonsUpLoop (); */
			return;
		}
	}
	if (!GrabEm (ASDefaultScr, Scr.Feel.cursors[ASCUR_Select])) {
		show_warning
				("failed to grab pointer while executing complex function \"%s\"",
				 name);
		XBell (dpy, Scr.screen);
		return;
	}

	clicks = 0;
	while (IsClickLoop (event, ButtonReleaseMask, Scr.Feel.ClickTime)) {
		clicks++;
		if (!IsClickLoop (event, ButtonPressMask, Scr.Feel.ClickTime))
			break;
		clicks++;
	}
	if (clicks <= MAX_CLICKS_HANDLED) {
		/* some functions operate on button release instead of
		 * presses. These gets really weird for complex functions ... */
/*        if (event->x.type == ButtonPress)
		event->x.type = ButtonRelease;
 */
		/* first running all the Imediate actions : */
		for (i = 0; i < func->items_num; i++)
			if (func->items[i].name) {
				c = func->items[i].name[0];
				if (c == clicks_upper[clicks] || c == clicks_lower[clicks])
					ExecuteFunctionExt (&(func->items[i]), event, -1, True);
			}
	}
/*    WaitForButtonsUpLoop (); */
	UngrabEm ();
	LOCAL_DEBUG_OUT
			("at the end : event %d, window 0x%lX, asw(%p), window_name \"%s\", function name \"%s\"",
			 event ? event->x.type : -1, event ? (unsigned long)event->w : 0,
			 event->client, event->client ? ASWIN_NAME (event->client) : "none",
			 name);
}

void ExecuteBatch (ComplexFunction * batch)
{
	ASEvent event = { 0 };
	register int i;

	LOCAL_DEBUG_CALLER_OUT ("event %p", batch);

	if (batch) {
		for (i = 0; i < batch->items_num; i++) {
			int func = batch->items[i].func;
			if (IsWindowFunc (func) || function_handlers[func] == NULL)
				continue;

			function_handlers[func] (&(batch->items[i]), &event, -1);
		}
	}
}


Bool desktop_category2complex_function (const char *name,
																				const char *category_name)
{
	ASCategoryTree *ct = NULL;
	ASDesktopCategory *dc = NULL;
	ComplexFunction *func;
	char **entries;
	int i, entries_num;

	if ((dc = name2desktop_category (category_name, &ct)) == NULL)
		return False;

	entries_num = PVECTOR_USED (dc->entries);
	if (entries_num == 0)
		return False;
	entries = PVECTOR_HEAD (char *, dc->entries);

	if ((func =
			 new_complex_func (Scr.Feel.ComplexFunctions, (char *)name)) == NULL)
		return False;

	func->items_num = 0;
	func->items = safecalloc (entries_num, sizeof (FunctionData));
	for (i = 0; i < entries_num; ++i) {
		ASDesktopEntry *de;
		FunctionData *fdata = &(func->items[func->items_num]);
		if ((de = fetch_desktop_entry (ct, entries[i])) == NULL)
			continue;
		if (de->type != ASDE_TypeApplication)
			continue;

		++(func->items_num);
		init_func_data (fdata);
		fdata->name = mystrdup ("I");	/* for immidiate execution */
		fdata->func = desktop_entry2function_code (de);
		if (de->clean_exec)
			fdata->text = mystrdup (de->clean_exec);
	}

	return True;
}

/***************************************************************************************
 *
 * All The Handlers Go Below, Go Below,
 *                              Go Below.
 * All The Handlers Go Below, Go Below, Go Below
 * Ta-da-da-da-da-da-da
 *
 ****************************************************************************************/

void beep_func_handler (FunctionData * data, ASEvent * event, int module)
{
	XBell (dpy, event->scr->screen);
}

void moveresize_func_handler (FunctionData * data, ASEvent * event,
															int module)
{																/* gotta have a window */
	ASWindow *asw = event->client;
	if (asw == NULL)
		return;

	if (!is_interactive_action (data)) {
		int new_val1 = 0, new_val2 = 0;
		int x = asw->status->x;
		int y = asw->status->y;
		int width = asw->status->width;
		int height = asw->status->height;

		new_val1 =
				APPLY_VALUE_UNIT (Scr.MyDisplayWidth, data->func_val[0],
													data->unit_val[0]);
		LOCAL_DEBUG_OUT ("val1 = %d, unit1 = %d, new_val1 = %d",
										 (int)data->func_val[0], (int)data->unit_val[0],
										 new_val1);
		new_val2 =
				APPLY_VALUE_UNIT (Scr.MyDisplayHeight, data->func_val[1],
													data->unit_val[1]);
		if (data->func == F_MOVE) {
			if (data->func_val[0] != INVALID_POSITION) {
				x = new_val1;
				if (!ASWIN_GET_FLAGS (asw, AS_Sticky))
					x -= asw->status->viewport_x;
			}
			if (data->func_val[1] != INVALID_POSITION) {
				y = new_val2;
				if (!ASWIN_GET_FLAGS (asw, AS_Sticky))
					y -= asw->status->viewport_y;
			}
		} else {
			if (data->func_val[0] != INVALID_POSITION)
				width = new_val1;
			if (data->func_val[1] != INVALID_POSITION)
				height = new_val2;
		}
		moveresize_aswindow_wm (asw, x, y, width, height, False);

	} else {
		ASMoveResizeData *mvrdata;
		/*release_pressure(); */
		if (data->func == F_MOVE) {
			mvrdata = move_widget_interactively (Scr.RootCanvas,
																					 asw->frame_canvas,
																					 event,
																					 apply_aswindow_move,
																					 complete_aswindow_move);
		} else {
			int side = 0;
			register unsigned long context = (event->context & C_FRAME);

			if (ASWIN_GET_FLAGS (asw, AS_Shaded)) {
				XBell (dpy, Scr.screen);
				return;
			}

			while ((0x01 & context) == 0 && side <= FR_SE) {
				++side;
				context = context >> 1;
			}

			if (side > FR_SE) {
				int pointer_x = 0, pointer_y = 0;
				ASQueryPointerRootXY (&pointer_x, &pointer_y);
				if (pointer_x >
						asw->frame_canvas->root_x + asw->frame_canvas->width / 2) {
					if (pointer_y >
							asw->frame_canvas->root_y + asw->frame_canvas->height / 2)
						side = FR_SE;
					else
						side = FR_NE;
				} else if (pointer_y >
									 asw->frame_canvas->root_y +
									 asw->frame_canvas->height / 2)
					side = FR_SW;
				else
					side = FR_NW;
			}

			mvrdata = resize_widget_interactively (Scr.RootCanvas,
																						 asw->frame_canvas,
																						 event,
																						 apply_aswindow_moveresize,
																						 complete_aswindow_moveresize,
																						 side);
		}
		if (mvrdata) {
			mvrdata->move_only = (data->func == F_MOVE);
			setup_aswindow_moveresize (asw, mvrdata);
			ASWIN_SET_FLAGS (asw, AS_MoveresizeInProgress);
		}
	}
}


static inline int
make_scroll_pos (int val, int unit, int curr, int max, int size)
{
	int pos = curr;
	if (val > -100000 && val < 100000) {
		pos += APPLY_VALUE_UNIT (size, val, unit);
		if (pos < 0)
			pos = 0;
		if (pos > max)
			pos = max;
	} else {
		pos += APPLY_VALUE_UNIT (size, val / 1000, unit);
		while (pos < 0)
			pos += max;
		while (pos > max)
			pos -= max;
	}
	return pos;
}

static inline int
make_edge_scroll (int curr_pos, int curr_view, int view_size, int max_view,
									int edge_scroll)
{
	int new_view = curr_view;
	edge_scroll = (edge_scroll * view_size) / 100;
	if (curr_pos >= new_view + view_size - 2) {
		while (curr_pos >= new_view + view_size - 2)
			new_view += edge_scroll;
		if (new_view > max_view)
			new_view = max_view;
	} else {
		while (curr_pos < new_view + 2)
			new_view -= edge_scroll;
		if (new_view < 0)
			new_view = 0;
	}
	return new_view;
}

void scroll_func_handler (FunctionData * data, ASEvent * event, int module)
{
#ifndef NO_VIRTUAL
	int x, y;
	register ScreenInfo *scr = event->scr;

	x = make_scroll_pos (data->func_val[0], data->unit_val[0], scr->Vx,
											 scr->VxMax, scr->MyDisplayWidth);
	y = make_scroll_pos (data->func_val[1], data->unit_val[1], scr->Vy,
											 scr->VyMax, scr->MyDisplayHeight);

	MoveViewport (x, y, True);
#endif
}

void movecursor_func_handler (FunctionData * data, ASEvent * event,
															int module)
{
	int curr_x, curr_y;
	int x, y;
	register ScreenInfo *scr = ASDefaultScr;

	ASQueryPointerRootXY (&curr_x, &curr_y);
	x = make_scroll_pos (data->func_val[0], data->unit_val[0],
											 scr->Vx + curr_x, scr->VxMax + scr->MyDisplayWidth,
											 scr->MyDisplayWidth);
	y = make_scroll_pos (data->func_val[1], data->unit_val[1],
											 scr->Vy + curr_y, scr->VyMax + scr->MyDisplayHeight,
											 scr->MyDisplayHeight);

#ifndef NO_VIRTUAL
	{
		int new_vx = 0, new_vy = 0;

		new_vx =
				make_edge_scroll (x, scr->Vx, scr->MyDisplayWidth, scr->VxMax,
													scr->Feel.EdgeScrollX);
		new_vy =
				make_edge_scroll (y, scr->Vy, scr->MyDisplayHeight, scr->VyMax,
													scr->Feel.EdgeScrollY);
		if (new_vx != scr->Vx || new_vy != scr->Vy)
			MoveViewport (new_vx, new_vy, True);
	}
#endif
	x -= scr->Vx;
	y -= scr->Vy;
	if (x != curr_x || y != curr_y)
		XWarpPointer (dpy, scr->Root, scr->Root, 0, 0, scr->MyDisplayWidth,
									scr->MyDisplayHeight, x, y);
}

void iconify_func_handler (FunctionData * data, ASEvent * event,
													 int module)
{
	if (event->client) {

		LOCAL_DEBUG_CALLER_OUT
				("function %d (val0 = %d), event %d, window 0x%lX, window_name \"%s\", module %d",
				 (int)(data ? data->func : 0), (int)(data ? data->func_val[0] : 0),
				 event ? event->x.type : -1, event ? (unsigned long)event->w : 0,
				 event->client ? ASWIN_NAME (event->client) : "none", module);
		if (ASWIN_GET_FLAGS (event->client, AS_Iconic)) {
			if (data->func_val[0] <= 0)
				set_window_wm_state (event->client, False, False);
		} else if (data->func_val[0] >= 0)
			set_window_wm_state (event->client, True, False);
	}
}

void raiselower_func_handler (FunctionData * data, ASEvent * event,
															int module)
{
	if (event->client) {
		if (event->client->last_restack_time != CurrentTime &&
				event->event_time != CurrentTime &&
				event->client->last_restack_time >= event->event_time)
			return;

		restack_window (event->client, None, (data->func == F_RAISE) ? Above :
										((data->func == F_RAISELOWER) ? Opposite : Below));
	}
}

void raise_it_func_handler (FunctionData * data, ASEvent * event,
														int module)
{
	ASWindow *asw = window2ASWindow (data->func_val[1]);
	activate_aswindow (asw, True, True);
}

void setlayer_func_handler (FunctionData * data, ASEvent * event,
														int module)
{
	if (event->client) {
		register int func = data->func;
		int layer = 0;

		if (func == F_PUTONTOP)
			layer = AS_LayerTop;
		else if (func == F_PUTONBACK)
			layer = AS_LayerBack;
		else if (func == F_TOGGLELAYER) {
			layer = ASWIN_LAYER (event->client);
			if (data->func_val[0] == 0)
				layer += data->func_val[1];
			else
				layer += data->func_val[0];
		} else
			layer = data->func_val[0];
		change_aswindow_layer (event->client, layer);
	}
}

void change_desk_func_handler (FunctionData * data, ASEvent * event,
															 int module)
{
	if (event->client)
		change_aswindow_desktop (event->client, data->func_val[0], False);
}

void toggle_status_func_handler (FunctionData * data, ASEvent * event,
																 int module)
{
	ASFlagType toggle_flags = 0;
	if (data->func == F_STICK)
		toggle_flags = AS_Sticky;
	else if (data->func == F_MAXIMIZE) {
		if (data->func_val[0] > 0 || data->func_val[1] == 0)
			toggle_flags = AS_MaximizedX;
		if (data->func_val[1] > 0 || data->func_val[0] == 0)
			toggle_flags |= AS_MaximizedY;
		if (event->client) {
			event->client->maximize_ratio_x =
					(data->unit_val[0] ==
					 0) ? data->func_val[0] : (data->func_val[0] *
																		 data->unit_val[0] * 100) /
					Scr.MyDisplayWidth;
			event->client->maximize_ratio_y =
					(data->unit_val[1] ==
					 0) ? data->func_val[1] : (data->func_val[1] *
																		 data->unit_val[1] * 100) /
					Scr.MyDisplayHeight;
		}
	} else if (data->func == F_SHADE)
		toggle_flags = AS_Shaded;
	else if (data->func == F_FULLSCREEN)
		toggle_flags = AS_Fullscreen;
	else
		return;
	toggle_aswindow_status (event->client, toggle_flags);
}

void focus_func_handler (FunctionData * data, ASEvent * event, int module)
{
	activate_aswindow (event->client, True, False);
}

void warp_func_handler (FunctionData * data, ASEvent * event, int module)
{
	register ASWindow *t = NULL;

	if (data->text != NULL)
		if (*(data->text) != '\0')
			t = pattern2ASWindow (data->text);
	if (t == NULL)
		t = warp_aswindow_list (Scr.Windows,
														(data->func == F_CHANGEWINDOW_DOWN
														 || data->func == F_WARP_B));
	if (t != NULL) {
		event->client = t;
		event->w = get_window_frame (t);
		StartWarping (ASDefaultScr);
		warp_to_aswindow (t,
											(data->func == F_WARP_F || data->func == F_WARP_B));
	}
}

void paste_selection_func_handler (FunctionData * data, ASEvent * event,
																	 int module)
{
	PasteSelection (event->scr);
}

void goto_bookmark_func_handler (FunctionData * data, ASEvent * event,
																 int module)
{
	ASWindow *asw = bookmark2ASWindow (data->text);
	if (asw)
		activate_aswindow (asw, True, False);
}

void bookmark_window_func_handler (FunctionData * data, ASEvent * event,
																	 int module)
{
	bookmark_aswindow (event->client, data->text);
}

void pin_menu_func_handler (FunctionData * data, ASEvent * event,
														int module)
{
	ASMenu *menu = NULL;
	char *menu_name = data->text ? data->text : data->name;
	LOCAL_DEBUG_OUT ("menu_name = \"%s\", client = %p, internal = %p",
									 menu_name ? "NULL" : menu_name, event->client,
									 event->client->internal);
	if (menu_name && menu_name[0] != '\0')
		menu = find_asmenu (menu_name);
	else if (event->client && event->client->internal) {
		ASMagic *data = event->client->internal->data;
		LOCAL_DEBUG_OUT ("data = %p, magic = %lX", data, data->magic);
		if (data->magic == MAGIC_ASMENU)
			menu = (ASMenu *) data;
	}
	if (menu == NULL)
		XBell (dpy, event->scr->screen);
	else
		pin_asmenu (menu);
}


void close_func_handler (FunctionData * data, ASEvent * event, int module)
{
	if (event->client) {
		Window w = event->client->w;

		LOCAL_DEBUG_OUT ("window(0x%lX)->protocols(0x%lX)", w,
										 event->client->hints->protocols);
		if (get_flags (event->client->hints->protocols, AS_DoesWmDeleteWindow)
				&& data->func != F_DESTROY) {
			LOCAL_DEBUG_OUT ("sending delete window request to 0x%lX", w);
			send_wm_protocol_request (w, _XA_WM_DELETE_WINDOW, CurrentTime);
		} else {
			if (event->client->internal != NULL
					|| validate_drawable (w, NULL, NULL) == None)
				Destroy (event->client, True);
			else if (data->func == F_DELETE)
				XBell (dpy, event->scr->screen);
			else
				XKillClient (dpy, w);
			XSync (dpy, 0);
		}
	}
}

void restart_func_handler (FunctionData * data, ASEvent * event,
													 int module)
{
	Done (True, data->text);
}

void exec_func_handler (FunctionData * data, ASEvent * event, int module)
{
	/* no sense in grabbing Pointer here as fork will return rather fast not creating 
	   much of the delay */
	spawn_child (data->text, -1, -1, NULL, None, C_NO_CONTEXT, True, False,
							 NULL);
}

void exec_in_dir_func_handler (FunctionData * data, ASEvent * event,
															 int module)
{
	char *cmd = tokenskip (data->text, 1);

	if (cmd == NULL || cmd[0] == '\0')
		exec_func_handler (data, event, module);
	else if (fork () == 0) {
		char *dirname = NULL, *fulldirname = NULL;
		parse_token (data->text, &dirname);
		fulldirname = put_file_home (dirname);
		if (chdir (fulldirname) == 0)
			spawn_child (cmd, -1, -1, NULL, None, C_NO_CONTEXT, False, False,
									 NULL);
	}
}

int find_escaped_chr_pos (const char *str, char c)
{
	int i;
	for (i = 0; str[i] != '\0'; ++i) {
		if (str[i] == '\\') {
			if (str[++i] == '\0')
				break;
		} else if (str[i] == c)
			break;
	}
	return i;
}

char *parse_term_cmdl (const char *term_name, const char *term_command,
											 const char *cmdl)
{
	int term_name_len, cmdl_len, curr_full, curr_cmdl;
	char *full_cmdl = NULL;
	Bool first = True;

	if (term_name == NULL || term_command == NULL || cmdl == NULL)
		return NULL;

	LOCAL_DEBUG_OUT
			("term_name = \"%s\", term_command = \"%s\", cmdl = \"%s\"",
			 term_name, term_command, cmdl);
	curr_full = strlen (term_command);
	cmdl_len = strlen (cmdl);
	term_name_len = strlen (term_name);
	curr_cmdl = 0;
	full_cmdl = safemalloc (curr_full + 4 + cmdl_len + 1);

	strcpy (full_cmdl, term_command);

	while (curr_cmdl < cmdl_len) {
		while (isspace (cmdl[curr_cmdl]))
			++curr_cmdl;
		if (mystrncasecmp (&(cmdl[curr_cmdl]), "if(", 3) == 0) {
			int tmp;

			curr_cmdl += 3;
			tmp = curr_cmdl;
			curr_cmdl += find_escaped_chr_pos (&(cmdl[curr_cmdl]), '}') + 1;
			while (isspace (cmdl[tmp]))
				++tmp;
			if (mystrncasecmp (&(cmdl[tmp]), term_name, term_name_len) == 0) {
				tmp += term_name_len;
				while (isspace (cmdl[tmp]))
					++tmp;
				if (cmdl[tmp] == ')') {
					++tmp;
					while (isspace (cmdl[tmp]))
						++tmp;
					if (cmdl[tmp] == '{')
						++tmp;
					while (isspace (cmdl[tmp]))
						++tmp;
					full_cmdl[curr_full++] = ' ';
					while (tmp < curr_cmdl - 1)
						full_cmdl[curr_full++] = cmdl[tmp++];
				}
			}
		} else {
			if (first) {
				if (cmdl[curr_cmdl] != '-') {
					if (cmdl[curr_cmdl] != '\0')
						sprintf (&(full_cmdl[curr_full]), " -e %s",
										 &(cmdl[curr_cmdl]));
					else
						full_cmdl[curr_full] = '\0';
					return full_cmdl;
				}
				first = False;
			}

			if (strncmp (&(cmdl[curr_cmdl]), "-e ", 3) == 0) {
				sprintf (&(full_cmdl[curr_full]), " %s", &(cmdl[curr_cmdl]));
				return full_cmdl;
			}

			full_cmdl[curr_full++] = ' ';
			while (curr_cmdl < cmdl_len && !isspace (cmdl[curr_cmdl]))
				full_cmdl[curr_full++] = cmdl[curr_cmdl++];
		}

	}
	full_cmdl[curr_full] = '\0';

	return full_cmdl;
}

void exec_in_term_func_handler (FunctionData * data, ASEvent * event,
																int module)
{
	if (Environment->tool_command[ASTool_Term] != NULL && data->text != NULL) {
		char *full_cmdl = NULL;
		char *term_name =
				strrchr (Environment->tool_command[ASTool_Term], '/');
		term_name =
				(term_name ==
				 NULL) ? Environment->tool_command[ASTool_Term] : term_name + 1;
		full_cmdl =
				parse_term_cmdl (term_name, Environment->tool_command[ASTool_Term],
												 data->text);
		if (full_cmdl) {
			LOCAL_DEBUG_OUT ("full_cmdl = [%s]", full_cmdl);
			/* no sense in grabbing Pointer here as fork will return rather fast not creating 
			   much of the delay */
			spawn_child (full_cmdl, -1, -1, NULL, None, C_NO_CONTEXT, True,
									 False, NULL);
			free (full_cmdl);
		}
	}
}

int /* return -1 - undetermined, 0 - no term, 1 - terminal requiresd */
check_tool_needs_term (const char *tool)
{
	static char *known_text_mode_apps[] = {
		"vi", "vim", "joe", "jed", "ne", "le", "moe", "elvis", "jove", "dav",
				"aoeui",
		"elinks", "lynx",
		NULL
	};
	ASDesktopEntry *de;

	char *name = NULL;
	struct stat st;
	int res = -1;
	int i;

	parse_file_name (tool, NULL, &name);
	LOCAL_DEBUG_OUT ("checking if \"%s\" requires term ...", name);
	de = fetch_desktop_entry (CombinedCategories, name);
	if (de != NULL) {
		LOCAL_DEBUG_OUT ("DesktopEntry \"%s\" found", de->Name);
		res = get_flags (de->flags, ASDE_Terminal) ? 1 : 0;
	} else if (lstat (tool, &st) != -1) {	/* now we need to resolve all the symlinking */

		if ((st.st_mode & S_IFMT) == S_IFLNK) {
			char linkdst[1024];
			int len = readlink (tool, linkdst, sizeof (linkdst) - 1);
			if (len > 0) {
				linkdst[len] = '\0';
				res = check_tool_needs_term (linkdst);
				LOCAL_DEBUG_OUT ("checking symlink target \"%s\" - res = %d",
												 linkdst, res);
			}
		}
	}

	/* last resort - check hardcoded list */
	for (i = 0; res < 0 && known_text_mode_apps[i]; ++i)
		if (strcmp (name, known_text_mode_apps[i]) == 0)
			res = 1;

	LOCAL_DEBUG_OUT ("\"%s\" requires term  = %d", name, res);

	free (name);
	return res;
}

void exec_tool_func_handler (FunctionData * data, ASEvent * event,
														 int module)
{
	ASToolType tool =
			(data->func == F_ExecBrowser) ? ASTool_Browser : ASTool_Editor;
	if (Environment->tool_command[tool] != NULL && data->text != NULL) {
		char *full_cmdl =
				safemalloc (strlen (Environment->tool_command[tool]) + 1 +
										strlen (data->text) + 1);
		sprintf (full_cmdl, "%s %s", Environment->tool_command[tool],
						 data->text);
		LOCAL_DEBUG_OUT ("full_cmdl = [%s]", full_cmdl);

		if (check_tool_needs_term (Environment->tool_command[tool]) > 0) {
			char *old_text = data->text;
			data->text = full_cmdl;
			exec_in_term_func_handler (data, event, module);
			data->text = old_text;
		} else {
			/* no sense in grabbing Pointer here as fork will return rather fast not creating 
			   much of the delay */
			spawn_child (full_cmdl, -1, -1, NULL, None, C_NO_CONTEXT, True,
									 False, NULL);
		}
		free (full_cmdl);
	}
}

void desktop_entry_func_handler (FunctionData * data, ASEvent * event,
																 int module)
{
	ASDesktopEntry *de =
			name2desktop_entry (data->text ? data->text : data->name, NULL);
	if (de == NULL)
		XBell (dpy, event->scr->screen);
	else {
		FunctionData *real_data = desktop_entry2function (de, NULL);
		if (real_data) {
			ExecuteFunctionExt (real_data, event, -1, True);
			destroy_func_data (&real_data);
		}
	}
}


static int _as_config_change_recursion = 0;
static int _as_config_change_count = 0;
static int _as_background_change_count = 0;

void commit_config_change (int func)
{
	if (_as_config_change_recursion <= 1) {
		if (_as_background_change_count > 0) {
			MyBackground *new_back =
					get_desk_back_or_default (Scr.CurrentDesk, False);
			/* we want to display selected background even if that was disabled in look,
			   this may cause problems, since when user loads AfterStep the next time - he/she 
			   will see what was configured in look again, and not what was selected from 
			   the menu ! */
			destroy_string (&(new_back->data));
			new_back->type = MB_BackImage;
			if (new_back->loaded_im_name) {
				free (new_back->loaded_im_name);
				new_back->loaded_im_name = NULL;
			}
			change_desktop_background (Scr.CurrentDesk);
			SendPacket (-1, M_NEW_BACKGROUND, 1, 1);
			_as_background_change_count = 0;
		}
		if (_as_config_change_count > 0) {
			if (func == F_CHANGE_THEME)
				QuickRestart ("theme");
			else if (func == F_CHANGE_COLORSCHEME)
				QuickRestart ("look");
			else
				QuickRestart ((func == F_CHANGE_LOOK) ? "look" : "feel");
			_as_config_change_count = 0;
		}
	}
}

static void change_background_internal (FunctionData * data,
																				Bool autoscale)
{
	char tmpfile[256], *realfilename;
	Bool success = False;

	++_as_config_change_recursion;
	XGrabPointer (dpy, Scr.Root, True, ButtonPressMask | ButtonReleaseMask,
								GrabModeAsync, GrabModeAsync, Scr.Root,
								Scr.Feel.cursors[ASCUR_Wait], CurrentTime);
	XSync (dpy, 0);

	if (Scr.screen == 0)
		sprintf (tmpfile, BACK_FILE, Scr.CurrentDesk);
	else
		sprintf (tmpfile, BACK_FILE ".scr%ld", Scr.CurrentDesk, Scr.screen);

	realfilename = make_session_data_file (Session, False, 0, tmpfile, NULL);
	cover_desktop ();
	if (autoscale) {
		ASImage *src_im =
				get_asimage (Scr.image_manager, data->text, 0xFFFFFFFF, 0);
		if (src_im) {
			if (src_im->width >= 600 && src_im->height >= 600) {
				int clip_width = src_im->width;
				int clip_height = src_im->height;
				ASImage *tiled = src_im;

				if (clip_width * Scr.MyDisplayHeight >
						((clip_height * 5) / 4) * Scr.MyDisplayWidth)
					clip_width =
							(((clip_height * 5) / 4) * Scr.MyDisplayWidth) /
							Scr.MyDisplayHeight;
				else if (((clip_width * 5) / 4) * Scr.MyDisplayHeight <
								 clip_height * Scr.MyDisplayWidth)
					clip_height =
							(((clip_width * 5) / 4) * Scr.MyDisplayHeight) /
							Scr.MyDisplayWidth;
				if (clip_width != src_im->width || clip_height != src_im->height) {
					display_progress (True, "Croping background to %dx%d ...",
														clip_width, clip_height);
					LOCAL_DEBUG_OUT ("Croping background to %dx%d ...", clip_width,
													 clip_height);
					tiled =
							tile_asimage (Scr.asv, src_im,
														(src_im->width - clip_width) / 2,
														(src_im->height - clip_height) / 2, clip_width,
														clip_height, TINT_LEAVE_SAME, ASA_ASImage, 0,
														ASIMAGE_QUALITY_DEFAULT);
				}
				if (tiled) {
					ASImage *scaled = tiled;
					if (tiled->width != Scr.MyDisplayWidth
							|| tiled->height != Scr.MyDisplayHeight) {
						display_progress (True, "Scaling background to %dx%d ...",
															Scr.MyDisplayWidth, Scr.MyDisplayHeight);
						LOCAL_DEBUG_OUT ("Scaling background to %dx%d ...",
														 Scr.MyDisplayWidth, Scr.MyDisplayHeight);
						scaled =
								scale_asimage (Scr.asv, tiled, Scr.MyDisplayWidth,
															 Scr.MyDisplayHeight, ASA_ASImage, 0,
															 ASIMAGE_QUALITY_DEFAULT);
					}
					if (scaled && scaled != src_im) {
						display_progress (True,
															"Saving transformed background into \"%s\" ...",
															realfilename);
						LOCAL_DEBUG_OUT
								("Saving transformed background into \"%s\" ...",
								 realfilename);
						success =
								save_asimage_to_file (realfilename, scaled, "png", "9",
																			NULL, 0, True);
					}
					if (scaled != tiled)
						destroy_asimage (&scaled);
					if (tiled != src_im)
						destroy_asimage (&tiled);
				}
			}
			safe_asimage_destroy (src_im);
		}
	}

	if (!success) {								/* just tile as usuall */
		display_progress (True,
											"Copying selected background \"%s\" into \"%s\" ...",
											data->text, realfilename);
		LOCAL_DEBUG_OUT ("Copying selected background \"%s\" into \"%s\" ...",
										 data->text, realfilename);
		success = (CopyFile (data->text, realfilename) == 0);
	}

	if (success) {
		++_as_background_change_count;
		if (Scr.CurrentDesk == 0)
			update_default_session (Session, F_CHANGE_BACKGROUND);
		change_desk_session (Session, Scr.CurrentDesk, realfilename,
												 F_CHANGE_BACKGROUND);
	}
	free (realfilename);

	commit_config_change (F_CHANGE_BACKGROUND);
	remove_desktop_cover ();

	XUngrabPointer (dpy, CurrentTime);
	XSync (dpy, 0);
	--_as_config_change_recursion;
}


typedef struct {
	char *url;
	char *cachedName;
	time_t downloadStart;
	FunctionData fdata;
	int pid;
} WebBackgroundDownloadAuxData;

static WebBackgroundDownloadAuxData webBackgroundDownloadAuxData =
		{ NULL, NULL, 0, {0}, 0 };


static void webBackgroundDownloadHandler (void *data)
{
	WebBackgroundDownloadAuxData *wb = (WebBackgroundDownloadAuxData *) data;
	int s1, s2;
	Bool downloadComplete =
			check_download_complete (wb->pid, wb->cachedName, &s1, &s2);
	if (downloadComplete) {
		if (s1 == 0 || s1 != s2) {
			downloadComplete = False;
			show_warning
					("Failed to download \"%s\", see \"%s.log\" for details",
					 wb->url, wb->cachedName);
			return;										/* download failed */
		}
		asdbus_Notify ("Image download complete", wb->url, -1);
	}

	if (downloadComplete) {
		change_background_internal (&(wb->fdata), True);
	} else
		timer_new (300, &webBackgroundDownloadHandler, data);
}

void change_background_func_handler (FunctionData * data, ASEvent * event,
																		 int module)
{
	timer_remove_by_data (&webBackgroundDownloadAuxData);
	change_background_internal (data, False);
}

void change_back_foreign_func_handler (FunctionData * data,
																			 ASEvent * event, int module)
{
	timer_remove_by_data (&webBackgroundDownloadAuxData);

	if (data->text != NULL && is_web_background (data)) {
		char *cachedFileName =
				make_session_webcache_file (Session, data->text);

		set_string (&(webBackgroundDownloadAuxData.url),
								mystrdup (data->text));
		set_string (&(webBackgroundDownloadAuxData.cachedName),
								cachedFileName);
		webBackgroundDownloadAuxData.downloadStart = time (NULL);
		webBackgroundDownloadAuxData.fdata = *data;
		webBackgroundDownloadAuxData.fdata.text = cachedFileName;

		if (CheckFile (cachedFileName) == 0)
			change_background_internal (&(webBackgroundDownloadAuxData.fdata),
																	True);
		else if ((webBackgroundDownloadAuxData.pid =
							spawn_download (webBackgroundDownloadAuxData.url,
															cachedFileName)) != 0) {
			asdbus_Notify ("Image download started",
										 webBackgroundDownloadAuxData.url, -1);
			timer_new (300, &webBackgroundDownloadHandler,
								 (void *)&webBackgroundDownloadAuxData);
		}
	} else
		change_background_internal (data, True);
}


void change_theme_func_handler (FunctionData * data, ASEvent * event,
																int module)
{
	++_as_config_change_recursion;
	if (install_theme_file (data->text))
		++_as_config_change_count;

	/* theme installation may trigger recursive look and feel changes - we
	 * don't want those to cause any restarts or config reloads.
	 */
	commit_config_change (data->func);

	--_as_config_change_recursion;
}


void change_config_func_handler (FunctionData * data, ASEvent * event,
																 int module)
{

	char *file_template;
	char tmpfile[256], *realfilename = NULL;
	int desk = 0;

	++_as_config_change_recursion;
#ifdef DIFFERENTLOOKNFEELFOREACHDESKTOP
	desk = Scr.CurrentDesk;
#endif
	if (Scr.screen == 0) {
		switch (data->func) {
		case F_CHANGE_LOOK:
			file_template = LOOK_FILE;
			break;
		case F_CHANGE_FEEL:
			file_template = FEEL_FILE;
			break;
		case F_CHANGE_COLORSCHEME:
			file_template = COLORSCHEME_FILE;
			break;
		default:
			file_template = THEME_FILE;
			break;
		}
		sprintf (tmpfile, file_template, desk);
	} else {
		switch (data->func) {
		case F_CHANGE_LOOK:
			file_template = LOOK_FILE ".scr%ld";
			break;
		case F_CHANGE_FEEL:
			file_template = FEEL_FILE ".scr%ld";
			break;
		case F_CHANGE_COLORSCHEME:
			file_template = COLORSCHEME_FILE ".scr%ld";
			break;
		default:
			file_template = THEME_FILE ".scr%ld";
			break;
		}
		sprintf (tmpfile, file_template, desk, Scr.screen);
	}

	realfilename = make_session_data_file (Session, False, 0, tmpfile, NULL);

	cover_desktop ();
	display_progress (True,
										"Copying selected config file \"%s\" into \"%s\" ...",
										data->text, realfilename);

	if (CopyFile (data->text, realfilename) == 0) {
		++_as_config_change_count;
		if (Scr.CurrentDesk == 0)
			update_default_session (Session, data->func);
		change_desk_session (Session, Scr.CurrentDesk, realfilename,
												 data->func);
	}
	free (realfilename);

	/* theme installation may trigger recursive look and feel changes - we
	 * don't want those to cause any restarts or config reloads.
	 */
	commit_config_change (data->func);
	remove_desktop_cover ();

	--_as_config_change_recursion;
}

void install_file_func_handler (FunctionData * data, ASEvent * event,
																int module)
{
	char *file = NULL;
	char *realfilename = NULL;
	Bool desktop_resource = False;
	char *dir_name = NULL;

	switch (data->func) {
	case F_INSTALL_LOOK:
		dir_name = as_look_dir_name;
		break;
	case F_INSTALL_FEEL:
		dir_name = as_feel_dir_name;
		break;
	case F_INSTALL_BACKGROUND:
		dir_name = as_background_dir_name;
		break;
	case F_INSTALL_FONT:
		dir_name = as_font_dir_name;
		desktop_resource = True;
		break;
	case F_INSTALL_ICON:
		dir_name = as_icon_dir_name;
		desktop_resource = True;
		break;
	case F_INSTALL_TILE:
		dir_name = as_tile_dir_name;
		desktop_resource = True;
		break;
	case F_INSTALL_THEME_FILE:
		dir_name = as_theme_file_dir_name;
		break;
	case F_INSTALL_COLORSCHEME:
		dir_name = as_colorscheme_dir_name;
		break;
	}
	if (dir_name != NULL) {
		parse_file_name (data->text, NULL, &file);

		cover_desktop ();
		display_progress (True, "Installing file \"%s\" into \"%s\" ...",
											data->text, dir_name);

		if (desktop_resource) {
			realfilename =
					make_session_data_file (Session, False, 0, DESKTOP_DIR, NULL);
			CheckOrCreate (realfilename);
			free (realfilename);
		}

		realfilename =
				make_session_data_file (Session, False, 0, dir_name, NULL);
		CheckOrCreate (realfilename);
		free (realfilename);

		realfilename =
				make_session_data_file (Session, False, 0, dir_name, file, NULL);
		CopyFile (data->text, realfilename);
		free (realfilename);
		free (file);
		remove_desktop_cover ();
	}
}

void install_feel_func_handler (FunctionData * data, ASEvent * event,
																int module)
{

}

void install_background_func_handler (FunctionData * data, ASEvent * event,
																			int module)
{

}

void install_font_func_handler (FunctionData * data, ASEvent * event,
																int module)
{

}

void install_icon_func_handler (FunctionData * data, ASEvent * event,
																int module)
{

}

void install_tile_func_handler (FunctionData * data, ASEvent * event,
																int module)
{

}

void install_theme_file_func_handler (FunctionData * data, ASEvent * event,
																			int module)
{

}


void save_workspace_func_handler (FunctionData * data, ASEvent * event,
																	int module)
{
	save_aswindow_list (Scr.Windows, data->text ? data->text : NULL, False);
}

Bool send_client_message_iter_func (void *data, void *aux_data)
{
	XClientMessageEvent *ev = (XClientMessageEvent *) aux_data;
	ASWindow *asw = (ASWindow *) data;

	ev->window = asw->w;
	XSendEvent (dpy, asw->w, False, 0, (XEvent *) ev);

	return True;
}


void signal_reload_GTKRC_file_handler (FunctionData * data,
																			 ASEvent * event, int module)
{
	XClientMessageEvent ev;

	memset (&ev, 0x00, sizeof (ev));
	ev.type = ClientMessage;
	ev.display = dpy;
	ev.message_type = _GTK_READ_RCFILES;
	ev.format = 8;

	iterate_asbidirlist (Scr.Windows->clients, send_client_message_iter_func,
											 &ev, NULL, False);
}

Bool send_kipc_client_message_iter_func (void *data, void *aux_data)
{
	XClientMessageEvent *ev = (XClientMessageEvent *) aux_data;
	ASWindow *asw = (ASWindow *) data;

	/* KDE not always sets this flag ??? */
	if (get_flags (asw->hints->protocols, AS_DoesKIPC)) {
		ev->window = asw->w;
		XSendEvent (dpy, asw->w, False, 0, (XEvent *) ev);
	}

	return True;
}

void KIPC_sendMessageAll (KIPC_Message msg, int data)
{
	XClientMessageEvent ev;

	memset (&ev, 0x00, sizeof (ev));
	ev.type = ClientMessage;
	ev.display = dpy;
	ev.message_type = _KIPC_COMM_ATOM;
	ev.format = 32;
	ev.data.l[0] = msg;
	ev.data.l[1] = data;

	iterate_asbidirlist (Scr.Windows->clients, send_client_message_iter_func,
											 &ev, NULL, False);
	/*  iterate_asbidirlist( Scr.Windows->clients, send_kipc_client_message_iter_func, &ev, NULL, False ); */
}



void KIPC_send_message_all_handler (FunctionData * data, ASEvent * event,
																		int module)
{
	KIPC_sendMessageAll ((KIPC_Message) data->func_val[0],
											 data->func_val[1]);

}

void refresh_func_handler (FunctionData * data, ASEvent * event,
													 int module)
{
	XSetWindowAttributes attributes;
	unsigned long valuemask;
	Window w;

	valuemask = (CWBackPixmap | CWBackingStore | CWOverrideRedirect);
	attributes.background_pixmap = None;
	attributes.backing_store = NotUseful;
	attributes.override_redirect = True;

	w = create_visual_window (Scr.asv, Scr.Root, 0, 0,
														Scr.MyDisplayWidth, Scr.MyDisplayHeight,
														0, InputOutput, valuemask, &attributes);

	XMapRaised (dpy, w);
	XSync (dpy, False);
	XDestroyWindow (dpy, w);
	XFlush (dpy);

	spool_unfreed_mem ("afterstep.allocs.refresh", NULL);
}

void goto_page_func_handler (FunctionData * data, ASEvent * event,
														 int module)
{
#ifndef NO_VIRTUAL
	int newvx = data->func_val[0] * event->scr->MyDisplayWidth;
	int newvy = data->func_val[1] * event->scr->MyDisplayHeight;
	LOCAL_DEBUG_OUT ("val(%d,%d)->scr(%d,%d)->newv(%d,%d)",
									 (int)data->func_val[0], (int)data->func_val[1],
									 event->scr->MyDisplayWidth, event->scr->MyDisplayHeight,
									 newvx, newvy);
	MoveViewport (newvx, newvy, True);
#endif
}

void toggle_page_func_handler (FunctionData * data, ASEvent * event,
															 int module)
{
#ifndef NO_VIRTUAL
	if (get_flags (Scr.Feel.flags, DoHandlePageing))
		clear_flags (Scr.Feel.flags, DoHandlePageing);
	else
		set_flags (Scr.Feel.flags, DoHandlePageing);

	SendPacket (-1, M_TOGGLE_PAGING, 1,
							get_flags (Scr.Feel.flags, DoHandlePageing));
	check_screen_panframes (ASDefaultScr);
#endif
}

void gethelp_func_handler (FunctionData * data, ASEvent * event,
													 int module)
{
	if (event->client != NULL)
		if (ASWIN_RES_NAME (event->client) != NULL) {
			char *realfilename = PutHome (HELPCOMMAND);
			spawn_child (realfilename, -1, -1, NULL, None, C_NO_CONTEXT, True,
									 False, ASWIN_RES_NAME (event->client), NULL);
			free (realfilename);
		}
}

void wait_func_handler (FunctionData * data, ASEvent * event, int module)
{
	ASWindow *asw;
	char *complex_pattern = data->text;
	if (data->name && data->name[1] == ':')
		complex_pattern = &(data->name[2]);

	asw = WaitWindowLoop (complex_pattern, -1);
	LOCAL_DEBUG_OUT
			("Wait completed for \"%s\", asw = %p, data->text = \"%s\"",
			 complex_pattern, asw, data->text ? data->text : "(null)");
	if (asw && data->text) {
		/* 1. parse text into a name_list struct */
		name_list *style = string2DatabaseStyle (data->text);
		LOCAL_DEBUG_OUT
				("style = %p, set_data_flags = 0x%lX, set_flags = 0x%lX", style,
				 style ? style->set_data_flags : 0, style ? style->set_flags : 0);
		if (style) {								/* 2. apply set data from name_list to asw */
			int new_vx = Scr.Vx;
			int new_vy = Scr.Vy;
			int x, y;
			int width, height;
			ASFlagType geom_flags =
					get_flags (style->set_data_flags,
										 STYLE_DEFAULT_GEOMETRY) ? style->default_geometry.
					flags : 0;
			ASFlagType state_flags = 0;

			if (get_flags (style->set_data_flags, STYLE_VIEWPORTX))
				new_vx = style->ViewportX;
			else
				new_vx = asw->status->viewport_x;
			if (get_flags (style->set_data_flags, STYLE_VIEWPORTY))
				new_vy = style->ViewportY;
			else
				new_vy = asw->status->viewport_y;

			x = get_flags (geom_flags,
										 XValue) ? style->default_geometry.x : asw->status->x;
			y = get_flags (geom_flags,
										 YValue) ? style->default_geometry.y : asw->status->y;
			width =
					get_flags (geom_flags,
										 WidthValue) ? (style->default_geometry.width +
																		asw->status->frame_size[FR_W] +
																		asw->status->frame_size[FR_E]) : asw->
					status->width;
			height =
					get_flags (geom_flags,
										 HeightValue) ? (style->default_geometry.height +
																		 asw->status->frame_size[FR_N] +
																		 asw->status->frame_size[FR_S]) : asw->
					status->height;

			if (get_flags (style->set_flags, STYLE_STICKY)) {
				if ((get_flags (style->flags, STYLE_STICKY)
						 && !ASWIN_GET_FLAGS (asw, AS_Sticky))
						|| (!get_flags (style->flags, STYLE_STICKY)
								&& ASWIN_GET_FLAGS (asw, AS_Sticky)))
					set_flags (state_flags, AS_Sticky);
			}
			if (get_flags (style->set_flags, STYLE_FULLSCREEN)) {
				if ((get_flags (style->flags, STYLE_FULLSCREEN)
						 && !ASWIN_GET_FLAGS (asw, AS_Fullscreen))
						|| (!get_flags (style->flags, STYLE_FULLSCREEN)
								&& ASWIN_GET_FLAGS (asw, AS_Fullscreen)))
					set_flags (state_flags, AS_Fullscreen);
			}

			if (state_flags != 0)
				toggle_aswindow_status (asw, state_flags);

			if (!ASWIN_GET_FLAGS (asw, AS_Sticky)) {
				if (get_flags (style->set_data_flags, STYLE_STARTUP_DESK))
					change_aswindow_desktop (asw, style->Desk, False);
			} else {
				new_vx = Scr.Vx;
				new_vy = Scr.Vy;
			}

			LOCAL_DEBUG_OUT
					("%s -> viewport : new(%+d%+d)old(%+d%+d) ; geom : new(%dx%d%+d%+d)old(%dx%d%+d%+d)",
					 ASWIN_GET_FLAGS (asw, AS_Sticky) ? "sticky" : "slippery",
					 new_vx, new_vy, asw->status->viewport_x,
					 asw->status->viewport_y, width, height, x, y,
					 asw->status->width, asw->status->height, asw->status->x,
					 asw->status->y);


			if (get_flags
					(style->set_data_flags,
					 STYLE_DEFAULT_GEOMETRY | STYLE_VIEWPORTX | STYLE_VIEWPORTY)) {
				x += Scr.Vx - asw->status->viewport_x + new_vx;
				y += Scr.Vy - asw->status->viewport_y + new_vy;
				moveresize_aswindow_wm (asw, x, y, width, height, False);
			}

			if (get_flags (style->set_data_flags, STYLE_LAYER)
					&& ASWIN_LAYER (asw) != style->layer) {
				LOCAL_DEBUG_OUT ("new_layer = %d", style->layer);
				change_aswindow_layer (asw, style->layer);
			}

			if (get_flags (style->set_flags, STYLE_START_ICONIC)) {
				if ((get_flags (style->flags, STYLE_START_ICONIC)
						 && !ASWIN_GET_FLAGS (asw, AS_Iconic))
						|| (!get_flags (style->flags, STYLE_START_ICONIC)
								&& ASWIN_GET_FLAGS (asw, AS_Iconic)))
					set_window_wm_state (event->client,
															 get_flags (style->flags,
																					STYLE_START_ICONIC), False);
			}

			style_delete (style, NULL);
		}
	}
	XSync (dpy, 0);
}

void desk_func_handler (FunctionData * data, ASEvent * event, int module)
{
	long new_desk;

	if (data->func_val[0] != 0)
		new_desk = Scr.CurrentDesk + data->func_val[0];
	else if (IsValidDesk (data->func_val[1]))
		new_desk = data->func_val[1];
	else
		return;
	ChangeDesks (new_desk);
}

void deskviewport_func_handler (FunctionData * data, ASEvent * event,
																int module)
{
	unsigned int new_desk1, new_desk2;
	int new_vx, new_vy, flags;
	int new_desk;

	if (data->text == NULL)
		return;
	if (parse_geometry
			(data->text, &new_vx, &new_vy, &new_desk1, &new_desk2,
			 &flags) == data->text)
		return;

	if (!get_flags (flags, XValue))
		new_vx = Scr.Vx;

	if (!get_flags (flags, YValue))
		new_vy = Scr.Vy;

	if (get_flags (flags, WidthValue))
		new_desk = new_desk1;
	else if (get_flags (flags, HeightValue))
		new_desk = new_desk2;
	else
		new_desk = Scr.CurrentDesk;

	ChangeDeskAndViewport (new_desk, new_vx, new_vy, False);
}

void module_func_handler (FunctionData * data, ASEvent * event, int module)
{
	UngrabEm ();
	ExecModule (data->text, event->client ? event->client->w : None,
							event->context);
}

void killmodule_func_handler (FunctionData * data, ASEvent * event,
															int module)
{
	KillModuleByName (data->text);
}

void killallmodules_func_handler (FunctionData * data, ASEvent * event,
																	int module)
{
	KillAllModulesByName (data->text);
}

void restartmodule_func_handler (FunctionData * data, ASEvent * event,
																 int module)
{
	char *cmd_line = GetModuleCmdLineByName (data->text);
	KillModuleByName (data->text);

	UngrabEm ();
	ExecModule (cmd_line ? cmd_line : data->text,
							event->client ? event->client->w : None, event->context);
	if (cmd_line)
		free (cmd_line);
}

void popup_func_handler (FunctionData * data, ASEvent * event, int module)
{
	run_menu (data->text ? data->text : data->name,
						event->client ? event->client->w : None);
}

void quit_func_handler (FunctionData * data, ASEvent * event, int module)
{
	if (!RequestLogout ())
		Done (0, NULL);
}

void windowlist_func_handler (FunctionData * data, ASEvent * event,
															int module)
{
#ifndef NO_WINDOWLIST
	MenuData *md =
			make_desk_winlist_menu (Scr.Windows,
															data->text ==
															NULL ? event->scr->CurrentDesk : data->
															func_val[0], Scr.Feel.winlist_sort_order,
															False);
	if (md != NULL) {
		ASMenu *menu = run_menu_data (md);
		/* attaching menu data to menu, so that we can destroy it when menu closes.
		   This is to accomodate those dynamically generated menus, 
		   such as window list and module list */
		/* Crude! should implement reference counting instead  */

		if (menu)
			menu->volitile_menu_data = md;
	}
#endif													/* ! NO_WINDOWLIST */
}

void modulelist_func_handler (FunctionData * data, ASEvent * event,
															int module)
{
	MenuData *md = (data->func == F_STOPMODULELIST) ?
			make_stop_module_menu (Scr.Feel.winlist_sort_order) :
			make_restart_module_menu (Scr.Feel.winlist_sort_order);
	if (md != NULL) {
		ASMenu *menu = run_menu_data (md);
		if (menu)
			menu->volitile_menu_data = md;
	}

}

void quickrestart_func_handler (FunctionData * data, ASEvent * event,
																int module)
{
	QuickRestart (data->text);
}

Bool send_aswindow_data_iter_func (void *data, void *aux_data)
{
	union {
		void *ptr;
		int id;
	} module_id;
	ASWindow *asw = (ASWindow *) data;

	module_id.ptr = aux_data;

	SendConfig (module_id.id, M_CONFIGURE_WINDOW, asw);
	/* always start with RES_CLASS to let module know if this is a DockApp or not */
	SendString (module_id.id, M_RES_CLASS, asw->w, asw->frame, asw,
							asw->hints->res_class, get_hint_name_encoding (asw->hints,
																														 asw->hints->
																														 res_class_idx));
	SendString (module_id.id, M_RES_NAME, asw->w, asw->frame, asw,
							asw->hints->res_name, get_hint_name_encoding (asw->hints,
																														asw->hints->
																														res_name_idx));
	SendString (module_id.id, M_ICON_NAME, asw->w, asw->frame, asw,
							asw->hints->icon_name, get_hint_name_encoding (asw->hints,
																														 asw->hints->
																														 icon_name_idx));
	SendString (module_id.id, M_WINDOW_NAME, asw->w, asw->frame, asw,
							asw->hints->names[0], get_hint_name_encoding (asw->hints,
																														0));
	return True;
}

void send_window_list_func_handler (FunctionData * data, ASEvent * event,
																		int module)
{
	if (module >= 0) {
		union {
			void *ptr;
			int id;
		} module_id;
		module_id.id = module;
		SendPacket (module, M_TOGGLE_PAGING, 1, DoHandlePageing);
		SendPacket (module, M_NEW_DESKVIEWPORT, 3, Scr.Vx, Scr.Vy,
								Scr.CurrentDesk);
		iterate_asbidirlist (Scr.Windows->clients,
												 send_aswindow_data_iter_func, module_id.ptr, NULL,
												 False);
		SendPacket (module, M_END_WINDOWLIST, 0);
		if (IsValidDesk (Scr.CurrentDesk))
			send_stacking_order (Scr.CurrentDesk);
	}
}

void screenshot_func_handler (FunctionData * data, ASEvent * event,
															int module)
{
	ASImage *im;
	Window target = None;
	sleep_a_millisec (300);
	if (event->client && data->func != F_TAKE_SCREENSHOT)
		target =
				(data->func ==
				 F_TAKE_WINDOWSHOT) ? event->client->w : event->client->frame;
	im = grab_root_asimage (ASDefaultScr, target, True);
	LOCAL_DEBUG_OUT ("grab_root_image returned %p", im);
	if (im != NULL) {
		char *realfilename = NULL;
		Bool replace = True;
		char *type = NULL;
		char *compress = NULL;			/* default compression */
#ifdef DONT_REPLACE_SCREENSHOT_FILES
		replace = False;
#endif
		if (data->text != NULL) {
			realfilename = PutHome (data->text);
			type = strrchr (realfilename, '.');
			if (type != NULL) {
				++type;
				if (mystrcasecmp (type, "jpg") == 0
						|| mystrcasecmp (type, "jpeg") == 0)
					compress = "0";
			}
		}
		if (realfilename == NULL) {
			char *capture_file_name = DEFAULT_CAPTURE_SCREEN_FILE;
			char *default_template;
			if (data->func == F_TAKE_WINDOWSHOT)
				capture_file_name = DEFAULT_CAPTURE_WINDOW_FILE;
			else if (data->func == F_TAKE_FRAMESHOT)
				capture_file_name = DEFAULT_CAPTURE_FRAMEDWINDOW_FILE;
			default_template = safemalloc (strlen (capture_file_name) + 100);
			sprintf (default_template, "%s.%lu.png", capture_file_name,
							 time (NULL));
			realfilename = PutHome (default_template);
			free (default_template);
			compress = "9";
			type = "png";
		}

		if (save_asimage_to_file
				(realfilename, im, type, compress, NULL, 0, replace))
			show_warning ("screenshot saved as \"%s\"", realfilename);
		free (realfilename);
		destroy_asimage (&im);
	}
}

void swallow_window_func_handler (FunctionData * data, ASEvent * event,
																	int module)
{
	if (event->client) {
		if (data->text)
			module = FindModuleByName (data->text);
		SendPacket (module, M_SWALLOW_WINDOW, 2, event->client->w,
								event->client->frame);
	}
}

void set_func_handler (FunctionData * data, ASEvent * event, int module)
{
	if (data->text != NULL) {
		char *eq_ptr = strchr (data->text, '=');
		if (eq_ptr) {
			int val = 0;
			char *tail = eq_ptr + 1;
			val = (int)parse_math (tail, &tail, strlen (tail));
			LOCAL_DEBUG_OUT ("tail = \"%s\", val = %d", tail, val);
			if (tail > eq_ptr + 1) {
				*eq_ptr = '\0';
				asxml_var_insert (data->text, val);
				if (strncmp (data->text, "menu.", 5) == 0) {
					ASFlagType change_flag = 0;
					ASFlagType old_look_flags = Scr.Look.flags;
					if (strcmp (data->text, ASXMLVAR_MenuShowMinipixmaps) == 0)
						change_flag = MenuMiniPixmaps;
					else if (strcmp (data->text, ASXMLVAR_MenuShowUnavailable) == 0)
						change_flag = MenuShowUnavailable;
					else if (strcmp (data->text, ASXMLVAR_MenuTxtItemsInd) == 0)
						change_flag = TxtrMenuItmInd;
					if (change_flag != 0) {
						if (!val)
							clear_flags (Scr.Look.flags, change_flag);
						else
							set_flags (Scr.Look.flags, change_flag);
					} else if (strcmp (data->text, ASXMLVAR_MenuRecentSubmenuItems)
										 == 0) {
						Scr.Feel.recent_submenu_items = (val <= 0) ? 0 : val;
					}

					if (old_look_flags != Scr.Look.flags) {
						/* need to referesh menus maybe? */
					}
					LOCAL_DEBUG_OUT ("old_flags = %lX, flags = %lX", old_look_flags,
													 Scr.Look.flags);
				}
				*eq_ptr = '=';
			}
		}
	}
}

void test_func_handler (FunctionData * data, ASEvent * event, int module)
{
	/* add test command processing here : */
/*         fprintf( stderr, "Testing <do_menu_new( \"Looks\", NULL ) ...\n" ); */
/*         do_menu_new( "Look", NULL, NULL ); */
	fprintf (stderr, "Testing completed\n");
}

void QuickRestart (char *what)
{
	unsigned long what_flags = 0;
	Bool update_background = False;

	if (what == NULL)
		return;

	if (strcasecmp (what, "all") == 0 || strcasecmp (what, "theme") == 0)
		what_flags = PARSE_EVERYTHING;
	else if (strcasecmp (what, "look&feel") == 0)
		what_flags = PARSE_LOOK_CONFIG | PARSE_FEEL_CONFIG;
	else if (strcasecmp (what, "startmenu") == 0
					 || strcasecmp (what, "feel") == 0)
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
	if (what) {
		InstallRootColormap ();
		GrabEm (ASDefaultScr, Scr.Feel.cursors[ASCUR_Wait]);
		LoadASConfig (Scr.CurrentDesk, what_flags);
		UngrabEm ();
	}

	if (update_background)
		SendPacket (-1, M_NEW_BACKGROUND, 1, 1);
	SendPacket (-1, M_NEW_CONFIG, 1, what_flags);
}
