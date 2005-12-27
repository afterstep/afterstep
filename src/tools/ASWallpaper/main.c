/*
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "../../../configure.h"
#include "../../../libAfterStep/asapp.h"
#include "../../../libAfterStep/module.h"
#include "../../../libAfterConf/afterconf.h"
#include "../../../libASGTK/asgtk.h"

#include "interface.h"


ASWallpaperState WallpaperState ;

int
main (int argc, char *argv[])
{
	
	init_asgtkapp( argc, argv, CLASS_ASWALLPAPER, NULL, 0);
	ConnectAfterStep(0,0);
//  add_pixmap_directory (DEFAULT_PIXMAP_DIR);

  /*
   * The following code was added by Glade to create one of each component
   * (except popup menus), just so that you see something after building
   * the project. Delete any components that you don't want shown initially.
   */
  init_ASWallpaper();

  gtk_main ();
  return 0;
}

