/****************************************************************************
 * Copyright (c) 2003 Sasha Vasko <sasha at aftercode.net>
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
 ****************************************************************************/

#define LOCAL_DEBUG

#include "../../configure.h"

#include "asinternals.h"

#include <limits.h>
#include <signal.h>

#include "../../libAfterStep/session.h"

/* Overview:
 * Theme could be represented by 3 different things :
 * 1) single installation script ( combines standard looks and feels and image files )
 * 2) theme tarball ( could be gzipped or bzipped ) that contains :
 *		look file, feel file, theme install script, theme file with config for modules
 *		It could also include TTF fonts and images.
 * 3) theme tarball ( old style ) without installation script :
 * 	    contains everything above but installation script.
 *
 * While installing themes we do several things :
 * 1) copy feel and look files into  afterstep/looks afterstep/feels directories
 * 2) copy all background files into afterstep/backgrounds
 * 3) copy remaining icon files into afterstep/desktop/icons
 * 4) copy remaining TTF  files into afterstep/desktop/fonts
 * 5) copy theme file into non-configurable/theme
 * 6) If ordered so in installation script we change look and feel to theme look and feel
 * 7) If ordered so in installation script we change backgrounds for different desktops
 * 8) finally we restart AS, but that is different story.
 *
 */
Bool
install_theme_file( const char *src, const char *dst )
{

	return False;
}

