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

#define AFTERSTEP_DBUS_SERVICE_NAME	            "org.afterstep.afterstep"
#define AFTERSTEP_DBUS_INTERFACE			    "org.afterstep.afterstep"
#define AFTERSTEP_DBUS_ROOT_PATH			    "/org/afterstep/afterstep"

#define HAVE_DBUS_CONTEXT 1

typedef struct ASDBusContext
{
	DBusConnection *session_conn;
	int watch_fd;
}ASDBusContext;

static ASDBusContext ASDBus = {NULL, -1};

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
void
asdbus_process_messages ()
{
	if (ASDBus.session_conn)
		do
		{
			dbus_connection_read_write_dispatch (ASDBus.session_conn, 0);
		}while (dbus_connection_get_dispatch_status (ASDBus.session_conn) 
				== DBUS_DISPATCH_DATA_REMAINS);
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

void asdbus_RegisterSMClient(const char *sm_client_id)
{
#ifdef HAVE_DBUS_CONTEXT
	if (ASDBus.session_conn && sm_client_id)
	{
		DBusMessage *message = dbus_message_new_method_call("org.gnome.SessionManager", 
							  "/org/gnome/SessionManager", 
							  "org.gnome.SessionManager", 
							  "RegisterClient" );
		if (message)
		{
			char *app_id = "afterstep" ;
			char *client_id = NULL;
			dbus_uint32_t msg_serial;
			dbus_message_append_args(message,
									  DBUS_TYPE_STRING, &app_id,
									  DBUS_TYPE_STRING, &sm_client_id,
									  DBUS_TYPE_INVALID
									  // ,DBUS_TYPE_OBJECT_PATH, &client_id,DBUS_TYPE_INVALID
									  );
			dbus_message_set_no_reply (message, TRUE);
			
			dbus_connection_send (ASDBus.session_conn, message, &msg_serial);
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
			const char *actions[] = {"",""};
			const char *hints[] = {"",""};
			
			dbus_uint32_t msg_serial;
			dbus_message_append_args(message,
									  DBUS_TYPE_STRING, &app_id,
										DBUS_TYPE_UINT32, &replace_id, 
										DBUS_TYPE_STRING, &icon,
									  DBUS_TYPE_STRING, &summary,
									  DBUS_TYPE_STRING, &body,
									  DBUS_TYPE_INVALID
									  );
										
#if 0
dbus_message_append_args(msg,
                            DBUS_TYPE_STRING, &(n->app_name),
                            DBUS_TYPE_UINT32, &(n->replaces_id),
                            DBUS_TYPE_STRING, &(n->app_icon),
                            DBUS_TYPE_STRING, &(n->summary),
                            DBUS_TYPE_STRING, &(n->body),
                            DBUS_TYPE_INVALID
                            );

   dbus_message_iter_init_append(msg, &iter);
   if (dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "s", &sub))
     {
        if (n->actions)
          {
             E_Notification_Action *action;
             EINA_LIST_FOREACH (n->actions, l, action)
               {
                  dbus_message_iter_append_basic(&sub, DBUS_TYPE_STRING, &(action->id));
                  dbus_message_iter_append_basic(&sub, DBUS_TYPE_STRING, &(action->name));
               }
          }
        dbus_message_iter_close_container(&iter, &sub);
     }
   else
     {
        ERR("dbus_message_iter_open_container() failed");
     }

   /* hints */
   if (dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "{sv}", &sub))
     {
        if (n->hints.urgency) /* we only need to send this if its non-zero*/
          e_notify_marshal_dict_byte(&sub, "urgency", n->hints.urgency);
        if (n->hints.category)
          e_notify_marshal_dict_string(&sub, "category", n->hints.category);
        if (n->hints.desktop)
          e_notify_marshal_dict_string(&sub, "desktop_entry", n->hints.desktop);
        if (n->hints.image_data)
          e_notify_marshal_dict_variant(&sub, "image-data", "(iiibiiay)", E_DBUS_VARIANT_MARSHALLER(e_notify_marshal_hint_image), n->hints.image_data);
        if (n->hints.sound_file)
          e_notify_marshal_dict_string(&sub, "sound-file", n->hints.sound_file);
        if (n->hints.suppress_sound) /* we only need to send this if its true */
          e_notify_marshal_dict_byte(&sub, "suppress-sound", n->hints.suppress_sound);
        if (n->hints.x > -1 && n->hints.y > -1)
          {
             e_notify_marshal_dict_int(&sub, "x", n->hints.x);
             e_notify_marshal_dict_int(&sub, "y", n->hints.y);
          }
        dbus_message_iter_close_container(&iter, &sub);
     }
   else
     {
        ERR("dbus_message_iter_open_container() failed");
     }
   dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &(n->expire_timeout));
////////////////
// From google :
dbus_message_iter_init_append(msg,&iter);
        dbus_message_iter_append_basic(&iter,DBUS_TYPE_STRING,&application);
        dbus_message_iter_append_basic(&iter,DBUS_TYPE_UINT32,&msgid);
        dbus_message_iter_append_basic(&iter,DBUS_TYPE_STRING,&appicon);
        dbus_message_iter_append_basic(&iter,DBUS_TYPE_STRING,&summary);
        dbus_message_iter_append_basic(&iter,DBUS_TYPE_STRING,&body);
        dbus_message_iter_open_container(&iter,DBUS_TYPE_ARRAY,DBUS_TYPE_STRING_AS_STRING,&array);
        dbus_message_iter_close_container(&iter,&array);
        dbus_message_iter_open_container(&iter,DBUS_TYPE_ARRAY,"{sv}",&array);
        if (image && image->getData()) {
          const FXchar * icon_data="icon_data"; /// spec 0.9 says "image_data". some use "icon_data" though..

          idata     = (const FXchar*)image->getData();
          ialpha    = true;
          iw        = image->getWidth();
          ih        = image->getHeight();
          is        = iw*4;
          ibps      = 8;
          ichannels = 4;
          isize     = iw*ih*4;

          dbus_message_iter_open_container(&array,DBUS_TYPE_DICT_ENTRY,0,&dict);
            dbus_message_iter_append_basic(&dict,DBUS_TYPE_STRING,&icon_data);
            dbus_message_iter_open_container(&dict,DBUS_TYPE_VARIANT,"(iiibiiay)",&variant);
              dbus_message_iter_open_container(&variant,DBUS_TYPE_STRUCT,NULL,&value);
                dbus_message_iter_append_basic(&value,DBUS_TYPE_INT32,&iw);
                dbus_message_iter_append_basic(&value,DBUS_TYPE_INT32,&ih);
                dbus_message_iter_append_basic(&value,DBUS_TYPE_INT32,&is);
                dbus_message_iter_append_basic(&value,DBUS_TYPE_BOOLEAN,&ialpha);
                dbus_message_iter_append_basic(&value,DBUS_TYPE_INT32,&ibps);
                dbus_message_iter_append_basic(&value,DBUS_TYPE_INT32,&ichannels);
                dbus_message_iter_open_container(&value,DBUS_TYPE_ARRAY,DBUS_TYPE_BYTE_AS_STRING,&data);
                  dbus_message_iter_append_fixed_array(&data,DBUS_TYPE_BYTE,&idata,isize);
                dbus_message_iter_close_container(&value,&data);
              dbus_message_iter_close_container(&variant,&value);
            dbus_message_iter_close_container(&dict,&variant);
          dbus_message_iter_close_container(&array,&dict);
          }
        dbus_message_iter_close_container(&iter,&array);
        dbus_message_iter_append_basic(&iter,DBUS_TYPE_INT32,&timeout);

      send(msg,this,ID_NOTIFY_REPLY);

#endif 										
										
										
			dbus_message_set_no_reply (message, TRUE);
			
			dbus_connection_send (ASDBus.session_conn, message, &msg_serial);
			dbus_message_unref (message);
		}
	}
#endif
}


/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
