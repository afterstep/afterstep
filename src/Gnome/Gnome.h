/* Copyright (C) 1999 Rafal Wierzbicki <rafal@mcss.mcmaster.ca>
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
 * $Id: Gnome.h,v 1.2 2002/08/04 06:48:07 sashav Exp $
 */

#define WIN_HINTS_SKIP_FOCUS      (1<<0) /*"alt-tab" skips this win*/
#define WIN_HINTS_SKIP_WINLIST    (1<<1) /*do not show in window list*/
#define WIN_HINTS_SKIP_TASKBAR    (1<<2) /*do not show on taskbar*/
#define WIN_HINTS_GROUP_TRANSIENT (1<<3) /*Reserved - definition is unclear*/
#define WIN_HINTS_FOCUS_ON_CLICK  (1<<4) /*app only accepts focus if clicked*/
#define WIN_HINTS_DO_NOT_COVER    (1<<5) /*attempt to not cover this window */

#define WIN_STATE_STICKY          (1<<0) /*everyone knows sticky*/
#define WIN_STATE_MINIMIZED       (1<<1) /*Reserved - definition is unclear*/
#define WIN_STATE_MAXIMIZED_VERT  (1<<2) /*window in maximized V state*/
#define WIN_STATE_MAXIMIZED_HORIZ (1<<3) /*window in maximized H state*/
#define WIN_STATE_HIDDEN          (1<<4) /*not on taskbar but window visible*/
#define WIN_STATE_SHADED          (1<<5) /*shaded (MacOS / Afterstep style)*/
#define WIN_STATE_HID_WORKSPACE   (1<<6) /*not on current desktop*/
#define WIN_STATE_HID_TRANSIENT   (1<<7) /*owner of transient is hidden*/
#define WIN_STATE_FIXED_POSITION  (1<<8) /*window is fixed in position even*/
#define WIN_STATE_ARRANGE_IGNORE  (1<<9) /*ignore for auto arranging*/

/*
#define WIN_LAYER_DESKTOP    0
#define WIN_LAYER_BELOW      2
#define WIN_LAYER_NORMAL     4
#define WIN_LAYER_ONTOP      6
#define WIN_LAYER_DOCK       8
#define WIN_LAYER_ABOVE_DOCK 10
#define WIN_LAYER_MENU       12
*/

/* flags for the window layer */
typedef enum
{
  WIN_APP_STATE_NONE,
  WIN_APP_STATE_ACTIVE1,
  WIN_APP_STATE_ACTIVE2,
  WIN_APP_STATE_ERROR1,
  WIN_APP_STATE_ERROR2,
  WIN_APP_STATE_FATAL_ERROR1,
  WIN_APP_STATE_FATAL_ERROR2,
  WIN_APP_STATE_IDLE1,
  WIN_APP_STATE_IDLE2,
  WIN_APP_STATE_WAITING1,
  WIN_APP_STATE_WAITING2,
  WIN_APP_STATE_WORKING1,
  WIN_APP_STATE_WORKING2,
  WIN_APP_STATE_NEED_USER_INPUT1,
  WIN_APP_STATE_NEED_USER_INPUT2,
  WIN_APP_STATE_STRUGGLING1,
  WIN_APP_STATE_STRUGGLING2,
  WIN_APP_STATE_DISK_TRAFFIC1,
  WIN_APP_STATE_DISK_TRAFFIC2,
  WIN_APP_STATE_NETWORK_TRAFFIC1,
  WIN_APP_STATE_NETWORK_TRAFFIC2,
  WIN_APP_STATE_OVERLOADED1,
  WIN_APP_STATE_OVERLOADED2,
  WIN_APP_STATE_PERCENT000_1,
  WIN_APP_STATE_PERCENT000_2,
  WIN_APP_STATE_PERCENT010_1,
  WIN_APP_STATE_PERCENT010_2,
  WIN_APP_STATE_PERCENT020_1,
  WIN_APP_STATE_PERCENT020_2,
  WIN_APP_STATE_PERCENT030_1,
  WIN_APP_STATE_PERCENT030_2,
  WIN_APP_STATE_PERCENT040_1,
  WIN_APP_STATE_PERCENT040_2,
  WIN_APP_STATE_PERCENT050_1,
  WIN_APP_STATE_PERCENT050_2,
  WIN_APP_STATE_PERCENT060_1,
  WIN_APP_STATE_PERCENT060_2,
  WIN_APP_STATE_PERCENT070_1,
  WIN_APP_STATE_PERCENT070_2,
  WIN_APP_STATE_PERCENT080_1,
  WIN_APP_STATE_PERCENT080_2,
  WIN_APP_STATE_PERCENT090_1,
  WIN_APP_STATE_PERCENT090_2,
  WIN_APP_STATE_PERCENT100_1,
  WIN_APP_STATE_PERCENT100_2
} GnomeWinAppState;

/* masks for AS pipe */
#define  mask_reg (M_ADD_WINDOW | M_CONFIGURE_WINDOW | M_DESTROY_WINDOW | M_TOGGLE_PAGING | M_NEW_PAGE | M_NEW_DESK | M_END_WINDOWLIST)

typedef struct node
{
    long id;
    long flags;
    int workspace;
    int area_x;
    int area_y;
    int old_area_x;
    int old_area_y;
    struct node *previous;
    struct node *next;
}list_item;

typedef struct
{
    list_item *first, *last;
}s_list;

int s_list_remove_by_data (s_list *list, long id);
list_item * s_list_find_by_data (s_list *list, long id);

static void gnome_compliance_init ();
static void gnome_get_prop_workspace (Window id);

static int list_configure (unsigned long *body);
static int list_add_window (unsigned long *body);
static int list_window_name  (unsigned long *body);
static int list_icon_name (unsigned long *body);
static int list_destroy_window (unsigned long *body);
static int list_end ();
static int list_iconify (unsigned long *body);
static int list_deiconify (unsigned long *body);
static int list_desk_change (unsigned long *body);
static int list_page_change (unsigned long *body);

static void read_as_socket ();

static Atom _XA_WIN_SUPPORTING_WM_CHECK;
static Atom _XA_WIN_PROTOCOLS;
static Atom _XA_WIN_LAYER;
static Atom _XA_WIN_STATE;
static Atom _XA_WIN_HINTS;
static Atom _XA_WIN_APP_STATE;
/*static Atom _XA_WIN_EXPANDED_SIZE;*/
static Atom _XA_WIN_ICONS;
static Atom _XA_WIN_WORKSPACE;
static Atom _XA_WIN_WORKSPACE_COUNT;
static Atom _XA_WIN_WORKSPACE_NAMES;
static Atom _XA_WIN_CLIENT_LIST;
static Atom _XA_WIN_DESKTOP_BUTTON_PROXY;
static Atom _XA_WIN_AREA_COUNT;
static Atom _XA_WIN_AREA;

char *MyName;
Display *dpy;
int screen;
static int fd[2], x_fd, fd_width;

static Window gnome_win, root_win;
