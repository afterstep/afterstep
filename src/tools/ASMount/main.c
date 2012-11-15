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
#include "../../../configure.h"
#include "../../../libAfterStep/asapp.h"
#include "../../../libAfterStep/module.h"
#include "../../../libAfterStep/session.h"
#include "../../../libAfterConf/afterconf.h"
#ifdef HAVE_GTK
#include "../../../libASGTK/asgtk.h"
#else
typedef void* GtkWidget;
#endif
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>

#define ENTRY_WIDTH		300


typedef struct ASMountState
{
	ASFlagType flags ;
	
	GtkWidget *main_window ;	   
	
	char *current_dir;
	
}ASMountState;

ASMountState AppState ;

#ifdef HAVE_GTK
void
on_destroy(GtkWidget *widget, gpointer user_data)
{
	gtk_main_quit();
}

#if 0
void
on_exec_clicked(GtkWidget *widget, gpointer user_data)
{
	if( AppState.target_entry )
	{	
		Bool in_term = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(AppState.run_in_term_check) );
		char *text = stripcpy(gtk_entry_get_text(GTK_ENTRY(AppState.target_entry)));

		if( exec_command(&text, in_term?ASRTool_Term:ASRTool_Normal) )
		{
			if( !get_flags( AppState.flags, ASRUN_Persist ) )
				gtk_main_quit();
			else
				asgtk_combo_box_add_to_history( GTK_COMBO_BOX(AppState.target_combo), text );
			
			free( text );
		}
	}
}

#endif

void init_ASMount(ASFlagType flags, const char *cmd )
{
	GtkWidget *main_vbox ;
	GtkWidget *hbox ;
	GtkWidget *frame ;
	GtkWidget *exec_btn, *cancel_btn ;

	memset( &AppState, 0x00, sizeof(AppState));
	AppState.flags = flags ;
return;
	AppState.main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

 	gtk_window_set_title (GTK_WINDOW (AppState.main_window), MyName);
	/*gtk_window_set_resizable( GTK_WINDOW (AppState.main_window), FALSE);*/
	colorize_gtk_widget( AppState.main_window, get_colorschemed_style_normal() );

	frame = gtk_frame_new( NULL );
	gtk_container_add (GTK_CONTAINER (AppState.main_window), frame);
	gtk_container_set_border_width( GTK_CONTAINER (frame), 5 );
	gtk_widget_show(frame);

	main_vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (frame), main_vbox);
	gtk_container_set_border_width( GTK_CONTAINER (main_vbox), 5 );

#if 0	
	/********   Line 1 *******/
	AppState.run_in_term_check = gtk_check_button_new_with_label("Exec in terminal");
	if( AppState.tool == ASRTool_Term )
		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(AppState.run_in_term_check), TRUE );

	hbox = gtk_hbox_new( FALSE, 5 );
	gtk_box_pack_start (GTK_BOX (main_vbox), hbox, TRUE, TRUE, 2);
	gtk_box_pack_start (GTK_BOX (hbox), gtk_label_new("Command to execute:"), FALSE, FALSE, 0);
	gtk_box_pack_end (GTK_BOX (hbox), AppState.run_in_term_check, FALSE, FALSE, 0);
	gtk_widget_show_all (hbox);
	
	/********   Line 2 *******/
	AppState.target_combo = gtk_combo_box_entry_new_text(); 
	gtk_container_set_border_width( GTK_CONTAINER (AppState.target_combo), 1 );
	gtk_widget_set_size_request (AppState.target_combo, ENTRY_WIDTH, -1);
	
	frame = gtk_frame_new( NULL );
	gtk_container_add (GTK_CONTAINER (frame), AppState.target_combo);
	gtk_container_set_border_width( GTK_CONTAINER (frame), 1 );
	gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 5);
	gtk_widget_show_all(frame);
	gtk_widget_show(frame);
	colorize_gtk_edit( AppState.target_combo );
	
	if( GTK_IS_CONTAINER(AppState.target_combo) )
		gtk_container_forall( GTK_CONTAINER(AppState.target_combo), find_combobox_entry, &AppState.target_entry );
	
	/********   Line 3 *******/
	hbox = gtk_hbutton_box_new();
	gtk_box_pack_end (GTK_BOX (main_vbox), hbox, TRUE, TRUE, 5);
	exec_btn = gtk_button_new_from_stock( GTK_STOCK_EXECUTE );
	gtk_box_pack_start (GTK_BOX (hbox), exec_btn, FALSE, FALSE, 0);
	cancel_btn = gtk_button_new_from_stock( GTK_STOCK_CANCEL );
	gtk_box_pack_end (GTK_BOX (hbox), cancel_btn, FALSE, FALSE, 0);
	gtk_widget_show_all (hbox);

	/********   Callbacks *******/
	/* if above succeeded then path_entry should be not NULL here : */
	/* TODO : insert proper change handlers and data pointers here : */
	if( AppState.target_entry ) 
	{	
		gtk_entry_set_has_frame(  GTK_ENTRY(AppState.target_entry), FALSE );
		g_signal_connect ( G_OBJECT (AppState.target_entry), "activate",
		      			   G_CALLBACK (on_exec_clicked), (gpointer) NULL);
		if( cmd )
			gtk_entry_set_text( GTK_ENTRY(AppState.target_entry), cmd );
	}

/*	g_signal_connect (G_OBJECT(AppState.target_combo), "changed",
			  			G_CALLBACK (NULL), (gpointer) NULL); */
	
	g_signal_connect (G_OBJECT (cancel_btn), "clicked", G_CALLBACK (on_destroy), NULL);
	g_signal_connect (G_OBJECT (exec_btn), "clicked", G_CALLBACK (on_exec_clicked), NULL);
	
	g_signal_connect (G_OBJECT (AppState.main_window), "destroy", G_CALLBACK (on_destroy), NULL);
	/********   Show them all *******/
#endif
	gtk_widget_show_all (main_vbox);
	gtk_widget_show(AppState.main_window);
	
//	gtk_window_set_focus( GTK_WINDOW(AppState.main_window), AppState.target_entry );
//	GTK_WIDGET_SET_FLAGS(exec_btn, GTK_CAN_DEFAULT );
//	gtk_window_set_default( GTK_WINDOW(AppState.main_window), exec_btn );
}	 
#endif                         /* HAVE_GTK  */

typedef struct ASVolume {
	GVolume *g_volume;
	

}ASVolume;

ASBiDirList *Volumes;

ASVolume *ASVolume_create (GVolume *g_v)
{
	ASVolume *v;
	
	if (g_v == NULL)
		return NULL;
		
	v = safecalloc (1, sizeof(ASVolume));
	v->g_volume = g_object_ref (g_v);
	return v;
}

void ASVolume_destroy( void *data )
{
	if (data)	{
		ASVolume *v = (ASVolume *) data;
		g_object_unref (v->g_volume);	
		v->g_volume = NULL;
		free (v);
	}
}

char* ASVolume_toString (ASVolume *v)
{
	if (v && v->g_volume)
		return g_volume_get_name(v->g_volume);
	return mystrdup("");
}


void check_drives()
{
	Volumes = create_asbidirlist (ASVolume_destroy);

	GVolumeMonitor * monitor;
    GList * mounts;
    GList * volumes;
		GList *tmp;

    monitor = g_volume_monitor_get();
    mounts = g_volume_monitor_get_mounts(G_VOLUME_MONITOR(monitor));

    volumes = g_volume_monitor_get_volumes(G_VOLUME_MONITOR(monitor));
    for (tmp = volumes; tmp != NULL; tmp = tmp->next) {
			ASVolume *v = ASVolume_create (tmp->data);
			append_bidirelem (Volumes, v);
    	char *volume_name = ASVolume_toString(v);
		  g_print("volume: %s\n", volume_name);
    	safefree(volume_name);
    }
    g_list_free (volumes);

    for (tmp = mounts; tmp != NULL; tmp = tmp->next) {
			GMount *mount = tmp->data;
    	char *mount_name = g_mount_get_name(mount);
		  g_print("mount: %s\n", mount_name);
    	g_free(mount_name);
			g_object_unref (mount);
    }
    g_list_free (mounts);

	destroy_asbidirlist (&Volumes);
}

int
main (int argc, char *argv[])
{
	ASFlagType flags = 0 ; 
	int i;
	char * initial_command = NULL ;

#ifdef HAVE_GTK		   
	init_asgtkapp( argc, argv, CLASS_ASRUN, NULL, 0);
#else
	InitMyApp (CLASS_ASMOUNT, argc, argv, NULL, NULL, 0 );
  	LinkAfterStepConfig();
  	InitSession();
    ConnectX( ASDefaultScr, 0 );
	LoadColorScheme();
	ReloadASEnvironment( NULL, NULL, NULL, False, True );
#endif	
	
	for( i = 1 ; i < argc ; ++i ) 
	{	
		if( argv[i] == NULL ) 
			continue;

	}
	
	ConnectAfterStep(0,0);
	
#ifdef HAVE_GTK		
	init_ASMount( flags, initial_command );
	
	check_drives();
	
	gtk_main ();
#endif
 	return 0;
}

