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


#ifdef HAVE_DBUS1
# include "dbus/dbus.h"

#define AFTERSTEP_DBUS_SERVICE_NAME	            "org.afterstep.afterstep"
#define AFTERSTEP_DBUS_INTERFACE			    "org.afterstep.afterstep"
#define AFTERSTEP_DBUS_ROOT_PATH			    "/org/afterstep/afterstep"

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
	ASDBus.session_conn = dbus_bus_get (DBUS_BUS_SESSION, &error);
	if (dbus_error_is_set (&error))
	{
		show_error ("Failed to connect to Session DBus: %s", error.message);
		dbus_error_free (&error);
		return -1;
	}
	
	res = dbus_bus_request_name (ASDBus.session_conn,
				 AFTERSTEP_DBUS_SERVICE_NAME,
				 DBUS_NAME_FLAG_REPLACE_EXISTING |
				 DBUS_NAME_FLAG_ALLOW_REPLACEMENT,
				 &error);
	
	if (dbus_error_is_set (&error))
	{
		show_error ("Failed to request name from DBus: %s", error.message);
		dbus_error_free (&error);
		return -1;
	}
	
    if (res != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER)
    {
		show_error ("Failed to request name from DBus - not a primary owner.");
		return -1;
    }
	
	dbus_connection_set_exit_on_disconnect (ASDBus.session_conn, FALSE);
	dbus_connection_get_unix_fd (ASDBus.session_conn, &(ASDBus.watch_fd));

    dbus_connection_register_object_path (ASDBus.session_conn,
										  AFTERSTEP_DBUS_ROOT_PATH, 
										  &ASDBusMessagesVTable, 0);

	
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

#endif

