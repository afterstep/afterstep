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

#include "../../include/asapp.h"
#include "../../include/afterstep.h"
#include "../../include/screen.h"
#include "../../include/event.h"
#include "asinternals.h"

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
static ASRequiredColormap *RequiredCmaps = NULL ;
static int MaxRequiredCmaps = 0;
static ASHashTable *Cmap2WindowXref = NULL ;

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
    int i = 0;

    if( RequiredCmaps == NULL )
        return;                                /* no colormap management needed */

    if( RootCmap && RootCmap->ref_count > 0 )
        RequiredCmaps[i++].desired = RootCmap->cmap ;
    if( i < MaxRequiredCmaps )
    {
        /* first lets check on semi-mandatory AfterStep colormap */
        if(   AfterStepCmap != NULL && AfterStepCmap != RootCmap &&
            ( i < MaxRequiredCmaps-1 ||  /* either we are allowed several maps at the same time */
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
uninstall_colormap( Colormap cmap, Window w )
{
    Bool freed = False ;

    if( cmap == None )
    {
        freed = True ;
        if( get_hash_item( Cmap2WindowXref, AS_HASHABLE(w), (void**)&cmap ) != ASH_Success )
            return;
    }
    if( InstalledCmapList && cmap != None )
    {
        ASBiDirElem *pcurr = LIST_START( InstalledCmapList );
        if( w != None )
            remove_hash_item (Cmap2WindowXref, AS_HASHABLE(w), NULL, True);
        while( pcurr )
        {
            ASInstalledColormap *ic = LISTELEM_DATA(pcurr);
            if( ic && ic->cmap == cmap)
            {
                if( --(ic->ref_count) <= 0 )
                {
                    if( w == None )
                    {   /*we have to get rid of all the references to this cmap : */
                        ASHashIterator hi ;
                        if( start_hash_iteration (Cmap2WindowXref, &hi) )
                            do
                            {
                                if( (Colormap)curr_hash_data (&hi) == cmap )
                                    remove_curr_hash_item (&hi, True);
                            }while( next_hash_item (&hi));
                    }
                    /* don't do XUninstallCmap if it was already freed by call to XFreePixmap :*/
                    if( freed )
                        ic->cmap = None ;
                    destroy_bidirelem( InstalledCmapList, pcurr );
                    correct_colormaps_order();
                }
                return;
            }
            LIST_GOTO_NEXT(pcurr);
        }
    }
}

static void
install_colormap( Colormap cmap, Window w )
{
    if( InstalledCmapList != NULL )
    {
        ASBiDirElem *pcurr;
        ASInstalledColormap *ic;
        /* lets check if the window is already known to us : */
        if( w != None )
        {
            Colormap old_cmap = None ;
            if( get_hash_item( Cmap2WindowXref, AS_HASHABLE(w), (void**)&old_cmap) == ASH_Success )
            {
                if( cmap == None )
                    cmap = old_cmap ;
                else if( cmap != old_cmap )
                    uninstall_colormap( old_cmap, w );
            }
        }
        if( cmap == None )
            return ;
        if( w != None )
        {
            add_hash_item( Cmap2WindowXref, AS_HASHABLE(w), (void*)cmap);
            add_window_event_mask( w, ColormapChangeMask );
        }
        /* now lets pop this colormap to the top of the list : */
        pcurr = LIST_START( InstalledCmapList );
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
        ic->ref_count = 1 ;
        prepend_bidirelem( InstalledCmapList, ic );
        correct_colormaps_order();
    }
}

/*************************************************************************/
/* Setup and cleanup procedures :                                        */
/*************************************************************************/
void
SetupColormaps()
{
    if( MaxRequiredCmaps == 0 )
    {
        /* we will maintain only list of required colormaps */
        MaxRequiredCmaps = MinCmapsOfScreen(ScreenOfDisplay(dpy,Scr.screen));
        if( MaxRequiredCmaps == 0 )
            return;
        RequiredCmaps = safecalloc( MaxRequiredCmaps, sizeof(ASRequiredColormap));
    }

    if( InstalledCmapList == NULL )
        InstalledCmapList = create_asbidirlist(destroy_ASInstalledColormap);

    if( Cmap2WindowXref == NULL )
        Cmap2WindowXref = create_ashash( 7, NULL, NULL, NULL );
}

void
CleanupColormaps()
{
    if( RequiredCmaps )
        free( RequiredCmaps );
    MaxRequiredCmaps = 0;

    if( InstalledCmapList )
        destroy_asbidirlist( &InstalledCmapList );

    if( Cmap2WindowXref )
        destroy_ashash( &Cmap2WindowXref );
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
DigestColormapEvent( XColormapEvent *cevent )
{
    if (cevent->new && cevent->window )
    {
        if( cevent->colormap == None )
            uninstall_colormap( None, cevent->window );
        else
            install_colormap( cevent->colormap, cevent->window );
    } else
    {   /* Update colormap status in required list : */
        register int i = MaxRequiredCmaps;
        while( --i >= 0 )
            if( RequiredCmaps[i].installed == cevent->colormap )
                RequiredCmaps[i].state = ( cevent->state == ColormapUninstalled)?
                                            AS_CmapUninstalled:AS_CmapInstalled ;
    }
}

void
HandleColormapNotify (ASEvent *event )
{
    Bool            reinstall = False;
    XColormapEvent *cevent = &(event->x.xcolormap);

    do
    {
        DigestColormapEvent( cevent );
        if( !cevent->new && cevent->state == ColormapUninstalled )
            reinstall = True ;

        if( !ASCheckTypedEvent (ColormapNotify, &event->x))
            break;
        DigestEvent( event );
        cevent = &(event->x.xcolormap);
    }while(1);

    if( reinstall )
        correct_colormaps_order();
}
/************************************************************************
 *
 * Re-Install the active colormap
 *
 *************************************************************************/
void
ReInstallActiveColormap (void)
{
    correct_colormaps_order();
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
    XWindowAttributes attr;
    Bool          main_cmap_installed = False ;
    Window        main_w = Scr.Root ;

	/* If no window, then install root colormap */
    if ( asw != NULL )
    {
        if (asw->hints && asw->hints->cmap_windows != NULL)
        {
            int  i = 0;
            for (; asw->hints->cmap_windows[i] != None; ++i)
            {
                Window w = asw->hints->cmap_windows[i];
                if( w == asw->w )
                    main_cmap_installed = True ;
                XGetWindowAttributes (dpy, w, &attr);
                if( attr.colormap )
                    install_colormap(w, attr.colormap);
            }
        }
        main_w = asw->w ;
    }
    if (!main_cmap_installed)
	{
        XGetWindowAttributes (dpy, main_w, &attr);
        if( attr.colormap )
            install_colormap(main_w, attr.colormap);
    }
}

void
UninstallWindowColormaps (ASWindow *asw)
{
    XWindowAttributes attr;
    Bool          main_cmap_done = False ;
    Window        main_w = Scr.Root ;

	/* If no window, then install root colormap */
    if ( asw != NULL )
    {
        if (asw->hints && asw->hints->cmap_windows != NULL)
        {
            int  i = 0;
            for (; asw->hints->cmap_windows[i] != None; ++i)
            {
                Window w = asw->hints->cmap_windows[i];
                if( w == asw->w )
                    main_cmap_done = True ;
                XGetWindowAttributes (dpy, w, &attr);
                uninstall_colormap( attr.colormap, w );
            }
        }
        main_w = asw->w ;
    }
    if (!main_cmap_done)
	{
        XGetWindowAttributes (dpy, main_w, &attr);
        uninstall_colormap(main_w, attr.colormap);
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
    if( RootCmap )
    {
        ++(RootCmap->ref_count);
        if( RootCmap->ref_count == 1 )
            correct_colormaps_order();
    }
}
void
UninstallRootColormap ()
{
    if( RootCmap )
    {
        --(RootCmap->ref_count);
        if( RootCmap->ref_count == 0 )
            correct_colormaps_order();
    }
}

void
InstallAfterStepColormap ()
{
    if( AfterStepCmap )
    {
        ++(AfterStepCmap->ref_count);
        if( AfterStepCmap->ref_count == 1 )
            correct_colormaps_order();
    }
}
void
UninstallAfterStepColormap ()
{
    if( AfterStepCmap )
    {
        --(AfterStepCmap->ref_count);
        if( AfterStepCmap->ref_count == 0 )
            correct_colormaps_order();
    }
}

