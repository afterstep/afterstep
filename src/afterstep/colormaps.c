/*
 * Copyright (C) 1994 Robert Nation
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
 */

#include "../../configure.h"

#include <stdio.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include "../../include/aftersteplib.h"
#include "../../include/afterstep.h"
#include "../../include/parse.h"
#include "../../include/misc.h"
#include "../../include/style.h"
#include "../../include/screen.h"
#include "../../include/module.h"

typedef enum {
    AS_CmapUninstalled = 0,
    AS_CmapPendingInstall,
    AS_CmapInstalled
}ASCmapState;

typedef enum {
    AS_CmapClient = 0,
    AS_CmapAfterStep,
    AS_CmapRoot
}ASCmapLevel;

typedef struct ASInstalledColormap
{
    Colormap    cmap;
    int         ref_count;
}ASInstalledColormap;

typedef struct ASRequiredColormap
{
    Colormap    desired;
    Colormap    installed;
    ASCmapState state;
}ASRequiredColormap;

static ASBiDirList  *InstalledCmapList = NULL;
static ASInstalledColormap *RootCmap = NULL ;
static ASInstalledColormap *AfterStepCmap = NULL ;
static ASRequiredColormap *RequiredCmapsList = NULL ;
static int MaxRequiredCmaps = 0;
static ASHashTable *Cmap2WindowXref = NULL ;



static Colormap      last_cmap = None;
static ASCmapState   last_cmap_state = AS_CmapNone;

void
destroy_ASInstalledColormap( void *data )
{
    if( data )
    {
        ASInstalledColormap *ic = (ASInstalledColormap*)data;
        XUninstallColormap( dpy, ic->cmap );
        free( data );
    }
}

static void
correct_colormaps_order()
{
    int i ;

    if( MaxRequiredCmaps == 0 )
    {
        /* we will maintain only list of required colormaps */
        MaxRequiredCmaps = MinCmapsOfScreen(ScreensOfDisplay(dpy,Scr.screen));
        if( MaxRequiredCmaps == 0 )
            return;
        RequiredCmaps = safecalloc( MaxRequiredCmaps, sizeof(ASRequiredColormap));
    }

    if( RequiredCmaps == NULL )
        return;

    i = 0 ;

    if( RootCmap && RootCmap->ref_count > 0 )
        RequiredCmaps[i++].desired = RootCmap->cmap ;
    if( i < MaxRequiredCmaps )
    {
        /* first lets check on semi-mandatory AfterStep colormap */
        if(   AfterStepCmap != NULL &&
            ( i < MaxRequiredCmaps-1 ||  /* either we are allowed several maps at the same time */ */
              AfterStepCmap->ref_count > 0))              /* or AfterStep cmap is required */
            RequiredCmaps[i++].desired = AfterStepCmap->cmap ;

        /* now lets proceed with installing client's colormaps : */
        if( InstalledCmapList )
        {
            ASBiDirElem *pcurr = LIST_START( InstalledCmapList );
            while( i < MaxRequiredCmaps && pcurr )
            {
                ASInstalledColormap *ic = LISTELEM_DATA(pcurr);
                if( ic )
                    RequiredCmaps[i++].desired = ic->cmap ;
            }
        }
    }
    while( i < MaxRequiredCmaps )
        RequiredCmaps[i++].desired = None ;

    while( --i >= 0 )
        if( RequiredCmaps[i].desired != RequiredCmaps[i].installed ||
            (RequiredCmaps[i].state == AS_CmapUninstalled && RequiredCmaps[i].installed != None ))
            break;
    /* now lets proceed with installation of cmaps that are new to us : */
    while( i >= 0 )
    {
        XInstallColormap( dpy, RequiredCmaps[i].desired );
        RequiredCmaps[i].installed = RequiredCmaps[i++].desired ;
        RequiredCmaps[i].state = AS_CmapPendingInstall ;
        --i ;
    }
}

static void
install_colormap( Colormap cmap, Window w )
{
    if( InstalledCmapList == NULL )
        InstalledCmapList = create_asbidirlist(destroy_ASInstalledColormap);

    if( InstalledCmapList != NULL )
    {
        ASBiDirElem *pcurr = LIST_START( InstalledCmapList );
        ASInstalledColormap *ic;
        while( pcurr )
        {
            ic = LISTELEM_DATA(pcurr);
            if( ic && ic->cmap == cmap)
            {
                ++(ic->ref_count);
                pop_bidirelem( InstalledCmapList, pcurr );
                correct_colormaps_order();
                return;
            }
            LIST_GOTO_NEXT(pcurr);
        }
        ic = safecalloc( 1, sizeof(ASInstalledColormap));
        ic->cmap = cmap ;
        ic->state = AS_CmapNew ;
        ic->ref_count = 1 ;
        prepend_bidirelem( InstalledCmapList, ic );
        correct_colormaps_order();
    }
}

static void
uninstall_colormap( Colormap cmap, Window w )
{
    if( InstalledCmapList )
    {
        ASBiDirElem *pcurr = LIST_START( InstalledCmapList );
        while( pcurr )
        {
            ASInstalledColormap *ic = LISTELEM_DATA(pcurr);
            if( ic && ic->cmap == cmap)
            {
                if( --(ic->ref_count) <= 0 )
                {
                    destroy_bidirelem( InstalledCmapList, pcurr );
                    correct_colormaps_order();
                }
                return;
            }
            LIST_GOTO_NEXT(pcurr);
        }
    }
}

void
ColormapCleanup()
{
    if( cleanup )
    {
        if( RequiredCmaps )
            free( RequiredCmaps );
        MaxRequiredCmaps = 0;
    }
    if( InstalledCmapList )
        destroy_asbidirlist( InstalledCmapList );
}
/***********************************************************************
 *
 *  Procedure:
 *	HandleColormapNotify - colormap notify event handler
 *
 * This procedure handles both a client changing its own colormap, and
 * a client explicitly installing its colormap itself (only the window
 * manager should do that, so we must set it correctly).
 *
 ***********************************************************************/
static void
DigestCmapEvent( XColormapEvent *cevent, ASWindow *asw )
{
    static XWindowAttributes   attr ;
    if( asw )
    {
        if (cevent->new && asw )
        {
            XGetWindowAttributes (dpy, asw->w, &attr);
            if ( Scr.colormap_win == asw && asw->hints->cmap_windows == NULL )
            {
                last_cmap = attr.colormap;
                last_cmap_state = AS_CmapNew ;
            }
        } else if ( last_cmap == cevent->colormap )
        {   /* Some window installed its colormap, change it back */
            last_cmap_state = (cevent->state == ColormapUninstalled)?
                                    AS_CmapUninstalled:AS_CmapInstalled ;
        }
    }
}

void
HandleColormapNotify (ASEvent *event )
{
    Bool          ReInstall = False;
    XColormapEvent *cevent = event->x.xcolormap;
    ASWindow *asw = event->client ;
    XWindowAttributes   attr ;


    DigestColormapEvent( &(event->x.xcolormap), event->client );

    while (ASCheckTypedEvent (ColormapNotify, &event->x))
	{
        DigestEvent( event );
        DigestColormapEvent( &(event->x.xcolormap), event->client );
    }
    if( last_cmap_state != AS_CmapInstalled )
        XInstallColormap (dpy, last_cmap);
}

void
InstallWindowColormaps( ASWindow *asw )
{


}




/************************************************************************
 *
 * Re-Install the active colormap
 *
 *************************************************************************/
void
ReInstallActiveColormap (void)
{
    correct_InstallWindowColormaps (Scr.colormap_win);
}

/***********************************************************************
 *
 *  Procedure:
 *	InstallWindowColormaps - install the colormaps for one afterstep window
 *
 *  Inputs:
 *	type	- type of event that caused the installation
 *	tmp	- for a subset of event types, the address of the
 *		  window structure, whose colormaps are to be installed.
 *
 ************************************************************************/

void
InstallWindowColormaps (ASWindow *asw)
{
	int           i;
    XWindowAttributes attributes;
	Window        w;
	Bool          ThisWinInstalled = False;


	/* If no window, then install root colormap */
    if ( asw == NULL )
        asw = &Scr.ASRoot;

    Scr.colormap_win = tmp;
	/* Save the colormap to be loaded for when force loading of
	 * root colormap(s) ends.
	 */
	Scr.pushed_window = tmp;
	/* Don't load any new colormap if root colormap(s) has been
	 * force loaded.
	 */
	if (Scr.root_pushes)
		return;

    if (asw->hints->cmap_windows != NULL)
	{
        for (i = 0; asw->hints->cmap_windows[i] != None; ++i)
		{
			w = tmp->hints->cmap_windows[i];
			if (w == tmp->w)
				ThisWinInstalled = True;
			XGetWindowAttributes (dpy, w, &attributes);

			if (last_cmap != attributes.colormap)
			{
				last_cmap = attributes.colormap;
				XInstallColormap (dpy, attributes.colormap);
			}
		}
	}
	if (!ThisWinInstalled)
	{
		if (last_cmap != tmp->attr.colormap)
		{
			last_cmap = tmp->attr.colormap;
			XInstallColormap (dpy, tmp->attr.colormap);
		}
	}
}


/***********************************************************************
 *
 *  Procedures:
 *	<Uni/I>nstallRootColormap - Force (un)loads root colormap(s)
 *
 *	   These matching routines provide a mechanism to insure that
 *	   the root colormap(s) is installed during operations like
 *	   rubber banding or menu display that require colors from
 *	   that colormap.  Calls may be nested arbitrarily deeply,
 *	   as long as there is one UninstallRootColormap call per
 *	   InstallRootColormap call.
 *
 *	   The final UninstallRootColormap will cause the colormap list
 *	   which would otherwise have be loaded to be loaded, unless
 *	   Enter or Leave Notify events are queued, indicating some
 *	   other colormap list would potentially be loaded anyway.
 ***********************************************************************/
void
InstallRootColormap ()
{
	if (Scr.root_pushes == 0)
	{
        ASWindow     *tmp = Scr.pushed_window;
		InstallWindowColormaps (&Scr.ASRoot);
		Scr.pushed_window = tmp;
	}
	Scr.root_pushes++;
	return;
}

/***************************************************************************
 *
 * Unstacks one layer of root colormap pushing
 * If we peel off the last layer, re-install th e application colormap
 *
 ***************************************************************************/
void
UninstallRootColormap ()
{
	Scr.root_pushes--;
	if (!Scr.root_pushes)
	{
		InstallWindowColormaps (Scr.pushed_window);
	}
	return;
}

