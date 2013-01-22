/*
 * Copyright (c) 2012 Sasha Vasko <sasha at aftercode.net>
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
#define LOCAL_DEBUG
#define EVENT_TRACE
#define PRINT_VOLUMES
#define PRINT_MOUNTS

#include "../../configure.h"
#include "../../libAfterStep/asapp.h"
#include "../../libAfterStep/afterstep.h"
#include "../../libAfterStep/screen.h"
#include "../../libAfterStep/parser.h"
#include "../../libAfterStep/mystyle.h"
#include "../../libAfterStep/mystyle_property.h"
#include "../../libAfterStep/balloon.h"
#include "../../libAfterStep/aswindata.h"
#include "../../libAfterStep/decor.h"
#include "../../libAfterStep/event.h"
#include "../../libAfterStep/wmprops.h"
#include "../../libAfterStep/moveresize.h"
#include "../../libAfterStep/shape.h"
#include "../../libAfterStep/module.h"
#include "../../libAfterStep/session.h"
#include "../../libAfterConf/afterconf.h"
#include <gio/gio.h>

#include <signal.h>
#include <unistd.h>
#include <fcntl.h>



#define ENTRY_WIDTH		300
#define DEFAULT_TILE_WIDTH 48
#define DEFAULT_TILE_HEIGHT 48


struct ASGnomeMediaXref{
	char *idString;
	char *description;
	
}GnomeMediaXref[] = 
{ {"drive-removable-media-ata", "" },
  {"drive-removable-media-file", "" },
  {"drive-removable-media-scsi", "" },
  {"drive-removable-media-usb", "" },
  {"drive-removable-media-ieee1394", "" },
  {"drive-removable-media", "" },
  {"drive-harddisk-ata", "" },
  {"drive-harddisk-scsi", "" },
  {"drive-harddisk-usb", "" },
  {"drive-harddisk-ieee1394", "" },
  {"drive-harddisk", "" },
  {"media-optical-cd-rom", "CD-ROM Disc"},
  {"media-optical-cd-r",          "CD-R Disc"},
  {"media-optical-cd-rw",         "CD-RW Disc"},
  {"media-optical-dvd-rom",       "DVD-ROM Disc"},
  {"media-optical-dvd-r",         "DVD-ROM Disc"},
  {"media-optical-dvd-rw",        "DVD-RW Disc"},
  {"media-optical-dvd-ram",       "DVD-RAM Disc"},
  {"media-optical-dvd-r-plus",    "DVD+R Disc"},
  {"media-optical-dvd-rw-plus",   "DVD+RW Disc"},
  {"media-optical-dvd-dl-r-plus", "DVD+R DL Disc"},
  {"media-optical-dvd-dl-r-plus", "DVD+RW DL Disc"},
  {"media-optical-bd-rom",        "Blu-Ray Disc"},
  {"media-optical-bd-r",          "Blu-Ray R Disc"},
  {"media-optical-bd-re",         "Blu-Ray RW Disc"},
  {"media-optical-hddvd-rom",     "HD DVD Disc"},
  {"media-optical-hddvd-r",       "HD DVD-R Disc"},
  {"media-optical-hddvd-rw",      "HD DVD-RW Disc"},
  {"media-optical-mo",            "MO Disc"},
  {"media-optical-mrw",           "MRW Disc"},
  {"media-optical-mrw-w",         "MRW/W Disc"},
  {"media-optical", "" },
  {"media-flash-cf", "" },
  {"media-flash-ms", "" },
  {"media-flash-sm", "" },
  {"media-flash-sd", "" },
	{"media-flash-sdhc", "" },
  {"media-flash", "" },
  {"media-floppy-zip", "" },
  {"media-floppy-jaz", "" },
	{"media-floppy", "" },
  {NULL, NULL}
};


typedef struct ASVolume {

#define ASVolume_Mounted	 	(0x01<<0)
#define ASVolume_Ejectable 	(0x01<<1)
#define ASVolume_Video			(0x01<<2)
#define ASVolume_Audio			(0x01<<3)
#define ASVolume_MountRequested	 		(0x01<<8)
#define ASVolume_EjectRequested 		(0x01<<9)
#define ASVolume_UnmountRequested 	(0x01<<10)
#define ASVolume_RequestPending 	(0x0F<<8)
	ASFlagType flags;

#define ASVolume_isMounted(v)	 	  ((v) && get_flags((v)->flags,ASVolume_Mounted))
#define ASVolume_isEjectable(v) 	((v) && get_flags((v)->flags,ASVolume_Ejectable))
#define ASVolume_isVideo(v) 		 	((v) && get_flags((v)->flags,ASVolume_Video))
#define ASVolume_isAudio(v) 			((v) && get_flags((v)->flags,ASVolume_Audio))
#define ASVolume_isRequestPending(v) 			((v) && get_flags((v)->flags,ASVolume_RequestPending))

	char* name;

	GVolume *gVolume;
	GMount  *gMount;
	
	GIcon *icon;
	ASImage* iconIm;
	ASImage* iconImScaled;
	char *idString;
	int   type; /* index in above Xref, -1 if unknown */

	ASCanvas   *canvas;
  ASTBarData *contents;

}ASVolume;

typedef struct ASMountState
{
	ASFlagType flags ;
	
	int tileWidth, tileHeight;
	
	ASBiDirList *volumes;
	GVolumeMonitor *volumeMonitor;

	Window mainWindow;
	ASCanvas   *mainCanvas;	

#define BUTTONS_NUM 3
#define MOUNT_CONTEXT C_TButton0
#define UNMOUNT_CONTEXT C_TButton1
#define EJECT_CONTEXT C_TButton2
	struct button_t* buttons[BUTTONS_NUM];
	
	ASVolume* 	pressedVolume;
	int 				pressedContext;	
}ASMountState;

ASMountState AppState ;
ASMountConfig *Config = NULL ;

ASGeometry TileSize = {0};

CommandLineOpts ASMount_cmdl_options[3] =
{
	{NULL, "tile-size","Overrides ASMount's TileSize setting", NULL, handler_set_geometry, &TileSize, 0, CMO_HasArgs },
  {NULL, NULL, NULL, NULL, NULL, NULL, 0 }
};


/******************************************************************************/
/* Forward declarations :*/

void ASVolume_destroy( void *data );
char* ASVolume_toString (struct ASVolume *v);
struct ASVolume *ASVolume_newGVolume (GVolume *g_v);
struct ASVolume *ASVolume_newGMount (GMount *g_m);
static void mount_added   (GVolumeMonitor *monitor, GMount *mount, GObject *unused);
static void mount_changed (GVolumeMonitor *monitor, GMount *mount, GObject *unused);
static void mount_removed (GVolumeMonitor *monitor,	GMount *mount, GObject *unused);
static void volume_added  (GVolumeMonitor *monitor, GVolume *volume, GObject *unused);
static void volume_changed(GVolumeMonitor *monitor,	GVolume *volume, GObject *unused);
static void volume_removed(GVolumeMonitor *monitor,	GVolume *volume, GObject *unused);


void DeadPipe (int nonsense);
void SetASMountLook();
void init_ASMount(ASFlagType flags, const char *cmd);
void updateVolumeContents (ASVolume *v);
void redecorateVolumes();
void createMainWindow();
void setHints (int maxWidth, int maxHeight);
void addTimeout();
/******************************************************************************/
/******************************************************************************/
/* Copied from gnome-applets with minor adaptations:                          */
/* Original copyright notice applies to this section only :
 * Drive Mount Applet
 * Copyright (c) 2004 Canonical Ltd
 * Copyright 2008 Pierre Ossman
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author:
 *   James Henstridge <jamesh@canonical.com>
 */
/******************************************************************************/
/*
 * gvm_check_dvd_only - is this a Video DVD?
 *
 * Returns TRUE if this was a Video DVD and FALSE otherwise.
 * (the original in gvm was also running the autoplay action,
 * I removed that code, so I renamed from gvm_check_dvd to
 * gvm_check_dvd_only)
 */
static gboolean
gvm_check_dvd_only (const char *udi, const char *device, const char *mount_point)
{
	char *path;
	gboolean retval;
	
	path = g_build_path (G_DIR_SEPARATOR_S, mount_point, "video_ts", NULL);
	retval = g_file_test (path, G_FILE_TEST_IS_DIR);
	g_free (path);
	
	/* try the other name, if needed */
	if (retval == FALSE) {
		path = g_build_path (G_DIR_SEPARATOR_S, mount_point,
				     "VIDEO_TS", NULL);
		retval = g_file_test (path, G_FILE_TEST_IS_DIR);
		g_free (path);
	}
	
	return retval;
}
/* END copied from gnome-volume-manager/src/manager.c */

static gboolean
check_dvd_video (GVolume *volume)
{
	GFile *file;
	char *udi, *device_path, *mount_path;
	gboolean result;
	GMount *mount;

	if (!volume)
		return FALSE;

	mount = g_volume_get_mount (volume);
	if (!mount)
	    return FALSE;

	file = g_mount_get_root (mount);
	g_object_unref (mount);
	
	if (!file)
		return FALSE;

	mount_path = g_file_get_path (file);

	g_object_unref (file);

	device_path = g_volume_get_identifier (volume,
					       G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE);
	udi = g_volume_get_identifier (volume,
				       G_VOLUME_IDENTIFIER_KIND_HAL_UDI);

	result = gvm_check_dvd_only (udi, device_path, mount_path);

	g_free (device_path);
	g_free (udi);
	g_free (mount_path);
	return result;
}

static gboolean
check_audio_cd (GVolume *volume)
{
	GFile *file;
	char *activation_uri;
	GMount *mount;

	if (!volume)
		return FALSE;

	mount = g_volume_get_mount (volume);
	if (!mount)
	    return FALSE;

	file = g_mount_get_root (mount);
	g_object_unref (mount);

	if (!file)
		return FALSE;

	activation_uri = g_file_get_uri (file);

	g_object_unref (file);

	/* we have an audioCD if the activation URI starts by 'cdda://' */
	gboolean result = (strncmp ("cdda://", activation_uri, 7) == 0);
	g_free (activation_uri);
	return result;
}

/******************************************************************************/
/* End of Copied from gnome-applets : */
/******************************************************************************/


ASVolume *ASVolume_create ()
{
	ASVolume *v;
	v = safecalloc (1, sizeof(ASVolume));
	return v;
}

ASImage* GIcon2ASImage (GIcon *icon)
{
	char        **icon_names;
	ASImage* asim = NULL;

	if (icon == NULL)
		return NULL;
	g_object_get (icon, "names", &icon_names, NULL);

#if 0
	{
		GdkPixbuf* iconPixbuf;
		GtkIconInfo  *icon_info;
		GtkIconTheme *icon_theme;
		GError       *error = NULL;
		icon_info = gtk_icon_theme_choose_icon (gtk_icon_theme_get_default(), (const char **)icon_names, DEFAULT_TILE_WIDTH, 0);
		icon_theme = gtk_icon_theme_get_for_screen (gdk_screen_get_default());
  	icon_info = gtk_icon_theme_lookup_by_gicon (icon_theme, v->icon, DEFAULT_TILE_WIDTH, GTK_ICON_LOOKUP_USE_BUILTIN);

		iconPixbuf = gtk_icon_info_load_icon (icon_info, &error);
		gtk_icon_info_free (icon_info);

		if (iconPixbuf )
			asim = GdkPixbuf2ASImage (iconPixbuf);
		else {
			g_warning ("could not load icon pixbuf: %s\n", error->message);
	  	g_clear_error (&error);
		}
	}
#else	
	{
		int i;
		for (i = 0 ; icon_names[i] ; ++i)
			if ((asim = load_environment_icon ("devices", icon_names[i], DEFAULT_TILE_WIDTH)) != NULL)
				break;
	}
#endif
	g_strfreev (icon_names);
	return asim;
}

void ASVolume_parseGnomeIconString (ASVolume *v)
{
	if (v) { 
		static char signature[] = ". GThemedIcon ";
		destroy_string (&(v->idString));

		if (v->icon) {
			ASImage *tmp = GIcon2ASImage (v->icon);
			gchar* str = g_icon_to_string (v->icon);

			show_activity ("volume added with icon \"%s\".", str);

			if (str) {
				if (strncmp (str, signature, sizeof(signature)-1) == 0)
					parse_token (&str [sizeof(signature)-1], &(v->idString));
				safefree (str);
			}

			if (tmp) {
				int l, t, r, b;
				get_asimage_closure (tmp, &l, &t, &r, &b, 10);
				v->iconIm = tile_asimage (Scr.asv, tmp, l, t, r+1-l, b+1-t, 0x4F7F7F7F, ASA_ASImage, 100, ASIMAGE_QUALITY_DEFAULT);
				safe_asimage_destroy (tmp);
			}					
		
			LOCAL_DEBUG_OUT ("Icon asim = %p", v->iconIm);
		}
		if (!v->idString)
			v->idString = mystrdup ("unknown");
	}
}

void ASVolume_freeData( ASVolume *v)
{
	destroy_string (&(v->name));
		
	if (v->icon)
		{
			if (v->iconIm)
				safe_asimage_destroy (v->iconIm);
			g_object_unref (v->icon);	
			v->icon = NULL;
		}
	destroy_string (&(v->idString));
}

void GVolume2ASVolume (ASVolume *v,  GVolume *g_v)
{
	GMount *mount = g_volume_get_mount (g_v);
	v->gVolume = g_v;
	ASVolume_freeData (v);
	set_string (&(v->name), g_volume_get_name (v->gVolume));
LOCAL_DEBUG_OUT ("mount = %p", mount);	
	v->flags = 0;
	if (g_volume_can_eject (g_v))
		set_flags (v->flags, ASVolume_Ejectable);
	if (mount) {
		set_flags (v->flags, ASVolume_Mounted);
	  g_object_unref (mount);
	}
	if (check_audio_cd (g_v))
		set_flags (v->flags, ASVolume_Audio);
	else if (check_dvd_video (g_v))
		set_flags (v->flags, ASVolume_Video);
	
	v->icon = g_volume_get_icon (g_v);		
	ASVolume_parseGnomeIconString (v);
}

ASVolume *ASVolume_newGVolume (GVolume *g_v)
{
	if (g_v == NULL)
		return NULL;
	else	{	
		ASVolume *v = ASVolume_create();
		GVolume2ASVolume (v,  g_v);
		{
   		char *volume_str = ASVolume_toString(v);
	  	show_activity("%s\n", volume_str);
   		safefree(volume_str);
		}
		return v;
	}
}

ASVolume *ASVolume_newGMount (GMount *g_m)
{
	ASVolume *v;
	
	if (g_m == NULL || g_mount_is_shadowed (g_m))      
		return NULL;
	else {
		GVolume *volume = g_mount_get_volume (g_m);
  	if (volume) {
			g_object_unref (volume);
			return NULL;
    }
	}
		
	v = ASVolume_create();
	v->gMount = g_m;
	v->name = g_mount_get_name (v->gMount);

	if (g_mount_can_eject (g_m))
		set_flags (v->flags, ASVolume_Ejectable);
	set_flags (v->flags, ASVolume_Mounted);

	v->icon = g_mount_get_icon (g_m);
	ASVolume_parseGnomeIconString (v);
	
	{
 		char *mount_str = ASVolume_toString(v);
		show_activity("%s\n", mount_str);
		safefree(mount_str);
	}
	return v;
}

void ASVolume_destroy( void *data )
{
	if (data)	{
		ASVolume *v = (ASVolume *) data;
		if (v->gVolume)
			{
				g_object_unref (v->gVolume);	
				v->gVolume = NULL;
			}
		if (v->gMount)
			{
				g_object_unref (v->gMount);	
				v->gMount = NULL;
			}
		ASVolume_freeData(v);
		safefree (v);
	}
}

ASBiDirElem *ASVolume_findGVolume_item (ASBiDirList *list, GVolume *g_v) 
{
	ASBiDirElem *curr = list->head;
	while (curr) {
		ASVolume *v = (ASVolume*)curr->data;
		if (v->gVolume == g_v)
			return curr;
		curr = curr->next;
	}
	return NULL;
}



Bool ASVolume_isVolume(ASVolume *v)
{
	return (v && v->gVolume);
}

Bool ASVolume_isMount(ASVolume *v)
{
	return (v && v->gMount);
}

char* ASVolume_toString (ASVolume *v)
{
	char *str = NULL;
	if (v) {
		str = safemalloc (strlen (v->name) + 1024+256);
		if (ASVolume_isVolume(v))
			strcpy (str, "volume: ");
		else if (ASVolume_isMount(v))
			strcpy (str, "mount : ");
		else {
			safefree (str);
			str = NULL;
		}
		if (str) {
			strcat (str, v->name);
			if (ASVolume_isEjectable(v))
				strcat (str, ", ejectable");
			if (ASVolume_isMounted(v))
				strcat (str, ", mounted");
			else
				strcat (str, ", not mounted");
			if (ASVolume_isVideo(v))
				strcat (str, ", video");
			if (ASVolume_isAudio(v))
				strcat (str, ", audio");
			strcat (str, ", ");
			strcat (str, v->idString);
		}
	}
	return str ? str : mystrdup("");
}


void check_drives()
{

/*	destroy_asbidirlist (&Volumes); */
}


typedef struct {
	int x;
	int y;
	ASVolume *v;
} ASVolumeCanvasAtPos;

Bool volumeAtPos(void *data, void *aux_data) {
  ASVolume *v = (ASVolume*)data;
	ASVolumeCanvasAtPos *adata = (ASVolumeCanvasAtPos*) aux_data;
	int x = adata->x;// - v->canvas->root_x;
	int y = adata->y;// - v->canvas->root_y;
  register ASTBarData *bar = v->contents;
	LOCAL_DEBUG_OUT ("pos = %+d%+d, canvas_geom = %dx%d%+d%+d, bar_geom = %dx%d%+d%+d", 
	                  x, y, v->canvas->width, v->canvas->height, v->canvas->root_x, v->canvas->root_y, bar->width, bar->height, bar->root_x, bar->root_y); 
  if (bar
	    && ( bar->root_x <= x && bar->root_x+bar->width > x &&
           bar->root_y <= y && bar->root_y+bar->height > y )) {
		adata->v = v;
		return False;
	}
  return True;
}

Bool volumeWindowMoved(void *data, void *aux_data) {
  ASVolume *v = (ASVolume*)data;
	handle_canvas_config (v->canvas);	
	if (update_astbar_transparency (v->contents, v->canvas, False)) {
		render_astbar (v->contents, v->canvas );
  	update_canvas_display (v->canvas );
	}
	return True;
}

ASVolume*
position2Volume( int x, int y )
{
	ASVolumeCanvasAtPos adata;
	adata.x = x;
	adata.y = y;
	adata.v = NULL;

	iterate_asbidirlist (AppState.volumes, volumeAtPos, &adata, NULL, False);
  return adata.v;
}


/******************************************************************************/
/* Signal Handlers : */
/******************************************************************************/
static void mount_added   (GVolumeMonitor *monitor, GMount *mount, GObject *unused)
{
	SHOW_CHECKPOINT;
	/* TODO */
	show_activity ("%s", __FUNCTION__);
}

static void mount_changed (GVolumeMonitor *monitor, GMount *mount, GObject *unused)
{
	SHOW_CHECKPOINT;
	/* TODO */
	
}

static void mount_removed (GVolumeMonitor *monitor,	GMount *mount, GObject *unused)
{
	SHOW_CHECKPOINT;
	/* TODO */
	show_activity ("%s", __FUNCTION__);
}

static void volume_added  (GVolumeMonitor *monitor, GVolume *volume, GObject *unused)
{
	ASVolume *v = ASVolume_newGVolume (volume);
	SHOW_CHECKPOINT;
	if (v) {
		show_progress ("Volume \"%s\" added", v->name);
		append_bidirelem (AppState.volumes, v);
		redecorateVolumes();
	}
}

void
ASVolume_refreshDisplay (ASVolume *v)
{
	updateVolumeContents (v);		
	render_astbar (v->contents, v->canvas );
  update_canvas_display (v->canvas );
}

static void
volume_timer_handler (void *data)
{
	SHOW_CHECKPOINT;
  GVolume *volume = (GVolume*) data;
	ASBiDirElem *item = ASVolume_findGVolume_item (AppState.volumes, volume);
	if (item) {
		ASVolume *v = (ASVolume*)item->data;
		LOCAL_DEBUG_OUT ("Volume changed %s", v == NULL ? "(none)": v->name);
		GVolume2ASVolume (v, volume);
		ASVolume_refreshDisplay (v);
	}
}

static void volume_changed(GVolumeMonitor *monitor,	GVolume *volume, GObject *unused)
{
	SHOW_CHECKPOINT;
	timer_remove_by_data (volume);
  timer_new (300, &volume_timer_handler, (void *)volume);
	addTimeout();
}

static void volume_removed(GVolumeMonitor *monitor,	GVolume *volume, GObject *unused)
{
	ASBiDirElem *item = ASVolume_findGVolume_item (AppState.volumes, volume);
	SHOW_CHECKPOINT;
	if (item) {
		destroy_bidirelem (AppState.volumes, item);
		redecorateVolumes();
	}
}

void mountVolumeAsyncCallback (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
  GVolume *volume = (GVolume*) user_data;
	ASBiDirElem *item = ASVolume_findGVolume_item (AppState.volumes, volume);
	if (item) {
		GError* error = NULL;
		ASVolume *v = (ASVolume*)item->data;
		Bool success = g_volume_mount_finish (v->gVolume, res, &error);
		LOCAL_DEBUG_OUT ("result = %p, eror = %p", res, error);
		clear_flags (v->flags, ASVolume_MountRequested);
		if (success)
			GVolume2ASVolume (v, volume);
		else
			show_error( "Mount on volume \"%s\" failed with message \"%s\"", v->name, error? error->message : "unknown error");
		ASVolume_refreshDisplay (v);
	}
}

static void mountVolume (ASVolume *v)
{
	SHOW_CHECKPOINT;
	if (ASVolume_isVolume (v) 
	    && !ASVolume_isRequestPending(v)
			&& !ASVolume_isMounted(v)) {
		GMountOperation *mount_op =NULL;
#ifdef HAVE_GTK		
	  /*mount_op = gtk_mount_operation_new (NULL);*/
#endif		
		set_flags (v->flags, ASVolume_MountRequested);
		ASVolume_refreshDisplay (v);
		LOCAL_DEBUG_OUT ("Mounting volume %s", v == NULL ? "(none)": v->name);
		g_volume_mount (v->gVolume, G_MOUNT_MOUNT_NONE, mount_op, NULL, mountVolumeAsyncCallback, v->gVolume);
		if (mount_op)
		  g_object_unref (mount_op);
	}
}

void ejectVolumeAsyncCallback (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
  GVolume *volume = (GVolume*) user_data;
	ASBiDirElem *item = ASVolume_findGVolume_item (AppState.volumes, volume);
	if (item) {
		GError* error = NULL;
		ASVolume *v = (ASVolume*)item->data;
		Bool success = g_volume_eject_with_operation_finish (v->gVolume, res, &error);
		LOCAL_DEBUG_OUT ("result = %p, eror = %p", res, error);
		clear_flags (v->flags, ASVolume_EjectRequested);
		if (success)
			GVolume2ASVolume (v, volume);
		else
			show_error( "Eject on volume \"%s\" failed with message \"%s\"", v->name, error ? error->message : "unknown error");
		ASVolume_refreshDisplay (v);
	}
}

static void ejectVolume (ASVolume *v)
{
	SHOW_CHECKPOINT;
	if (ASVolume_isVolume (v) 
	    && !ASVolume_isRequestPending(v)
			&& ASVolume_isEjectable(v)){
		set_flags (v->flags, ASVolume_EjectRequested);
		ASVolume_refreshDisplay (v);
		LOCAL_DEBUG_OUT ("Ejecting volume %s", v == NULL ? "(none)": v->name);
		g_volume_eject_with_operation (v->gVolume, G_MOUNT_UNMOUNT_NONE,
		                               NULL, NULL,
		                               (GAsyncReadyCallback) ejectVolumeAsyncCallback,
	  	                             v->gVolume);
	}
}

void unmountVolumeAsyncCallback (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
  GVolume *volume = (GVolume*) user_data;
	ASBiDirElem *item = ASVolume_findGVolume_item (AppState.volumes, volume);
	if (item) {
		GError* error = NULL;
		ASVolume *v = (ASVolume*)item->data;
		GMount *mount = (GMount*)source_object;
		Bool success = False;
		if (mount) {
			success = g_mount_unmount_with_operation_finish(mount, res, &error);
		  g_object_unref (mount);
		}
		LOCAL_DEBUG_OUT ("result = %p, eror = %p", res, error);
 		clear_flags (v->flags, ASVolume_UnmountRequested);
		if (success)
			GVolume2ASVolume (v, volume);
		else
			show_error( "Unmount on volume \"%s\" failed with message \"%s\"", v->name, error ? error->message : "unknown error" );
		ASVolume_refreshDisplay (v);
	}
}

static void unmountVolume (ASVolume *v)
{
	SHOW_CHECKPOINT;
	if (ASVolume_isVolume (v) 
	    && !ASVolume_isRequestPending(v)
			&& ASVolume_isMounted(v)){
		GMount *mount = g_volume_get_mount (v->gVolume);
		LOCAL_DEBUG_OUT ("Unmounting volume %s", v == NULL ? "(none)": v->name);
		if (mount) {
			set_flags (v->flags, ASVolume_UnmountRequested);
			ASVolume_refreshDisplay (v);
			g_mount_unmount_with_operation (mount, G_MOUNT_UNMOUNT_NONE, NULL, NULL, unmountVolumeAsyncCallback, v->gVolume);
  	  /* g_object_unref (mount) will be called in AsyncResult function above */
		}
	}		
}

/*************************************************************************
 * Event handling :
 *************************************************************************/
void 
depressButton () {
	if (AppState.pressedVolume != NULL) {
		set_astbar_btn_pressed (AppState.pressedVolume->contents, 0);
    set_astbar_pressed (AppState.pressedVolume->contents, AppState.pressedVolume->canvas, False );
    if( is_canvas_dirty(AppState.pressedVolume->canvas) )
	    update_canvas_display(AppState.pressedVolume->canvas);
	}
	AppState.pressedVolume = NULL;
	AppState.pressedContext = 0;
}

void 
releaseButton () {
	if (AppState.pressedVolume != NULL) {
		switch (AppState.pressedContext) {
			case MOUNT_CONTEXT: mountVolume (AppState.pressedVolume); break;
			case UNMOUNT_CONTEXT: unmountVolume (AppState.pressedVolume); break;
			case EJECT_CONTEXT: ejectVolume (AppState.pressedVolume); break;
		}
	}
	depressButton ();
}

void 
pressButton (ASVolume *event_v, ASEvent *event)
{
	depressButton();
	if (event_v != NULL && event->context != 0 && !ASVolume_isRequestPending(event_v)) {
		AppState.pressedVolume = event_v;
		AppState.pressedContext = event->context;
		set_astbar_btn_pressed (event_v->contents, event->context);
    set_astbar_pressed (event_v->contents, event_v->canvas, False );
    if( is_canvas_dirty(AppState.pressedVolume->canvas) ){
	    update_canvas_display(AppState.pressedVolume->canvas);
		}
	}
}		

void
DispatchEvent (ASEvent * event)
{
	ASVolume *event_volume = NULL;
  SHOW_EVENT_TRACE(event);

	if( (event->eclass & ASE_POINTER_EVENTS) != 0 ) {
    XKeyEvent *xk = &(event->x.xkey);
		int pointer_root_x = xk->x_root;
		int pointer_root_y = xk->y_root;
		event_volume = position2Volume (pointer_root_x,// - (int)AppState.mainCanvas->bw,
                                    pointer_root_y);// - (int)AppState.mainCanvas->bw);
	
		LOCAL_DEBUG_OUT ("event_volume = %p, name = \"%s\"", event_volume, event_volume ? event_volume->name : "(none)");
		if(is_balloon_click( &(event->x) ) )
		{
			withdraw_balloon(NULL);
			return;
		}
		if (event_volume)
			event->context = check_astbar_point( event_volume->contents, pointer_root_x, pointer_root_y );
	}

  event->client = NULL ;

  switch (event->x.type)  {
		case ConfigureNotify:
				{
	        ASFlagType changes = handle_canvas_config (AppState.mainCanvas);
          if( changes != 0 )
					{
	          set_root_clip_area( AppState.mainCanvas );
						iterate_asbidirlist (AppState.volumes, volumeWindowMoved, NULL, NULL, False);
					}
					show_activity ("changes = 0x%lx", changes);
				}
        break;
    case KeyPress :
         return ;
    case KeyRelease :
				return ;
    case ButtonPress:
			  pressButton (event_volume, event);				
				return;
    case ButtonRelease:
LOCAL_DEBUG_OUT( "state(0x%X)->state&ButtonAnyMask(0x%X)", event->x.xbutton.state, event->x.xbutton.state&ButtonAnyMask );
        if( (event->x.xbutton.state&ButtonAnyMask) == (Button1Mask<<(event->x.xbutton.button-Button1)) )
					releaseButton();
			return ;
    case EnterNotify :
			if( event->x.xcrossing.window == Scr.Root )
				withdraw_active_balloon();
			return ;
    case LeaveNotify :
    case MotionNotify :
			{
				static Bool root_pointer_moved = True ;
				if( event->x.type == MotionNotify ) 
					root_pointer_moved = True ; 
    	  if (event_volume)	{	
        	on_astbar_pointer_action (event_volume->contents, event->context, (event->x.type == LeaveNotify), root_pointer_moved);
					root_pointer_moved = False ; 
				}
				return ;
			}
    case ClientMessage:
      LOCAL_DEBUG_OUT("ClientMessage(\"%s\",format = %d, data=(%8.8lX,%8.8lX,%8.8lX,%8.8lX,%8.8lX)", XGetAtomName( dpy, event->x.xclient.message_type ), event->x.xclient.format, event->x.xclient.data.l[0], event->x.xclient.data.l[1], event->x.xclient.data.l[2], event->x.xclient.data.l[3], event->x.xclient.data.l[4]);
      if ( event->x.xclient.format == 32 &&
           event->x.xclient.data.l[0] == _XA_WM_DELETE_WINDOW )
			{
	      DeadPipe(0);
      }
			return ;
	  case PropertyNotify:
    	if( event->x.xproperty.atom == _XA_NET_WM_STATE )
      {
				LOCAL_DEBUG_OUT( "_XA_NET_WM_STATE updated!%s","");
				return;
			}
			handle_wmprop_event (Scr.wmprops, &(event->x));
      if( event->x.xproperty.atom == _AS_BACKGROUND )
      {
      	LOCAL_DEBUG_OUT( "root background updated!%s","");
        safe_asimage_destroy( Scr.RootImage );
        Scr.RootImage = NULL ;
				iterate_asbidirlist (AppState.volumes, volumeWindowMoved, NULL, NULL, False);
      }else if( event->x.xproperty.atom == _AS_STYLE )
			{
				LOCAL_DEBUG_OUT( "AS Styles updated!%s","");
				mystyle_list_destroy_all(&(Scr.Look.styles_list));
				LoadColorScheme();
				SetASMountLook();
	/* now we need to update everything */
				redecorateVolumes ();	
				iterate_asbidirlist (AppState.volumes, volumeWindowMoved, NULL, NULL, False);
			}
			return ;	
		default:
#ifdef XSHMIMAGE
			LOCAL_DEBUG_OUT( "XSHMIMAGE> EVENT : completion_type = %d, event->type = %d ", Scr.ShmCompletionEventType, event->x.type );
			if( event->x.type == Scr.ShmCompletionEventType )
				handle_ShmCompletion( event );
#endif /* SHAPE */
      return;
	}
  /*update_pager_shape();*/
}

typedef struct _x11_source {
  GSource source;
  Display *dpy;
//  Window w;
	int pending_events;
} x11_source_t;

static gboolean
x11_fd_prepare(GSource *source,
           gint *timeout)
{
  *timeout = -1;
  return FALSE;
}

static int timers_count = 0;

static gboolean
my_timer_handle(GSource* source)
{
	SHOW_CHECKPOINT;
	timer_handle ();
	timers_count--;
	addTimeout();	
	return FALSE;
}

void addTimeout()
{
	time_t sec, usec;
  if (timers_count <= 0 && timer_delay_till_next_alarm (&sec, &usec)){
		SHOW_CHECKPOINT;
		time_t tout = ((sec * 1000) + (usec / 1000))+100; 
		LOCAL_DEBUG_OUT ("timeout in %ld millisecs", tout);
		g_timeout_add (tout,(GSourceFunc)my_timer_handle, NULL);
		timers_count++;		
	}
}

static void
HandleXEvent()
{
	int handled = 0, pending;
  ASEvent event;
#if 0
	while (XQLength(dpy)>0)
#else
	while ((pending = XPending(dpy))>0)
#endif	
	{
//		show_debug (__FILE__,__FUNCTION__,__LINE__, "%d pending X events", pending);
		if( ASNextEvent (&(event.x), False) ){
   		event.client = NULL ;
			LOCAL_DEBUG_OUT("event = %d", event.x.type);
     	setup_asevent_from_xevent( &event );
     	DispatchEvent( &event );
			handled++;
		}
	}
	if (handled > 0) {
		addTimeout();
	}
}

#if 0
static gboolean
x11_fd_check (GSource *source)
{

//	SHOW_CHECKPOINT;
/* for some reason if events are handled in dispatch - it messes up the timeout events */
//	show_debug (__FILE__,__FUNCTION__,__LINE__,"XQLength = %d, x_fd = %d", XQLength(dpy), x_fd);
	HandleXEvent();
//	XSync (dpy, True);
//	show_debug (__FILE__,__FUNCTION__,__LINE__,"XQLength = %d", XQLength(dpy));
	return FALSE;
}

static gboolean
x11_fd_dispatch(GSource* source, GSourceFunc callback, gpointer user_data)
{
	SHOW_CHECKPOINT;
  /* never actually get here - see _check() above */
	/* if FALSE then our GSource will get removed */
  return TRUE;
}
#else
static gboolean x11_fd_check (GSource *source){	return FALSE; }
static gboolean x11_fd_dispatch(GSource* source, GSourceFunc callback, gpointer user_data) { return TRUE;}
#endif
/******************************************************************************/
void mainLoop ()
{
  GPollFD dpy_pollfd = {x_fd, G_IO_IN, 0};// | G_IO_HUP | G_IO_ERR
	GSourceFuncs x11_source_funcs = {
    x11_fd_prepare,
    x11_fd_check,    x11_fd_dispatch,
    NULL, /* finalize */
    NULL, /* closure_callback */
    NULL /* closure_marshal */
      };

	GSource *x11_source = g_source_new(&x11_source_funcs, sizeof(x11_source_t));

	((x11_source_t*)x11_source)->dpy = dpy;
	((x11_source_t*)x11_source)->pending_events = 0;
	g_source_add_poll(x11_source, &dpy_pollfd);
	g_source_attach(x11_source,NULL); /* must use default context so that DBus signal handling is not impeded */
	{
#if 0
		GMainLoop *mainloop = g_main_loop_new(NULL,FALSE);
		g_main_loop_run(mainloop);
#else
		GMainContext *ctx = g_main_context_default ();
		GPollFD qfd_a[10];
		do {
	    gint timeout;
			int priority = 0;
			int i;
    	g_main_context_prepare (ctx, &priority);		
    	gint recordCount = g_main_context_query(ctx, 2, &timeout, qfd_a, 10);
			/* we need to specify the timeout, in order for glib signals to propagate properly
			   Ain't that stupid!!!!!! */
			g_poll (qfd_a, recordCount, 200);
			/* don't want to trust glib's main loop with our X event handling - it gets all screwed up */
#if 1
			for (i = 0 ; i < recordCount ; ++i)
				if (qfd_a[i].revents) {
/*					show_progress ( "%d: fd = %d, events = 0x%X, revents = 0x%X", time(NULL), qfd_a[i].fd, qfd_a[i].events, qfd_a[i].revents); */
					if (qfd_a[i].fd == x_fd) {
						HandleXEvent();
						qfd_a[i].revents = 0;
					}
				}
#endif  	 
		  g_main_context_check(ctx, -1, qfd_a, recordCount);
			g_main_context_dispatch (ctx);
		}while (1);
#endif		
	}
}	

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/*   Configuration handling */
/*****************************************************************************
 * This routine is responsible for reading and parsing the config file
 ****************************************************************************/

void
SetASMountLook(){
  int i ;
  char *default_style = safemalloc( 1+strlen(MyName)+1);
  char buf[4096];

	default_style[0] = '*' ;
	strcpy (&(default_style[1]), MyName );

  mystyle_get_property (Scr.wmprops);
  for( i = 0 ; i < BACK_STYLES ; ++i )
		Scr.Look.MSWindow[i] = NULL ;

  Scr.Look.MSWindow[BACK_UNFOCUSED] = mystyle_find( Config->UnmountedStyle );
	LOCAL_DEBUG_OUT( "Configured MyStyle %d \"%s\" is %p", BACK_UNFOCUSED, Config->UnmountedStyle?Config->UnmountedStyle:"(null)", Scr.Look.MSWindow[BACK_UNFOCUSED] );
  Scr.Look.MSWindow[BACK_FOCUSED] = mystyle_find( Config->MountedStyle );
	LOCAL_DEBUG_OUT( "Configured MyStyle %d \"%s\" is %p", BACK_FOCUSED, Config->MountedStyle?Config->MountedStyle:"(null)", Scr.Look.MSWindow[BACK_FOCUSED] );

  for( i = 0 ; i < 2 ; ++i )  {
   static char *default_window_style_name[BACK_STYLES] ={"unfocused_window_style", "sticky_window_style", NULL};
   static char *default_style_names[DESK_STYLES] ={"*%sUnmounted", "*%sMounted" };

   sprintf( buf, default_style_names[i], MyName );
   if( Scr.Look.MSWindow[i] == NULL )
  	 Scr.Look.MSWindow[i] = mystyle_find (buf);
   if( Scr.Look.MSWindow[i] == NULL )
  	 Scr.Look.MSWindow[i] = mystyle_find (default_window_style_name[i]);
   if( Scr.Look.MSWindow[i] == NULL && i != BACK_URGENT )
     Scr.Look.MSWindow[i] = mystyle_find_or_default( default_style );
		show_activity( "MyStyle %d is \"%s\"", i, Scr.Look.MSWindow[i]?Scr.Look.MSWindow[i]->name:"none" );
  }
  
	free( default_style );
#if defined(LOCAL_DEBUG) && !defined(NO_DEBUG_OUTPUT)
  PrintASMountConfig (Config);
  Print_balloonConfig ( Config->asmodule_config.balloon_configs[0]);
#endif
  balloon_config2look( &(Scr.Look), NULL, Config->asmodule_config.balloon_configs[0], "*ASMountBalloon" );
  set_balloon_look( Scr.Look.balloon_look );
}

/******************************************************************************/
/* Initial setup of volume management                                         */
/******************************************************************************/
void reloadButtons ()
{
	int i;
	static char *button_imgs[BUTTONS_NUM][2] = {{"mount-white", "mount-green-pressed"},
																							{"unmount-white", "unmount-red-pressed"},
																							{"eject-white", "eject-red-pressed"}};
	static int button_contexts[BUTTONS_NUM] = { MOUNT_CONTEXT, UNMOUNT_CONTEXT, EJECT_CONTEXT};																							
	int btn_size = (AppState.tileWidth - 15) / 2;

	for ( i = 0 ; i < BUTTONS_NUM ; ++i){
		if (AppState.buttons[i] == NULL)	
			AppState.buttons[i] = safecalloc (1, sizeof(struct button_t));
		else
			free_button_resources (AppState.buttons[i]);

		load_button (AppState.buttons[i], button_imgs[i], Scr.image_manager );
		AppState.buttons[i]->context = button_contexts[i];
		LOCAL_DEBUG_OUT ("button[%d].size = %dx%d, image %dx%d", i,
										AppState.buttons[i]->width, AppState.buttons[i]->height, 
	                  AppState.buttons[i]->pressed.image?AppState.buttons[i]->pressed.image->width:0, 
										AppState.buttons[i]->pressed.image?AppState.buttons[i]->pressed.image->height:0);
		scale_button (AppState.buttons[i], btn_size, btn_size, Scr.asv);
	}
}

void init_ASMount(ASFlagType flags, const char *cmd)
{
	memset( &AppState, 0x00, sizeof(AppState));
	AppState.flags = flags ;
	AppState.tileWidth = DEFAULT_TILE_WIDTH;
	AppState.tileHeight = DEFAULT_TILE_HEIGHT;

	createMainWindow();

	reloadButtons();
	AppState.volumes = create_asbidirlist (ASVolume_destroy);

	g_type_init();
	GVolumeMonitor * monitor  = g_volume_monitor_get();

	g_signal_connect_object (monitor, "mount-added",    G_CALLBACK (mount_added), NULL, 0);
  g_signal_connect_object (monitor, "mount-changed",  G_CALLBACK (mount_changed), NULL, 0);
  g_signal_connect_object (monitor, "mount-removed",  G_CALLBACK (mount_removed), NULL, 0);
  g_signal_connect_object (monitor, "volume-added",   G_CALLBACK (volume_added), NULL, 0);
  g_signal_connect_object (monitor, "volume-changed", G_CALLBACK (volume_changed), NULL, 0);
  g_signal_connect_object (monitor, "volume-removed", G_CALLBACK (volume_removed), NULL, 0);

	GList *tmp;
	GList *list = g_volume_monitor_get_volumes(G_VOLUME_MONITOR(monitor));

  for (tmp = list; tmp != NULL; tmp = tmp->next) {
		ASVolume *v = ASVolume_newGVolume (tmp->data);
		if (v)
			append_bidirelem (AppState.volumes, v);
		else 
			g_object_unref (tmp->data);
	}
  g_list_free (list);

  list = g_volume_monitor_get_mounts(G_VOLUME_MONITOR(monitor));
  for (tmp = list; tmp != NULL; tmp = tmp->next) {
		ASVolume *v = ASVolume_newGMount (tmp->data);
		if (v)
			append_bidirelem (AppState.volumes, v);
		else 
			g_object_unref (tmp->data);
	}
  g_list_free (list);

	AppState.volumeMonitor = monitor;	
	
	redecorateVolumes ();	
}	 



/******************************************************************************/
/* Layout and window creation 																								*/
/******************************************************************************/
typedef struct {
	Bool vertical;
	int tileWidth, tileHeight;

	int currPos;
} ASVolumeCanvasPlacement;


void updateVolumeContents (ASVolume *v){
	int align = Config->Align, h_spacing = 1, v_spacing = 1;
	ASFlagType context_mask = 0;
	if (v->contents == NULL ) {
    v->contents = create_astbar ();
    v->contents->context = C_TITLE ;
  } else /* delete label if it was previously created : */
	  delete_astbar_tile (v->contents, -1 );

	if (!ASVolume_isRequestPending(v)) {
		if (ASVolume_isEjectable(v))
			set_flags (context_mask, EJECT_CONTEXT);
		if (ASVolume_isMounted(v))
			set_flags (context_mask, UNMOUNT_CONTEXT);
		else
			set_flags (context_mask, MOUNT_CONTEXT);
	}
	
	LOCAL_DEBUG_OUT ("volume %s context mask 0x%lx", v->name, context_mask);
  set_astbar_style_ptr (v->contents, -1, Scr.Look.MSWindow[ASVolume_isMounted(v)?BACK_UNFOCUSED:BACK_FOCUSED] );
  set_astbar_hilite (v->contents, BAR_STATE_FOCUSED, Config->MountedBevel);
  set_astbar_hilite (v->contents, BAR_STATE_UNFOCUSED, Config->UnmountedBevel);
  set_astbar_composition_method(v->contents, BAR_STATE_FOCUSED, TEXTURE_TRANSPIXMAP_ALPHA);
  set_astbar_composition_method(v->contents, BAR_STATE_UNFOCUSED, TEXTURE_TRANSPIXMAP_ALPHA);

	if (v->iconIm){

		safe_asimage_destroy (v->iconImScaled);
#if 0		
		v->iconImScaled = scale_asimage (Scr.asv, v->iconIm, AppState.tileWidth-5, AppState.tileHeight-5-(AppState.buttons[0]->height + 4), ASA_ASImage, 100, ASIMAGE_QUALITY_DEFAULT);
		add_astbar_icon(v->contents, 0, 0, False, 0, v->iconImScaled);
#else		
		v->iconImScaled = scale_asimage (Scr.asv, v->iconIm, AppState.tileWidth*2/3, (AppState.tileHeight-(AppState.buttons[0]->height + 4)), ASA_ASImage, 100, ASIMAGE_QUALITY_DEFAULT);
		add_astbar_icon(v->contents, 0, 0, False, ALIGN_CENTER, v->iconImScaled);
#endif		
	}
  add_astbar_label (v->contents, 0, 0, False, align|AS_TilePadRight, h_spacing, v_spacing, v->name, AS_Text_UTF8 );

	LOCAL_DEBUG_OUT ("mount_button.width = %d, mount_button.height = %d, image %dx%d", 
										AppState.buttons[0]->width, AppState.buttons[0]->height, 
	                  AppState.buttons[0]->pressed.image->width, AppState.buttons[0]->pressed.image->height);
	add_astbar_btnblock (v->contents, 0, 2, True, align, AppState.buttons, context_mask, BUTTONS_NUM, 2, 2, 5, 0);

	if (get_flags(Config->flags, ASMOUNT_ShowHints)) {
		char* hint = ASVolume_toString (v);
		set_astbar_balloon (v->contents, C_TITLE, hint, AS_Text_UTF8);	
		safefree(hint);
		set_astbar_balloon (v->contents, MOUNT_CONTEXT, "Mount volume", AS_Text_UTF8);	
		set_astbar_balloon (v->contents, EJECT_CONTEXT, "Eject drive", AS_Text_UTF8);	
		set_astbar_balloon (v->contents, UNMOUNT_CONTEXT, "Un-mount volume", AS_Text_UTF8);	
	}
}

Bool redecorateVolume(void *data, void *aux_data) {
  ASVolume *v = (ASVolume*)data;
	ASVolumeCanvasPlacement *placement = (ASVolumeCanvasPlacement*) aux_data;
	
	int width = placement->tileWidth;
	int height = placement->tileHeight;
	int x = 0, y = 0;
	if (placement->vertical) {
		y = placement->currPos; 
		placement->currPos += height;
	} else {
		x = placement->currPos; 
		placement->currPos += width;
	}		
	
	if (v) {
		XSetWindowAttributes attr;
//	  ARGB2PIXEL(Scr.asv,Config->border_color_argb,&(attr.border_pixel));
    if( v->canvas == NULL ) {
   		Window w;
      attr.event_mask = EnterWindowMask|PointerMotionMask|LeaveWindowMask|StructureNotifyMask|ButtonReleaseMask|ButtonPressMask ;

      w = create_visual_window(Scr.asv, AppState.mainWindow, x, y, width, height, 0, InputOutput, CWEventMask|CWBorderPixel, &attr );
      v->canvas = create_ascanvas( w );
      show_progress("creating volume canvas (%p) for \"%s\"", v->canvas, v->name);
      handle_canvas_config( v->canvas );
    } else {
			XSetWindowBorder( dpy, v->canvas->w, attr.border_pixel );
			moveresize_canvas (v->canvas, x, y, width, height);
		}

		updateVolumeContents (v);		
    set_astbar_size (v->contents, width, height);
		/* redraw will happen on ConfigureNotify */
#if 0
    render_astbar (v->contents, v->canvas );
    update_canvas_display (v->canvas );
#endif		
	}

	return True;
}

void redecorateVolumes() {
	ASVolumeCanvasPlacement placement;
	int width, height;
	
	placement.vertical = get_flags(Config->flags, ASMOUNT_Vertical);
	placement.tileWidth = DEFAULT_TILE_WIDTH;
	placement.tileHeight = DEFAULT_TILE_HEIGHT;
  placement.currPos = 0;
	
	iterate_asbidirlist (AppState.volumes, redecorateVolume, &placement, NULL, False);

	XMapSubwindows (dpy, AppState.mainCanvas->w);

  width = placement.tileWidth;
  height = placement.tileHeight;
	
	if (placement.vertical)
		height = placement.currPos;
	else 
		width = placement.currPos;

	setHints (width, height);
	/* setHints must happen first */
	show_progress ( "resizing main canvas to %dx%d", width, height);	
	resize_canvas (AppState.mainCanvas, width, height);
	ASSync (False);
}

void setHints (int maxWidth, int maxHeight)
{
	XSizeHints    shints;
	ExtendedWMHints extwm_hints ;

//  shints.min_width = 32;
//  shints.min_height = 32;
  shints.max_width = maxWidth;
  shints.max_height = maxHeight;
  shints.width_inc = 1;
  shints.height_inc = 1;
	
  shints.flags = PSize|PPosition|PMaxSize|PResizeInc;//|PWinGravity;

	extwm_hints.pid = getpid();
  extwm_hints.flags = EXTWM_PID|EXTWM_StateSet|EXTWM_TypeSet ;
	extwm_hints.type_flags = EXTWM_TypeNormal ;
	extwm_hints.state_flags = EXTWM_StateSkipTaskbar|EXTWM_StateSkipPager ;

	set_client_hints( AppState.mainWindow, NULL, &shints, AS_DoesWmDeleteWindow, &extwm_hints );
}

void createMainWindow()
{
  XSetWindowAttributes attr;
	int width = DEFAULT_TILE_WIDTH;
	int height = DEFAULT_TILE_HEIGHT;
	
	attr.event_mask = StructureNotifyMask|ButtonPressMask|ButtonReleaseMask ;
  AppState.mainWindow = create_visual_window( Scr.asv, Scr.Root, 0, 0, width, height, 0, InputOutput, CWEventMask, &attr);
  set_client_names( AppState.mainWindow, MyName, MyName, CLASS_ASMOUNT, CLASS_ASMOUNT );

	setHints (width, height);
	set_client_cmd (AppState.mainWindow);

	/* showing window to let user see that we are doing something */
	XMapRaised (dpy, AppState.mainWindow);
//	{ XEvent e;  do {  XNextEvent(dpy, &e); } while (e.type != MapNotify); }

	AppState.mainCanvas = create_ascanvas (AppState.mainWindow);
}

/******************************************************************************/
/*============================================================================*/
/* MAIN MAIN MAIN MAIN MAIN MAIN MAIN MAIN MAIN MAIN MAIN MAIN MAIN MAIN MAIN */
/******************************************************************************/
int
main (int argc, char *argv[])
{
	ASFlagType flags = 0 ; 
	int i;
	char * initial_command = NULL ;
	static char *deleted_arg = "_deleted_arg_" ;

	memset( &AppState, 0x00, sizeof(AppState));

	InitMyApp (CLASS_ASMOUNT, argc, argv, NULL, NULL, 0 );
	for( i = 1 ; i < argc ; ++i ) 
		if( argv[i] == NULL ) 
			argv[i] = strdup(deleted_arg) ;
 	LinkAfterStepConfig();
 	InitSession();

	g_type_init();
#if 0
	ConnectXDisplay (gdk_x11_display_get_xdisplay(gdk_display_open(NULL)), NULL, False);
#else	
	ConnectX( ASDefaultScr, EnterWindowMask );
#endif
	
	ReloadASEnvironment( NULL, NULL, NULL, False, True );

	ConnectAfterStep(0,0);
	Config = AS_ASMOUNT_CONFIG(parse_asmodule_config_all( getASMountConfigClass() )/*asm_config*/);
	CheckASMountConfigSanity(Config, &TileSize);

	SetASMountLook();
	
	init_ASMount( flags, initial_command);
	mainLoop();	
 	return 0;
}

/******************************************************************************/
/* EXIT EXIT EXIT EXIT EXIT EXIT EXIT EXIT EXIT EXIT EXIT EXIT EXIT EXIT EXIT */
/******************************************************************************/
void
DeadPipe (int nonsense)
{
	static int already_dead = False ; 

	if( already_dead ) 
		return;
	already_dead = True ;

	g_signal_handlers_disconnect_by_func (AppState.volumeMonitor, G_CALLBACK (mount_added), NULL);
  g_signal_handlers_disconnect_by_func (AppState.volumeMonitor, G_CALLBACK (mount_changed), NULL);
  g_signal_handlers_disconnect_by_func (AppState.volumeMonitor, G_CALLBACK (mount_removed), NULL);
  g_signal_handlers_disconnect_by_func (AppState.volumeMonitor, G_CALLBACK (volume_added), NULL);
  g_signal_handlers_disconnect_by_func (AppState.volumeMonitor, G_CALLBACK (volume_changed), NULL);
  g_signal_handlers_disconnect_by_func (AppState.volumeMonitor, G_CALLBACK (volume_removed), NULL);
	
	destroy_asbidirlist ( &(AppState.volumes));

	free_button_resources (AppState.buttons[0]);
	free_button_resources (AppState.buttons[1]);

	
/*	window_data_cleanup();
    destroy_ascanvas( &PagerState.main_canvas );
    destroy_ascanvas( &PagerState.icon_canvas );
    
    if( Config )
        DestroyPagerConfig (Config);
*/
    FreeMyAppResources();
#ifdef DEBUG_ALLOCS
	print_unfreed_mem ();
#endif /* DEBUG_ALLOCS */

    XFlush (dpy);
    XCloseDisplay (dpy);
    exit (0);
}

