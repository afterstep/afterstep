/****************************************************************************
 * Copyright (c) 2008 Sasha Vasko <sasha at aftercode.net>
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
#define LOCAL_DEBUG

#include "asinternals.h"

#include <signal.h>
#include <fcntl.h>
#include <unistd.h>

#undef HAVE_DBUS_CONTEXT

#if defined(HAVE_GIOLIB) && defined(HAVE_GSETTINGS)
# include <gio/gio.h>
# define GSM_MANAGER_SCHEMA        "org.gnome.SessionManager"
# define KEY_AUTOSAVE              "auto-save-session"
#endif

#ifdef HAVE_DBUS1
# include "dbus/dbus.h"

# ifndef TEST_AS_DBUS
#  define AFTERSTEP_APP_ID			            "afterstep"
# else
#  define AFTERSTEP_APP_ID			            "afterstep-test"
# endif

#undef ASDBUS_DISPATCH

#define AFTERSTEP_DBUS_SERVICE_NAME	      "org.afterstep." AFTERSTEP_APP_ID
#define AFTERSTEP_DBUS_INTERFACE			    "org.afterstep." AFTERSTEP_APP_ID
#define AFTERSTEP_DBUS_ROOT_PATH			    "/org/afterstep/" AFTERSTEP_APP_ID

#define SESSIONMANAGER_NAME				"org.gnome.SessionManager"
#define SESSIONMANAGER_PATH				"/org/gnome/SessionManager"
#define SESSIONMANAGER_INTERFACE	"org.gnome.SessionManager"
#define IFACE_SESSION_PRIVATE 		SESSIONMANAGER_INTERFACE ".ClientPrivate"

#define CK_NAME      "org.freedesktop.ConsoleKit"
#define CK_PATH      "/org/freedesktop/ConsoleKit"
#define CK_INTERFACE "org.freedesktop.ConsoleKit"

#define CK_MANAGER_PATH      CK_PATH "/Manager"
#define CK_MANAGER_INTERFACE CK_NAME ".Manager"
#define CK_SEAT_INTERFACE    CK_NAME ".Seat"
#define CK_SESSION_INTERFACE CK_NAME ".Session"

#define UPOWER_NAME 			"org.freedesktop.UPower"
#define UPOWER_PATH 			"/org/freedesktop/UPower"
#define UPOWER_INTERFACE	"org.freedesktop.UPower"

#define KSMSERVER_NAME 			"org.kde.ksmserver"
#define KSMSERVER_PATH 			"/KSMServer"
#define KSMSERVER_INTERFACE	"org.kde.KSMServerInterface"

typedef enum  {
	KDE_ShutdownConfirmDefault = -1,
	KDE_ShutdownConfirmNo = 0,
	KDE_ShutdownConfirmYes = 1
}KDE_ShutdownConfirm;

typedef enum {
  KDE_ShutdownModeDefault = -1,
  KDE_ShutdownModeSchedule = 0,
  KDE_ShutdownModeTryNow = 1,
  KDE_ShutdownModeForceNow = 2,
  KDE_ShutdownModeInteractive = 3
}	KDE_ShutdownMode;

typedef enum {
  KDE_ShutdownTypeDefault = -1,
  KDE_ShutdownTypeNone = 0,
  KDE_ShutdownTypeReboot = 1,
 	KDE_ShutdownTypeHalt = 2,
  KDE_ShutdownTypeLogout = 3 /* unused - use None instead */
} KDE_ShutdownType;

typedef struct ASDBusOjectDescr {
	char *displayName;
	Bool systemBus;
	char *name;
	char *path;
	char *interface;
}ASDBusOjectDescr;

static ASDBusOjectDescr dbusSessionManager = {"Session Manager", False, SESSIONMANAGER_NAME, SESSIONMANAGER_PATH, SESSIONMANAGER_INTERFACE };
static ASDBusOjectDescr dbusUPower = {"Power Management Daemon", True, UPOWER_NAME, UPOWER_PATH, UPOWER_INTERFACE };

#define HAVE_DBUS_CONTEXT 1

typedef struct ASDBusContext {
	DBusConnection *system_conn;
	DBusConnection *session_conn;
	ASVector *watchFds;  // vector of ASDBusFds
	char *gnomeSessionPath;
	Bool sessionManagerCanShutdown;
	int kdeSessionVersion;
	ASBiDirList *dispatches;
} ASDBusContext;

static ASDBusContext ASDBus = { NULL, NULL, NULL, NULL, False, 0, NULL };

static DBusHandlerResult asdbus_handle_message (DBusConnection *,
																								DBusMessage *, void *);

static DBusObjectPathVTable ASDBusMessagesVTable = {
	NULL, asdbus_handle_message,	/* handler function */
	NULL, NULL, NULL, NULL
};

/******************************************************************************/
/* internal stuff */
/******************************************************************************/
DBusHandlerResult
asdbus_handle_message (DBusConnection * conn, DBusMessage * msg,
											 void *data)
{
	Bool handled = False;

	show_progress ("Dbus message received from \"%s\", member \"%s\"",
								 dbus_message_get_interface (msg),
								 dbus_message_get_member (msg));

	if (dbus_message_is_signal
			(msg, "org.gnome.SessionManager", "SessionOver")) {
		dbus_message_unref (msg);
		handled = True;
		Done (False, NULL);
	}

	return handled ? DBUS_HANDLER_RESULT_HANDLED :
			DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

}

static DBusConnection *
_asdbus_get_session_connection()
{
	DBusError error;
	int res;
	DBusConnection *session_conn;
	dbus_error_init (&error);

	session_conn = dbus_bus_get (DBUS_BUS_SESSION, &error);

	if (dbus_error_is_set (&error)) {
		show_error ("Failed to connect to Session DBus: %s", error.message);
	} else {
		dbus_connection_set_exit_on_disconnect (session_conn, FALSE);
		res = dbus_bus_request_name (session_conn,
																 AFTERSTEP_DBUS_SERVICE_NAME,
																 DBUS_NAME_FLAG_REPLACE_EXISTING |
																 DBUS_NAME_FLAG_ALLOW_REPLACEMENT,
																 &error);
		if (dbus_error_is_set (&error)) {
			show_error ("Failed to request name from DBus: %s", error.message);
		} else if (res != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
			show_error ("Failed to request name from DBus - not a primary owner.");
		} else {
			dbus_connection_register_object_path (session_conn,
																						AFTERSTEP_DBUS_ROOT_PATH,
																						&ASDBusMessagesVTable, 0);
		}
	}
	if (dbus_error_is_set (&error))
		dbus_error_free (&error);

	return session_conn;
}

static DBusConnection *
_asdbus_get_system_connection()
{
	DBusError error;
	DBusConnection *sys_conn;

	dbus_error_init (&error);
	sys_conn = dbus_bus_get (DBUS_BUS_SYSTEM, &error);

	if (dbus_error_is_set (&error)) {
		show_error ("Failed to connect to System DBus: %s", error.message);
		dbus_error_free (&error);
	} else {
		dbus_connection_set_exit_on_disconnect (sys_conn, FALSE);
		show_progress ("Connected to System DBus.");
	}
	return sys_conn;
}

/******************************************************************************/
/* Watch functions */
/******************************************************************************/
static dbus_bool_t add_watch(DBusWatch *w, void *data)
{
    	if (!dbus_watch_get_enabled(w))
      		return TRUE;

	ASDBusFd *fd = safecalloc (1, sizeof(ASDBusFd));
	fd->fd =  dbus_watch_get_unix_fd(w);
	unsigned int flags = dbus_watch_get_flags(w);
	if (get_flags(flags, DBUS_WATCH_READABLE))
		fd->readable = True;
    /*short cond = EV_PERSIST;
    if (flags & DBUS_WATCH_READABLE)
        cond |= EV_READ;
    if (flags & DBUS_WATCH_WRITABLE)
        cond |= EV_WRITE; */

      // TODO add to the list of FDs
	dbus_watch_set_data(w, fd, NULL);
	if (ASDBus.watchFds == NULL)
		ASDBus.watchFds = create_asvector (sizeof(ASDBusFd*));

	append_vector(ASDBus.watchFds, &fd, 1);

	show_debug(__FILE__,__FUNCTION__,__LINE__,"added dbus watch fd=%d watch=%p readable =%d\n", fd->fd, w, fd->readable);
	return TRUE;
}

static void remove_watch(DBusWatch *w, void *data)
{
    ASDBusFd* fd = dbus_watch_get_data(w);

    vector_remove_elem (ASDBus.watchFds, &fd);
    dbus_watch_set_data(w, NULL, NULL);
    show_debug(__FILE__,__FUNCTION__,__LINE__,"removed dbus watch watch=%p\n", w);
}

static void toggle_watch(DBusWatch *w, void *data)
{
    show_debug(__FILE__,__FUNCTION__,__LINE__,"toggling dbus watch watch=%p\n", w);
    if (dbus_watch_get_enabled(w))
        add_watch(w, data);
    else
        remove_watch(w, data);
}
#ifdef ASDBUS_DISPATCH
typedef struct ASDBusDispatch {
	DBusConnection *connection;
	void *data;
}ASDBusDispatch;

static ASDBusDispatch *asdbus_create_dispatch(DBusConnection *connection, void *data){
    ASDBusDispatch *d = safecalloc (1, sizeof(ASDBusDispatch));
    d->data = data;
    d->connection = connection;
    return d;
}

static void queue_dispatch(DBusConnection *connection, DBusDispatchStatus new_status, void *data){
	if (new_status == DBUS_DISPATCH_DATA_REMAINS){
	        show_debug(__FILE__,__FUNCTION__,__LINE__,"ADDED dbus dispatch=%p\n", data);
		append_bidirelem (ASDBus.dispatches, asdbus_create_dispatch(connection, data));
	}
}
#endif
static void  asdbus_handle_timer (void *vdata) {
	show_debug(__FILE__,__FUNCTION__,__LINE__,"dbus_timeout_handle data=%p\n", vdata);
	dbus_timeout_handle (vdata);
}

static void asdbus_set_dbus_timer (struct timeval *expires, DBusTimeout *timeout) {
	int interval = dbus_timeout_get_interval(timeout);
	gettimeofday (expires, NULL);
	tv_add_ms(expires, interval);
	show_debug(__FILE__,__FUNCTION__,__LINE__,"time = %d, adding dbus timeout data=%p, interval = %d\n", time(NULL), timeout, interval);
	timer_new (interval, asdbus_handle_timer, timeout);
}

static dbus_bool_t add_timeout(DBusTimeout *timeout, void *data){
	/* add expiration data to timeout */
	struct timeval *expires = dbus_malloc(sizeof(struct timeval));
	if (!expires)
		return FALSE;
	dbus_timeout_set_data(timeout, expires, dbus_free);

	asdbus_set_dbus_timer (expires, timeout);
	return TRUE;
}

static void toggle_timeout(DBusTimeout *timeout, void *data){
	/* reset expiration data */
	struct timeval *expires = dbus_timeout_get_data(timeout);
	timer_remove_by_data (timeout);

	asdbus_set_dbus_timer (expires, timeout);
}

static void remove_timeout(DBusTimeout *timeout, void *data){
	show_debug(__FILE__,__FUNCTION__,__LINE__,"removing dbus timeout =%p\n", timeout);
	timer_remove_by_data (timeout);
}
#ifdef ASDBUS_DISPATCH

void asdbus_dispatch_destroy (void *data) {
    free (data);
}
#endif
static _asdbus_add_match (DBusConnection *conn, const char* iface, const char* member) {
	char match[256];
	sprintf(match,	member?"type='signal',interface='%s',member='%s'":"type='signal',interface='%s'", iface, member);
    	DBusError error;
    	dbus_error_init(&error);
    	dbus_bus_add_match(conn, match, &error);
	show_debug(__FILE__,__FUNCTION__,__LINE__, "added match :[%s]", match);
    	if (dbus_error_is_set(&error)) {
      		show_error("dbus_bus_add_match() %s failed: %s\n",   member, error.message);
      		dbus_error_free(&error);
	}
}

/******************************************************************************/
/* External interfaces : */
/******************************************************************************/
Bool asdbus_init ()
{																/* return connection unix fd */
	char *tmp;
#ifdef ASDBUS_DISPATCH
	if (!ASDBus.dispatches)
		ASDBus.dispatches = create_asbidirlist(asdbus_dispatch_destroy);
#endif
	if (!ASDBus.session_conn) {
		ASDBus.session_conn = _asdbus_get_session_connection();
		if (!dbus_connection_set_watch_functions(ASDBus.session_conn, add_watch, remove_watch,  toggle_watch, ASDBus.session_conn, NULL)) {
		 	show_error("dbus_connection_set_watch_functions() failed");
		}
		_asdbus_add_match (ASDBus.session_conn,  SESSIONMANAGER_INTERFACE, NULL);
		//_asdbus_add_match (ASDBus.session_conn,  IFACE_SESSION_PRIVATE, "QueryEndSession");
		//_asdbus_add_match (ASDBus.session_conn,  IFACE_SESSION_PRIVATE, "EndSession");
		//_asdbus_add_match (ASDBus.session_conn,  IFACE_SESSION_PRIVATE, "Stop");
		dbus_connection_set_timeout_functions(ASDBus.session_conn, add_timeout, remove_timeout, toggle_timeout, NULL, NULL);
#ifdef ASDBUS_DISPATCH
		dbus_connection_set_dispatch_status_function(ASDBus.session_conn, queue_dispatch, NULL, NULL);
		queue_dispatch(ASDBus.session_conn, dbus_connection_get_dispatch_status(ASDBus.session_conn), NULL);
#endif
	}

	if (!ASDBus.system_conn){
		ASDBus.system_conn = _asdbus_get_system_connection();
		/*if (!dbus_connection_set_watch_functions(ASDBus.system_conn, add_watch, remove_watch,  toggle_watch, ASDBus.system_conn, NULL)) {
		 	show_error("dbus_connection_set_watch_functions() failed");
		}*/
	}

	/*if (ASDBus.session_conn && ASDBus.watchFds == NULL){
		//dbus_connection_get_unix_fd (ASDBus.session_conn, &(ASDBus.watch_fd));
		//dbus_whatch_get_unix_fd (ASDBus.session_conn, &(ASDBus.watch_fd));
	}*/

	if ((tmp = getenv ("KDE_SESSION_VERSION")) != NULL)
		ASDBus.kdeSessionVersion = atoi(tmp);

	return (ASDBus.session_conn != NULL);
}

ASVector* asdbus_getFds() {
	return ASDBus.watchFds;
}

void asdbus_handleDispatches (){
#ifdef ASDBUS_DISPATCH
	void *data;
	while ((data = extract_first_bidirelem (ASDBus.dispatches)) != NULL){
		ASDBusDispatch *d = (ASDBusDispatch*)data;
		while (dbus_connection_get_dispatch_status(d->data) == DBUS_DISPATCH_DATA_REMAINS){
			dbus_connection_dispatch(d->data);
			show_debug(__FILE__,__FUNCTION__,__LINE__,"dispatching dbus  data=%p\n", d->data);
		}
		free (d);
	}
#endif
}

void asdbus_shutdown ()
{
	if (ASDBus.session_conn)
		dbus_bus_release_name (ASDBus.session_conn,
													 AFTERSTEP_DBUS_SERVICE_NAME, NULL);

	if (ASDBus.session_conn || ASDBus.system_conn)
		dbus_shutdown ();
}

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
Bool get_gnome_autosave ()
{
	Bool autosave = False;
#ifdef HAVE_GIOLIB
	static Bool g_types_inited = False;
	if (!g_types_inited) {
#ifndef GLIB_VERSION_2_36
		g_type_init ();
#endif
		g_types_inited = True;
	}
	if (ASDBus.gnomeSessionPath) {
#if defined(HAVE_GSETTINGS)
		GSettings *gsm_settings = g_settings_new (GSM_MANAGER_SCHEMA);
		if (gsm_settings) {
			autosave = g_settings_get_boolean (gsm_settings, KEY_AUTOSAVE);
  		g_object_unref (gsm_settings);
		} else
			show_error (" Failed to get gnome-session Autosave settings");
#endif
	}
#endif
	return autosave;
}

/******************************************************************************/
void asdbus_EndSessionOk ();


void asdbus_process_messages (ASDBusFd* fd)
{
	//show_progress ("checking dbus messages for fd = %d", fd->fd);
#ifndef ASDBUS_DISPATCH
	while (ASDBus.session_conn) {
		DBusMessage *msg;
		const char *interface, *member;
		/* non blocking read of the next available message */
		dbus_connection_read_write (ASDBus.session_conn, 0);
		msg = dbus_connection_pop_message (ASDBus.session_conn);

		if (NULL == msg) {
			/* show_progress ("no more Dbus messages..."); */
			//show_progress("time(%ld):dbus message not received during the timeout - sleeping...", time (NULL));
			return;
		}
		interface = dbus_message_get_interface (msg);
		member = dbus_message_get_member (msg);
		show_debug(__FILE__,__FUNCTION__, __LINE__, "dbus msg iface = \"%s\", member = \"%s\"", interface?interface:"(nil)", member?member:"(nil)");
		if (interface == NULL || member == NULL) {
			show_progress ("time(%ld):dbus message cannot be parsed...",
										 time (NULL));
		} else {
			show_progress
					("time(%ld):dbus message received from \"%s\", member \"%s\"",
					 time (NULL), interface, member);
			if (strcmp (interface, IFACE_SESSION_PRIVATE) == 0) {
				if (strcmp (member, "QueryEndSession") == 0) {	/* must replay yes  within 10 seconds */
					asdbus_EndSessionOk ();
					asdbus_Notify ("Session is ending ...",
												 "Checking if it is safe to logout", 0);
					SaveSession (True);
				} else if (strcmp (member, "EndSession") == 0) {
					/*cover_desktop ();
					   display_progress (True, "Session is ending, please wait ..."); */
					asdbus_Notify ("Session is ending ...",
												 "Closing apps, please wait.", 0);
					dbus_connection_read_write (ASDBus.session_conn, 0);
					/* Yield to let other clients process Session Management requests */
					sleep_a_millisec (300);
					CloseSessionClients (False);
					/* we want to end to the very end */
				} else if (strcmp (member, "Stop") == 0) {
					asdbus_Notify ("Session is over.", "Bye-bye!", 0);
					dbus_connection_read_write (ASDBus.session_conn, 0);
					Done (False, NULL);
				}
			}
		}
#if 0
		if (dbus_message_is_method_call (msg, "test.method.Type", "Method"))
			reply_to_method_call (msg, conn);
#endif
		dbus_message_unref (msg);
	}
#else
	if (ASDBus.session_conn)
		do {
			dbus_connection_read_write_dispatch (ASDBus.session_conn, 0);
		} while (dbus_connection_get_dispatch_status (ASDBus.session_conn) ==
						 DBUS_DISPATCH_DATA_REMAINS);
#endif
}



/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
#else

int asdbus_init ()
{
	return -1;
}

void asdbus_shutdown ()
{
}

void asdbus_process_messages ()
{
};

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
#endif

/*****************************************************************************/
/* Gnome Session Manager's dbus Protocol :

1) send messages to org.gnome.SessionManager :

Setenv ("IMAGE_PATH", image_path)
Setenv ("FONT_PATH", font_path)

RegisterClient ( app_id, client_startup_id, object_path (returned))

2) If Quit/Quit is selected, send message :
Logout (0);

3) If Quit/Shutdown is selected :
Shutdown ();

4) What to do if Quit/Restart? send UnregisterClient() ?

Dbus calls :

dbus_message_new_method_call ("org.gnome.SessionManager",
							  "/org/gnome/SessionManager",
							  "org.gnome.SessionManager",
							  name_of_the_message );

use dbus_message_append_args (msg, type1, arg1, ..., DBUS_TYPE_INVALID) if needed.
use dbus_message_set_no_replay (msg, NULL) if no return value expected.

use dbus__connection_send () to send it.

*/

static inline Bool set_sm_client_id (DBusMessageIter * iter,
																		 const char *sm_client_id)
{
	if (sm_client_id == NULL || sm_client_id[0] == '\0')
		return False;
	show_progress
			("Using \"%s\" as the client ID for registration with the Session Manager",
			 sm_client_id);
	dbus_message_iter_append_basic (iter, DBUS_TYPE_STRING, &sm_client_id);
	return True;
}

char *asdbus_RegisterSMClient (const char *sm_client_id)
{
	char *client_path = NULL;

#ifdef HAVE_DBUS_CONTEXT
	if (ASDBus.session_conn) {
		DBusMessage *message =
				dbus_message_new_method_call (SESSIONMANAGER_NAME,
																			SESSIONMANAGER_PATH,
																			SESSIONMANAGER_INTERFACE,
																			"RegisterClient");
		if (message) {
			DBusMessage *replay;
			DBusError error;
			char *app_id = AFTERSTEP_APP_ID;
			DBusMessageIter iter;

			dbus_message_iter_init_append (message, &iter);
			dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &app_id);

			if (!set_sm_client_id (&iter, sm_client_id)) {
				if (!set_sm_client_id (&iter, getenv (SESSION_ID_ENVVAR))) {
					char *instance_id =
							safemalloc (sizeof (AFTERSTEP_APP_ID) + 1 + 32);
					sprintf (instance_id, "%s-%d", AFTERSTEP_APP_ID, getpid ());
					set_sm_client_id (&iter, instance_id);
					free (instance_id);
				}
			}

			dbus_error_init (&error);
			replay =
					dbus_connection_send_with_reply_and_block (ASDBus.session_conn,
																										 message, 20000, /* startup is busy time - better give gnome-session time to reply */
																										 &error);
			dbus_message_unref (message);

			if (!replay)
				show_error
						("Failed to register as a client with the Gnome Session Manager. DBus error: %s",
						 dbus_error_is_set (&error) ? error.message : "unknown error");
			else {										/* get the allocated session ClientID */
				char *ret_client_path;
				if (!dbus_message_get_args
						(replay, &error, DBUS_TYPE_OBJECT_PATH, &ret_client_path,
						 DBUS_TYPE_INVALID))
					show_error
							("Malformed registration replay from Gnome Session Manager. DBus error: %s",
							 dbus_error_is_set (&error) ? error.
							 message : "unknown error");
				else {
					client_path = mystrdup (ret_client_path);
					ASDBus.gnomeSessionPath = mystrdup (ret_client_path);
				}

				if (dbus_error_is_set (&error))
					dbus_error_free (&error);
				dbus_message_unref (replay);
			}
		}
	}
#endif
	return client_path;
}

void asdbus_UnregisterSMClient (const char *sm_client_path)
{
#ifdef HAVE_DBUS_CONTEXT
	if (ASDBus.session_conn && sm_client_path) {
		DBusMessage *message =
				dbus_message_new_method_call (SESSIONMANAGER_NAME,
																			SESSIONMANAGER_PATH,
																			SESSIONMANAGER_INTERFACE,
																			"UnregisterClient");
		if (message) {
			DBusMessage *replay;
			DBusError error;
			DBusMessageIter iter;

			dbus_message_iter_init_append (message, &iter);
			dbus_message_iter_append_basic (&iter, DBUS_TYPE_OBJECT_PATH,
																			&sm_client_path);
			dbus_error_init (&error);
			replay =
					dbus_connection_send_with_reply_and_block (ASDBus.session_conn,
																										 message, 200, &error);
			dbus_message_unref (message);

			if (!replay)
				show_error
						("Failed to unregister as a client with the Gnome Session Manager. DBus error: %s",
						 dbus_error_is_set (&error) ? error.message : "unknown error");
			else {										/* nothing is returned in replay to this message */
				show_progress ("Unregistered from Gnome Session Manager");
				dbus_message_unref (replay);
			}
			if (dbus_error_is_set (&error))
				dbus_error_free (&error);
		}
	}
#endif
}

void asdbus_EndSessionOk ()
{
		show_debug(__FILE__, __FUNCTION__, __LINE__, "dbus EndSessionOk");
#ifdef HAVE_DBUS_CONTEXT
	if (ASDBus.session_conn) {
		DBusMessage *message =
				dbus_message_new_method_call (SESSIONMANAGER_NAME,
																			ASDBus.gnomeSessionPath,	/*"/org/gnome/SessionManager", */
																			IFACE_SESSION_PRIVATE,
																			"EndSessionResponse");
		show_debug(__FILE__, __FUNCTION__, __LINE__, "dbus EndSessionResponse to iface = \"%s\", path = \"%s\", manager = \"%s\"", 
			    IFACE_SESSION_PRIVATE, ASDBus.gnomeSessionPath, SESSIONMANAGER_NAME);
		if (message) {
			DBusMessageIter iter;
			char *reason = "";
			dbus_bool_t ok = 1;
			dbus_uint32_t msg_serial;

			dbus_message_iter_init_append (message, &iter);
			dbus_message_iter_append_basic (&iter, DBUS_TYPE_BOOLEAN, &ok);
			dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &reason);
			dbus_message_set_no_reply (message, TRUE);
			if (!dbus_connection_send
					(ASDBus.session_conn, message, &msg_serial))
				show_error ("Failed to send EndSession indicator.");
			else
				show_progress ("Sent Ok to end session (time, %ld).", time (NULL));
			dbus_message_unref (message);
		}
	}
#endif
}


void *asdbus_SendSimpleCommandSync (ASDBusOjectDescr *descr, const char *command, int timeout)
{
	void *reply = NULL;

#ifdef HAVE_DBUS_CONTEXT
	DBusConnection *conn = descr->systemBus ? ASDBus.system_conn : ASDBus.session_conn;
	if (conn) {
		DBusMessage *message =
				dbus_message_new_method_call (descr->name,
																			descr->path,
																			descr->interface,
                         						  command);
		if (message) {
			DBusError error;

			dbus_error_init (&error);
			reply = dbus_connection_send_with_reply_and_block (conn, message, timeout, &error);
			dbus_message_unref (message);

			if (!reply) {
					show_error ("Request %s to %s failed: %s", command, descr->displayName,
				  						 dbus_error_is_set (&error) ? error.message : "unknown error");
			}
			if (dbus_error_is_set (&error))
				dbus_error_free (&error);
		}
	}
#endif
	return reply;
}

Bool asdbus_SendSimpleCommandSyncNoRep (ASDBusOjectDescr *descr, const char *command, int timeout)
{
  Bool res = False;
#ifdef HAVE_DBUS_CONTEXT
	DBusMessage *reply = asdbus_SendSimpleCommandSync (descr, command, -1);
	if (reply)
	{
		res = True;
    dbus_message_unref (reply);
	}
#endif
	return res;
}

Bool asdbus_GetIndicator (ASDBusOjectDescr *descr, const char *command)
{
  Bool res = False;
#ifdef HAVE_DBUS_CONTEXT
	DBusMessage *reply = asdbus_SendSimpleCommandSync (descr, command, -1);
	if (reply)
	{
		DBusMessageIter iter;
    dbus_bool_t ok = 0;

    dbus_message_iter_init (reply, &iter);
    dbus_message_iter_get_basic (&iter, &ok);
		res = ok;
    dbus_message_unref (reply);
	}
#endif
	return res;
}


static Bool asdbus_LogoutKDE (KDE_ShutdownConfirm confirm, KDE_ShutdownMode mode, KDE_ShutdownType type, int timeout)
{
	Bool requested = False;
#ifdef HAVE_DBUS_CONTEXT
	if (ASDBus.session_conn) {
		DBusMessage *message =
				dbus_message_new_method_call (KSMSERVER_NAME,
																			KSMSERVER_PATH,
																			KSMSERVER_INTERFACE,
																			"logout");
		if (message) {
			DBusMessageIter iter;
			dbus_uint32_t msg_serial;
			dbus_int32_t i32_confirm = confirm;
			dbus_int32_t i32_mode = mode;
			dbus_int32_t i32_type = type;

			dbus_message_iter_init_append (message, &iter);
			dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32, &i32_confirm);
			dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32, &i32_mode);
			dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32, &i32_type);

			dbus_message_set_no_reply (message, TRUE);
			if (!dbus_connection_send	(ASDBus.session_conn, message, &msg_serial))
				show_error ("Failed to send Logout request to KDE.");
			else {
				requested = True;
				show_progress ("Sent Ok to end session (time, %ld).", time (NULL));
			}
			dbus_message_unref (message);
		}
	}
#endif
	return requested;
}

static Bool asdbus_LogoutGNOME (int mode, int timeout)
{
	Bool requested = False;
#ifdef HAVE_DBUS_CONTEXT
	if (ASDBus.session_conn) {
		DBusMessage *message =
				dbus_message_new_method_call (SESSIONMANAGER_NAME,
																			SESSIONMANAGER_PATH,
																			SESSIONMANAGER_INTERFACE,
																			"Logout");
		if (message) {
			DBusMessage *replay;
			DBusError error;
			DBusMessageIter iter;
			dbus_uint32_t ui32_mode = mode;

			dbus_message_iter_init_append (message, &iter);
			dbus_message_iter_append_basic (&iter, DBUS_TYPE_UINT32, &ui32_mode);
			dbus_error_init (&error);
			replay =
					dbus_connection_send_with_reply_and_block (ASDBus.session_conn,
																										 message, timeout,
																										 &error);
			dbus_message_unref (message);

			if (!replay) {
				show_error
						("Failed to request Logout with the Gnome Session Manager. DBus error: %s",
						 dbus_error_is_set (&error) ? error.message : "unknown error");
			} else {										/* nothing is returned in replay to this message */
				dbus_message_unref (replay);
				requested = True;
			}
			if (dbus_error_is_set (&error))
				dbus_error_free (&error);
		}
	}
#endif
	return requested;
}

Bool asdbus_Logout (ASDbusLogoutMode mode, int timeout)
{
	Bool requested = False;
#ifdef HAVE_DBUS_CONTEXT
	if (ASDBus.session_conn) {
		if (ASDBus.gnomeSessionPath != NULL)
			requested = asdbus_LogoutGNOME (mode, timeout);
		else if (ASDBus.kdeSessionVersion >= 4) {
			KDE_ShutdownConfirm kdeConfirm = KDE_ShutdownConfirmDefault;
			KDE_ShutdownMode kdeMode = KDE_ShutdownModeDefault;
			if (mode == ASDBUS_Logout_Confirmed) {
				kdeConfirm = KDE_ShutdownConfirmNo;
				kdeMode = KDE_ShutdownModeTryNow;
			} else if (mode == ASDBUS_Logout_Force) {
				kdeConfirm = KDE_ShutdownConfirmNo;
				kdeMode = KDE_ShutdownModeForceNow;
			}
			requested = asdbus_LogoutKDE (kdeConfirm, kdeMode, KDE_ShutdownTypeNone, timeout);
		}
	}
#endif
	return requested;
}

Bool asdbus_GetCanLogout ()
{
	return (ASDBus.kdeSessionVersion >= 4 || ASDBus.gnomeSessionPath != NULL);
}

Bool asdbus_Shutdown (int timeout)
{
	Bool requested = False;
#ifdef HAVE_DBUS_CONTEXT
	if (ASDBus.session_conn) {
		if (ASDBus.gnomeSessionPath != NULL)
			requested = asdbus_SendSimpleCommandSyncNoRep (&dbusSessionManager, "Shutdown", timeout);
		else if (ASDBus.kdeSessionVersion >= 4)
			requested = asdbus_LogoutKDE (KDE_ShutdownConfirmDefault, KDE_ShutdownModeDefault, KDE_ShutdownTypeHalt, timeout);
	}
#endif
	return requested;
}

Bool asdbus_GetCanShutdown ()
{
	return (ASDBus.kdeSessionVersion >= 4
	        || (ASDBus.gnomeSessionPath != NULL && asdbus_GetIndicator (&dbusSessionManager, "CanShutdown")));
}

Bool asdbus_Suspend (int timeout)
{
	return asdbus_SendSimpleCommandSyncNoRep (&dbusUPower, "Suspend", timeout);
}

Bool asdbus_GetCanSuspend ()
{
	return asdbus_GetIndicator (&dbusUPower, "SuspendAllowed");
}

Bool asdbus_Hibernate (int timeout)
{
	return asdbus_SendSimpleCommandSyncNoRep (&dbusUPower, "Hibernate", timeout);
}

Bool asdbus_GetCanHibernate ()
{
	return asdbus_GetIndicator (&dbusUPower, "HibernateAllowed");
}

/*******************************************************************************
 * Freedesktop Console Kit
 *******************************************************************************/
char* asdbus_GetConsoleSessionId ()
{
  char     *session_id = NULL;
#ifdef HAVE_DBUS_CONTEXT
	if (ASDBus.system_conn) {
		DBusMessage *message = dbus_message_new_method_call (CK_NAME,
                        						                     CK_MANAGER_PATH,
                                      						       CK_MANAGER_INTERFACE,
                                            						 "GetCurrentSession");
    if (message) {
			DBusMessage *reply;
			DBusError error;
			DBusMessageIter iter;
      const char     *val = NULL;

  	  dbus_error_init (&error);
      reply = dbus_connection_send_with_reply_and_block (ASDBus.system_conn, message, -1, &error);
			dbus_message_unref (message);

      if (reply == NULL) {
				if (dbus_error_is_set (&error))
					show_error ("Unable to determine Console Kit Session: %s", error.message);
			} else {
        dbus_message_iter_init (reply, &iter);
        dbus_message_iter_get_basic (&iter, &val);
				session_id = mystrdup (val);
        dbus_message_unref (reply);
			}
			if (dbus_error_is_set (&error))
				dbus_error_free (&error);
		}
	}
#endif
	return session_id;
}

char* asdbus_GetConsoleSessionType (const char *session_id)
{
  char     *session_type = NULL;
#ifdef HAVE_DBUS_CONTEXT
	if (ASDBus.system_conn && session_id) {
		DBusMessage *message = dbus_message_new_method_call (CK_NAME,
																													session_id,
																													CK_SESSION_INTERFACE,
																													"GetSessionType");
    if (message) {
			DBusMessage *reply;
			DBusError error;

			dbus_error_init (&error);
			reply = dbus_connection_send_with_reply_and_block (ASDBus.system_conn, message, -1, &error);
			dbus_message_unref (message);

      if (reply == NULL) {
				if (dbus_error_is_set (&error))
					show_error ("Unable to determine Console Kit Session Type: %s", error.message);
			} else {
				DBusMessageIter iter;
  	    const char     *val = NULL;
        dbus_message_iter_init (reply, &iter);
        dbus_message_iter_get_basic (&iter, &val);
				session_type = mystrdup (val);
/*				show_progress ("sess_type returned = \"%s\", arg_type = \"%c\"", val, dbus_message_iter_get_arg_type (&iter)); */
        dbus_message_unref (reply);
			}
			if (dbus_error_is_set (&error))
				dbus_error_free (&error);
		}
	}
#endif
	return session_type;
}
/*******************************************************************************
 * Notifications
 *******************************************************************************/
void asdbus_Notify (const char *summary, const char *body, int timeout)
{
#ifdef HAVE_DBUS_CONTEXT
	if (ASDBus.session_conn) {
		DBusMessage *message = dbus_message_new_method_call  ("org.freedesktop.Notifications",
																													"/org/freedesktop/Notifications",
																													"org.freedesktop.Notifications",
																													"Notify");
		if (message) {
			const char *app_id = "AfterStep Window Manager";
			dbus_uint32_t replace_id = 0;
			const char *icon = "";
			DBusMessageIter iter;

			dbus_message_iter_init_append (message, &iter);
			dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &app_id);
			dbus_message_iter_append_basic (&iter, DBUS_TYPE_UINT32,
																			&replace_id);
			dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &icon);
			dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &summary);
			dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &body);
			/* Must add the following even if we don't need it */
			{
				DBusMessageIter sub;
				dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY,
																					DBUS_TYPE_STRING_AS_STRING,
																					&sub);
				dbus_message_iter_close_container (&iter, &sub);
				dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY, "{sv}",
																					&sub);
				dbus_message_iter_close_container (&iter, &sub);
			}
			dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32, &timeout);

			{
				DBusMessage *replay;
				DBusError error;

				dbus_error_init (&error);
				replay = dbus_connection_send_with_reply_and_block (ASDBus.session_conn,
																											 message, 200,
																											 &error);
				dbus_message_unref (message);

				if (!replay) {
					show_error ("Failed to send the notification. DBus error: %s",
											dbus_error_is_set (&error) ? error.message : "unknown error");
				} else {									/* get the allocated session ClientID */
					dbus_uint32_t req_id;
					if (!dbus_message_get_args(replay, &error, DBUS_TYPE_UINT32, &req_id, DBUS_TYPE_INVALID)) {
						show_error ("Malformed Notification replay. DBus error: %s",
												dbus_error_is_set (&error) ? error.message : "unknown error");
					}
#ifdef TEST_AS_DBUS
					else {
						show_progress ("Notification Request_id = %d", req_id);
					}
#endif
					dbus_message_unref (replay);
				}
				if (dbus_error_is_set (&error))
					dbus_error_free (&error);
			}
		}
	}
#endif
}


/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

#ifdef TEST_AS_DBUS
int ASDBus_fd = -1;
char *GnomeSessionClientID = NULL;

void asdbus_GetClients ()
{
#ifdef HAVE_DBUS_CONTEXT
	if (ASDBus.session_conn) {
		DBusMessageIter args;
		DBusPendingCall *pending;
		int current_type;

		DBusMessage *msg =
				dbus_message_new_method_call ("org.gnome.SessionManager",
																			"/org/gnome/SessionManager",
																			"org.gnome.SessionManager",
																			"GetClients");
		if (msg == NULL) {
			show_error ("Message Null");
			return;
		}
		// send message and get a handle for a reply
		if (!dbus_connection_send_with_reply (ASDBus.session_conn, msg, &pending, -1)) {	// -1 is default timeout
			show_error ("Out Of Memory!");
			return;
		}
		if (NULL == pending) {
			show_error ("Pending Call Null");
			return;
		}
		dbus_connection_flush (ASDBus.session_conn);

		// free message
		dbus_message_unref (msg);

		// block until we receive a reply
		dbus_pending_call_block (pending);

		// get the reply message
		msg = dbus_pending_call_steal_reply (pending);
		if (NULL == msg) {
			show_error ("Reply Null");
			return;
		}
		// free the pending message handle
		dbus_pending_call_unref (pending);

		// read the parameters
		if (!dbus_message_iter_init (msg, &args))
			show_error ("Message has no arguments!");
		while ((current_type =
						dbus_message_iter_get_arg_type (&args)) != DBUS_TYPE_INVALID) {
			show_progress ("current_type: %c", current_type, DBUS_TYPE_INVALID);
			if (current_type == DBUS_TYPE_ARRAY) {
				DBusMessageIter item;
				int item_type;
				dbus_message_iter_recurse (&args, &item);
				while ((item_type =
								dbus_message_iter_get_arg_type (&item)) !=
							 DBUS_TYPE_INVALID) {
					show_progress ("item_type: %c", item_type);
					if (item_type == DBUS_TYPE_OBJECT_PATH) {
						char *str = NULL;
						dbus_message_iter_get_basic (&item, &str);
						show_progress ("\t value = \"%s\"", str);
					}
					dbus_message_iter_next (&item);
				}
			} else if (current_type == DBUS_TYPE_STRING) {
				char *str = NULL;
				dbus_message_iter_get_basic (&args, &str);
				show_progress ("\t value = \"%s\"", str);
			}
			dbus_message_iter_next (&args);
		}

		// free reply and close connection
		dbus_message_unref (msg);
	}
#endif
}

void Done (Bool restart, char *cmd)
{
	exit (1);
}
void SaveSession (Bool force)
{
}

void CloseSessionClients (Bool only_modules)
{}

int main (int argc, char **argv)
{
	int logout_mode = -1;
	Bool shutdown_mode = False;
	char *console_session_id, *console_session_type;

	if (argc > 1) {
		if (strcmp (argv[1], "--logout") == 0) {
			if (argc > 2)
				logout_mode = atoi (argv[2]);
			else
				logout_mode = 0;
		}else if (strcmp (argv[1], "--shutdown") == 0)
			shutdown_mode = True;
	}
	InitMyApp ("test_asdbus", argc, argv, NULL, NULL, 0);

	ASDBus_fd = asdbus_init ();
	if (ASDBus_fd < 0) {
		show_error ("Failed to accure Session DBus connection.");
		return 0;
	}
	show_progress ("Successfuly accured Session DBus connection.");

	change_func_code ("Restart", F_NOP);
	if (!asdbus_GetCanShutdown())
		change_func_code ("SystemShutdown", F_NOP);

	console_session_id = asdbus_GetConsoleSessionId ();
	console_session_type = asdbus_GetConsoleSessionType (console_session_id);
	show_progress ("ConsoleKit session id is \"%s\", type = \"%s\"", console_session_id, console_session_type);
	show_progress ("CanLogout = %d", asdbus_GetCanLogout());
	show_progress ("CanShutdown = %d", asdbus_GetCanShutdown());
	show_progress ("CanSuspend = %d", asdbus_GetCanSuspend());
	show_progress ("CanHibernate = %d", asdbus_GetCanHibernate());

	asdbus_Notify ("TestNotification Summary", "Test notification body",
								 3000);

	GnomeSessionClientID = asdbus_RegisterSMClient (NULL);

	show_progress ("gnome-session Autosave set to %d", 	get_gnome_autosave ());
	if (GnomeSessionClientID != NULL)
		show_progress
				("Successfuly registered with GNOME Session Manager with Client Path \"%s\".",
				 GnomeSessionClientID);

	if (GnomeSessionClientID != NULL) {
		asdbus_GetClients ();
		asdbus_UnregisterSMClient (GnomeSessionClientID);
	}

	if (logout_mode >= 0 && logout_mode <= 2)
		asdbus_Logout (logout_mode, 100000);
	else if (shutdown_mode)
		asdbus_Shutdown (100000);


	asdbus_shutdown ();

	return 1;
}
#endif
