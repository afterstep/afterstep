/****************************************************************************
 *
 * Copyright (c) 2005 Sasha Vasko <sasha at aftercode.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.   See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 ****************************************************************************/

#define LOCAL_DEBUG
#include "../configure.h"

#include "../include/afterbase.h"
#include "../libAfterStep/asapp.h"


#include "asgtk.h"

#include <gdk/gdkx.h>

#include "../libAfterStep/module.h"
#include "../libAfterConf/afterconf.h"

void 
init_asgtkapp( int argc, char *argv[], const char *module_name, void (*custom_usage_func) (void), ASFlagType opt_mask)
{
	GdkDisplay *gdk_display ;
	int i ; 
	static char *deleted_arg = "_deleted_arg_" ;
  	
	InitMyApp (module_name, argc, argv, NULL, custom_usage_func, opt_mask );
	for( i = 1 ; i < argc ; ++i ) 
		if( argv[i] == NULL ) 
			argv[i] = strdup(deleted_arg) ;
  	LinkAfterStepConfig();
  	InitSession();

#ifdef ENABLE_NLS
  	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
  	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  	textdomain (GETTEXT_PACKAGE);
#endif

  	gtk_set_locale ();
  	gtk_init (&argc, &argv);

	gdk_display = gdk_display_get_default();
	
	LoadColorScheme();

	ConnectXDisplay (gdk_x11_display_get_xdisplay(gdk_display), NULL, False);
	ReloadASEnvironment( NULL, NULL, NULL, False, True );
	/*MyArgs.display_name*/
}
