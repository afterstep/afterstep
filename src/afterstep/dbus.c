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

#ifdef HAVE_DBUS1
# include "dbus/dbus.h"

#ifndef TEST_AS_DBUS
#define AFTERSTEP_APP_ID			            "afterstep"
#else
#define AFTERSTEP_APP_ID			            "afterstep-test"
#endif

#define AFTERSTEP_DBUS_SERVICE_NAME	      "org.afterstep." AFTERSTEP_APP_ID
#define AFTERSTEP_DBUS_INTERFACE			    "org.afterstep." AFTERSTEP_APP_ID
#define AFTERSTEP_DBUS_ROOT_PATH			    "/org/afterstep/" AFTERSTEP_APP_ID

#define IFACE_SESSION_PRIVATE 						"org.gnome.SessionManager.ClientPrivate"

#define HAVE_DBUS_CONTEXT 1

typedef struct ASDBusContext
{
	DBusConnection *session_conn;
	int watch_fd;
	char *client_session_path;
}ASDBusContext;

static ASDBusContext ASDBus = {NULL, -1, NULL};

static DBusHandlerResult asdbus_handle_message (DBusConnection *, DBusMessage *, void *);

static DBusObjectPathVTable ASDBusMessagesVTable = 
{
	NULL, asdbus_handle_message, /* handler function */
	NULL, NULL, NULL, NULL
};

/******************************************************************************/
/* internal stuff */
/******************************************************************************/
DBusHandlerResult 
asdbus_handle_message (DBusConnection *conn, DBusMessage *msg, void *data)
{
	Bool handled = False;
	
	show_progress ("Dbus message received from \"%s\", member \"%s\"", dbus_message_get_interface (msg), dbus_message_get_member (msg));
	
	if (dbus_message_is_signal(msg, "org.gnome.SessionManager", "SessionOver")) {
      dbus_message_unref(msg);	
			handled = True;
			Done (False, NULL);
	}	
	
	return handled ? DBUS_HANDLER_RESULT_HANDLED : DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

}

/******************************************************************************/
/* External interfaces : */
/******************************************************************************/
int asdbus_init() /* return connection unix fd */
{
	DBusError error;
	int res;

	dbus_error_init (&error);

	if (!ASDBus.session_conn)
	{
		DBusConnection *session_conn = dbus_bus_get (DBUS_BUS_SESSION, &error);
		
		if (dbus_error_is_set (&error))
			show_error ("Failed to connect to Session DBus: %s", error.message);
		else
		{
			res = dbus_bus_request_name (session_conn,
										 AFTERSTEP_DBUS_SERVICE_NAME,
										 DBUS_NAME_FLAG_REPLACE_EXISTING |
										 DBUS_NAME_FLAG_ALLOW_REPLACEMENT,
										 &error);
			if (dbus_error_is_set (&error))
			{
				show_error ("Failed to request name from DBus: %s", error.message);
			}else if (res != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER)
			{
				show_error ("Failed to request name from DBus - not a primary owner.");
			}else
			{
				dbus_connection_set_exit_on_disconnect (session_conn, FALSE);
				dbus_connection_register_object_path (session_conn,
													  AFTERSTEP_DBUS_ROOT_PATH, 
													  &ASDBusMessagesVTable, 0);
				ASDBus.session_conn = session_conn;
			}
		}
	}
	
	if (ASDBus.session_conn && ASDBus.watch_fd < 0)
		dbus_connection_get_unix_fd (ASDBus.session_conn, &(ASDBus.watch_fd));
	
	dbus_error_free (&error);
	
	return ASDBus.watch_fd;
}

void asdbus_shutdown()
{
	if (ASDBus.session_conn)
	{
		dbus_bus_release_name (ASDBus.session_conn, AFTERSTEP_DBUS_SERVICE_NAME, NULL);
		dbus_shutdown ();
	}
}
/******************************************************************************/
void asdbus_EndSessionOk();


void
asdbus_process_messages ()
{
/*	show_progress ("checking Dbus messages"); */
#if 1	
	while (ASDBus.session_conn) {
    DBusMessage *msg;
		const char *interface, *member;
		/* non blocking read of the next available message */
		dbus_connection_read_write(ASDBus.session_conn, 500);
    msg = dbus_connection_pop_message(ASDBus.session_conn);

    if (NULL == msg) { 
			/* show_progress ("no more Dbus messages..."); */
			show_progress ("time(%ld):Dbus message not received during the timeout - sleeping...", time(NULL));
      return; 
    }
		interface = dbus_message_get_interface (msg);
		member = dbus_message_get_member (msg);
		
		if (interface == NULL || member == NULL) {
			show_progress ("time(%ld):Dbus message cannot be parsed...", time(NULL));
		} else {
			show_progress ("time(%ld):Dbus message received from \"%s\", member \"%s\"", time(NULL), interface, member);
			if (strcmp (interface, IFACE_SESSION_PRIVATE) == 0) {
				if (strcmp (member, "QueryEndSession") == 0) { /* must replay yes  within 10 seconds */
					asdbus_EndSessionOk();
				}else if (strcmp (member, "EndSession") == 0 || strcmp (member, "Stop") == 0) { /* must replay yes  within 10 seconds */
					Done (False, NULL);
				}
			}
		}
#if 0		
    if (dbus_message_is_method_call(msg, "test.method.Type", "Method"))
         reply_to_method_call(msg, conn);
#endif
    dbus_message_unref(msg);
   }
#else
	if (ASDBus.session_conn)
		do
		{
			dbus_connection_read_write_dispatch (ASDBus.session_conn, 0);
		}while (dbus_connection_get_dispatch_status (ASDBus.session_conn) == DBUS_DISPATCH_DATA_REMAINS);
#endif
}



/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
#else

int asdbus_init(){return -1;}
void asdbus_shutdown(){}
void asdbus_process_messages (){};

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

static inline Bool set_sm_client_id (DBusMessageIter *iter, const char *sm_client_id)
{
	if (sm_client_id == NULL || sm_client_id[0] == '\0')
		return False;
	show_progress ("Using \"%s\" as the client ID for registration with the Session Manager", sm_client_id);
  dbus_message_iter_append_basic(iter,DBUS_TYPE_STRING,&sm_client_id);
	return True;
}

char* asdbus_RegisterSMClient(const char *sm_client_id)
{
	char *client_path = NULL;

#ifdef HAVE_DBUS_CONTEXT
	if (ASDBus.session_conn)
	{
		DBusMessage *message = dbus_message_new_method_call("org.gnome.SessionManager", 
							  "/org/gnome/SessionManager", 
							  "org.gnome.SessionManager", 
							  "RegisterClient" );
		if (message)
		{
			DBusMessage *replay;
			DBusError error;
			char *app_id = AFTERSTEP_APP_ID ;
			DBusMessageIter iter;
			
			dbus_message_iter_init_append(message,&iter);
      dbus_message_iter_append_basic(&iter,DBUS_TYPE_STRING,&app_id);

			if (!set_sm_client_id (&iter, sm_client_id)) {
				if (!set_sm_client_id (&iter, getenv(SESSION_ID_ENVVAR))) {
					char *instance_id = safemalloc (sizeof(AFTERSTEP_APP_ID)+1+32);
					sprintf (instance_id, "%s-%d", AFTERSTEP_APP_ID, getpid());
					set_sm_client_id (&iter, instance_id);
					free (instance_id);
				}
			}

			dbus_error_init (&error);
			replay = dbus_connection_send_with_reply_and_block (ASDBus.session_conn, message, 1000, &error); 
			dbus_message_unref (message);
	    
			if (!replay)
				show_error ("Failed to register as a client with the Gnome Session Manager. DBus error: %s", dbus_error_is_set (&error) ? error.message : "unknown error");
			else { /* get the allocated session ClientID */
				char *ret_client_path;
				if (!dbus_message_get_args (replay, &error, DBUS_TYPE_OBJECT_PATH, &ret_client_path, DBUS_TYPE_INVALID))
					show_error ("Malformed registration replay from Gnome Session Manager. DBus error: %s", dbus_error_is_set (&error) ? error.message : "unknown error");
				else {
					client_path = mystrdup (ret_client_path);
					ASDBus.client_session_path = mystrdup (ret_client_path);
				}

				dbus_message_unref (replay);
			}
		}
	}
#endif
	return client_path;
}

void asdbus_UnregisterSMClient(const char *sm_client_path)
{
#ifdef HAVE_DBUS_CONTEXT
	if (ASDBus.session_conn && sm_client_path)
	{
		DBusMessage *message = dbus_message_new_method_call("org.gnome.SessionManager", 
							  "/org/gnome/SessionManager", 
							  "org.gnome.SessionManager", 
							  "UnregisterClient" );
		if (message)
		{
			DBusMessage *replay;
			DBusError error;
			DBusMessageIter iter;
			
			dbus_message_iter_init_append(message,&iter);
      dbus_message_iter_append_basic(&iter,DBUS_TYPE_OBJECT_PATH,&sm_client_path);
			dbus_error_init (&error);
			replay = dbus_connection_send_with_reply_and_block (ASDBus.session_conn, message, 200, &error); 
			dbus_message_unref (message);
	    
			if (!replay)
				show_error ("Failed to unregister as a client with the Gnome Session Manager. DBus error: %s", dbus_error_is_set (&error) ? error.message : "unknown error");
			else {/* nothing is returned in replay to this message */
				show_progress ("Unregistered from Gnome Session Manager");
				dbus_message_unref (replay);
			}
		}
	}
#endif
}

Bool asdbus_Logout(int mode, int timeout)
{
	Bool requested = False;
#ifdef HAVE_DBUS_CONTEXT
	if (ASDBus.session_conn)
	{
		DBusMessage *message = dbus_message_new_method_call("org.gnome.SessionManager", 
							  "/org/gnome/SessionManager", 
							  "org.gnome.SessionManager", 
							  "Logout" );
		if (message)
		{
			DBusMessage *replay;
			DBusError error;
			DBusMessageIter iter;
			dbus_uint32_t ui32_mode = mode;
			
			dbus_message_iter_init_append(message,&iter);
      dbus_message_iter_append_basic(&iter,DBUS_TYPE_UINT32,&ui32_mode);
			dbus_error_init (&error);
			replay = dbus_connection_send_with_reply_and_block (ASDBus.session_conn, message, timeout, &error); 
			dbus_message_unref (message);
	    
			if (!replay)
				show_error ("Failed to request Logout with the Gnome Session Manager. DBus error: %s", dbus_error_is_set (&error) ? error.message : "unknown error");
			else {/* nothing is returned in replay to this message */
				dbus_message_unref (replay);
				requested = True;
			}
		}
	}
#endif
	return requested;
}

void asdbus_EndSessionOk()
{
#ifdef HAVE_DBUS_CONTEXT
	if (ASDBus.session_conn)
	{
		DBusMessage *message = dbus_message_new_method_call("org.gnome.SessionManager", 
							  ASDBus.client_session_path, /*"/org/gnome/SessionManager", */
							  IFACE_SESSION_PRIVATE, 
							  "EndSessionResponse" );
		if (message)
		{
			DBusMessageIter iter;
			char *reason = "";
			dbus_bool_t ok = 1;
			dbus_uint32_t msg_serial;
			
			dbus_message_iter_init_append(message,&iter);
      dbus_message_iter_append_basic(&iter,DBUS_TYPE_BOOLEAN,&ok);
      dbus_message_iter_append_basic(&iter,DBUS_TYPE_STRING,&reason);
			dbus_message_set_no_reply (message, TRUE);
			if (!dbus_connection_send (ASDBus.session_conn, message, &msg_serial)) 
				show_error ("Failed to send EndSession indicator.");
			else
				show_progress ("Sent Ok to end session (time, %ld).", time(NULL));
			dbus_message_unref (message);
		}
	}
#endif
}


void asdbus_Notify(const char *summary, const char *body, int timeout)
{
#ifdef HAVE_DBUS_CONTEXT
	if (ASDBus.session_conn)	{
		DBusMessage *message = dbus_message_new_method_call("org.freedesktop.Notifications", 
							  "/org/freedesktop/Notifications", 
							  "org.freedesktop.Notifications", 
							  "Notify" );
		if (message)		{
			const char *app_id = "AfterStep Window Manager" ;
			dbus_uint32_t replace_id = 0;
			const char *icon = "";
			DBusMessageIter iter;
			
			dbus_message_iter_init_append(message,&iter);
      dbus_message_iter_append_basic(&iter,DBUS_TYPE_STRING,&app_id);
      dbus_message_iter_append_basic(&iter,DBUS_TYPE_UINT32,&replace_id);
      dbus_message_iter_append_basic(&iter,DBUS_TYPE_STRING,&icon);
      dbus_message_iter_append_basic(&iter,DBUS_TYPE_STRING,&summary);
      dbus_message_iter_append_basic(&iter,DBUS_TYPE_STRING,&body);
			/* Must add the following even if we don't need it */
			{
				DBusMessageIter sub;
      	dbus_message_iter_open_container(&iter,DBUS_TYPE_ARRAY,DBUS_TYPE_STRING_AS_STRING,&sub);
      	dbus_message_iter_close_container(&iter,&sub);
      	dbus_message_iter_open_container(&iter,DBUS_TYPE_ARRAY,"{sv}",&sub);
      	dbus_message_iter_close_container(&iter,&sub);
			}
      dbus_message_iter_append_basic(&iter,DBUS_TYPE_INT32,&timeout);
			
			{
				DBusMessage *replay;
				DBusError error;

				dbus_error_init (&error);
				replay = dbus_connection_send_with_reply_and_block (ASDBus.session_conn, message, 200, &error); 
				dbus_message_unref (message);
	    
				if (!replay)
					show_error ("Failed to send the notification. DBus error: %s", dbus_error_is_set (&error) ? error.message : "unknown error");
				else { /* get the allocated session ClientID */
					dbus_uint32_t req_id;
					if (!dbus_message_get_args (replay, &error, DBUS_TYPE_UINT32, &req_id, DBUS_TYPE_INVALID))
						show_error ("Malformed Notification replay. DBus error: %s", dbus_error_is_set (&error) ? error.message : "unknown error");
#ifdef TEST_AS_DBUS
					else
						show_progress ( "Notification Request_id = %d", req_id);
#endif						
					dbus_message_unref (replay);
				}
			}
		}
	}
#endif
}


/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

#ifdef TEST_AS_DBUS
int 		  ASDBus_fd = -1;
char *GnomeSessionClientID = NULL;

void asdbus_GetClients()
{
#ifdef HAVE_DBUS_CONTEXT
	if (ASDBus.session_conn)	{
		DBusMessageIter args;
		DBusPendingCall* pending;
		int current_type;
	
		DBusMessage *msg = dbus_message_new_method_call("org.gnome.SessionManager", 
							  "/org/gnome/SessionManager", 
							  "org.gnome.SessionManager", 
							  "GetClients" );
		if (msg == NULL){ 
      show_error("Message Null");
      return;
    }

    // send message and get a handle for a reply
    if (!dbus_connection_send_with_reply (ASDBus.session_conn, msg, &pending, -1)) { // -1 is default timeout
    	show_error("Out Of Memory!"); 
      return;
    }
    if (NULL == pending) { 
    	show_error ("Pending Call Null"); 
      return; 
		}
    dbus_connection_flush(ASDBus.session_conn);

    // free message
    dbus_message_unref(msg);
		
		// block until we receive a reply
    dbus_pending_call_block(pending);
   
    // get the reply message
    msg = dbus_pending_call_steal_reply(pending);
    if (NULL == msg) {
    	show_error ("Reply Null"); 
      return; 
		}
    // free the pending message handle
    dbus_pending_call_unref(pending);

    // read the parameters
    if (!dbus_message_iter_init(msg, &args))
      show_error ("Message has no arguments!"); 
		while ((current_type = dbus_message_iter_get_arg_type (&args)) != DBUS_TYPE_INVALID) {
	    show_progress("current_type: %c", current_type, DBUS_TYPE_INVALID);
			if (current_type == DBUS_TYPE_ARRAY) {
				DBusMessageIter item;
				int item_type;
				dbus_message_iter_recurse (&args, &item);
				while ((item_type = dbus_message_iter_get_arg_type (&item)) != DBUS_TYPE_INVALID) {
	  		  show_progress("item_type: %c", item_type);
					if (item_type == DBUS_TYPE_OBJECT_PATH) {
						char *str = NULL;
						dbus_message_iter_get_basic (&item, &str);
						show_progress("\t value = \"%s\"", str);
					}
					dbus_message_iter_next (&item);
				}
			} else if (current_type == DBUS_TYPE_STRING) {
				char *str = NULL;
				dbus_message_iter_get_basic (&args, &str);
				show_progress("\t value = \"%s\"", str);
			}
			dbus_message_iter_next (&args);
		}
	
	   // free reply and close connection
  	 dbus_message_unref(msg);   
	}
#endif
}

void Done (Bool restart, char *cmd)
{
	exit (1);
}

void main (int argc, char ** argv)
{
	int logout_mode = -1;
	if (argc > 1 && strcmp(argv[1], "--logout") == 0) {
		if (argc > 2)
			logout_mode = atoi(argv[2]);
		else
			logout_mode = 0;
	}	
	InitMyApp( "test_asdbus", argc, argv, NULL, NULL, 0);

	ASDBus_fd = asdbus_init();
	if (ASDBus_fd<0)	{
		show_error ("Failed to accure Session DBus connection.");	
		return 0;
	}	

	show_progress ("Successfuly accured Session DBus connection.");	

	asdbus_Notify("TestNotification Summary", "Test notification body", 3000);
	
	GnomeSessionClientID = asdbus_RegisterSMClient(NULL);
	if (GnomeSessionClientID != NULL)
		show_progress ("Successfuly registered with GNOME Session Manager with Client Path \"%s\".", GnomeSessionClientID);	

	if (GnomeSessionClientID != NULL) {
		asdbus_GetClients();
		asdbus_UnregisterSMClient(GnomeSessionClientID);
	}

	if (logout_mode >= 0 && logout_mode <= 2)
		asdbus_Logout(logout_mode, 100000);
	
	asdbus_shutdown();

	return 1;	
}
#endif
