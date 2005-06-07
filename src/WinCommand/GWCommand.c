/*
 * Copyright (C) 2005 Fabian Yamaguchi <fabiany at gmx.net>
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
 */

#include "../../configure.h"
#include "../../libAfterStep/asapp.h"
#include "../../libAfterStep/module.h"
#include "../../libAfterConf/afterconf.h"


#include <gdk/gdkx.h>

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define COMMAND "WinCommand"

struct gui
{
	GtkWidget *dialog;
	GtkWidget *dialog_vbox;
	GtkWidget *vbox;
	GtkWidget *entry;
	GtkWidget *label;
	GtkWidget *dialog_action_area;
}gw_gui;


/* Prototypes */
char *join_with_whitespace(int nelems, char **ar);
void on_entry_activate ( GtkEntry *entry, gpointer data );
void init_gui(char *params, char **argv);
void usage(void);

char *params;

/* Returns a pointer to a string consisting 
 * of nelems elements from ar seperated by whitespace
 */

char *
join_with_whitespace(int nelems, char **ar)
{
	int i;
	int size = 0;
	char *res;
	
	/* Calculate size */
	for ( i = 0; i < nelems; ++i)
	{
		if(ar[i] == NULL)
			continue;
		/* We add an additional char for whitespace/'\0'*/
		size += strlen(ar[i]) + 1;
	}

	/* Allocate memory for buffer */
	res = (char *) malloc( sizeof(char) * size);
	if(res == NULL)
		return NULL;
	
	/* Join strings. */
	for( i = 0; i < nelems; ++i)
	{
		if(ar[i] == NULL)
			continue;
		strcat(res, ar[i]);
		if(i+1 != nelems)
			strcat(res , " ");
	}

	return res;
	
}

void
on_entry_activate ( GtkEntry *entry,
		    gpointer data )
{
	FILE *res;
	char res_buf[1000];
	char com[1000];

	int chars_read;

	const gchar * pattern = gtk_entry_get_text( entry );

	
	/* Launch the program */
	snprintf(com, 1000 - 1 , "%s %s -pattern %s", COMMAND, params, pattern);
	
	fprintf( stderr, com);
	if ( ( res = popen ( com, "r")) == NULL)
		return;
	
	while ( (chars_read = fread(res_buf, 1, 1000 - 1, res)) > 0 )
	{
		res_buf[chars_read - 1] = '\0';
		fprintf(stdout, res_buf);
	}
	
	
	gtk_main_quit();
}

void
init_gui(char *params, char **argv)
{
	struct gui *g = &gw_gui;
	g->dialog = gtk_dialog_new();
	
	gtk_window_set_title (GTK_WINDOW (g->dialog), "GWCommand");
	gtk_window_set_modal (GTK_WINDOW (g->dialog), TRUE);
	gtk_window_set_skip_taskbar_hint (GTK_WINDOW (g->dialog), TRUE);
	gtk_window_set_type_hint (GTK_WINDOW (g->dialog), GDK_WINDOW_TYPE_HINT_DIALOG);

	g->dialog_vbox = GTK_DIALOG (g->dialog)->vbox;
	gtk_widget_show (g->dialog_vbox);

	g->vbox = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (g->vbox);
	gtk_box_pack_start (GTK_BOX (g->dialog_vbox), g->vbox, FALSE, FALSE, 0);

	g->label = gtk_label_new (params);
	gtk_widget_show (g->label);
	gtk_box_pack_start (GTK_BOX (g->vbox), g->label, FALSE, FALSE, 0);

	g->entry = gtk_entry_new ();
	gtk_widget_show (g->entry);
	gtk_box_pack_start (GTK_BOX (g->vbox), g->entry, FALSE, FALSE, 0);

	g->dialog_action_area = GTK_DIALOG (g->dialog)->action_area;
	gtk_widget_show (g->dialog_action_area);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (g->dialog_action_area), GTK_BUTTONBOX_END);

	
	/* Callbacks */
	g_signal_connect ((gpointer) g->entry, "activate",
			  G_CALLBACK (on_entry_activate),
			  argv);
	
	gtk_widget_show(g->dialog);

}

void
usage(void)
{
	g_print("Supply at least one argument\n");
}


int main(int argc, char **argv)
{
	GdkDisplay *gdk_display ;
	int i ; 
	static char *deleted_arg = "_deleted_arg_" ;

  	InitMyApp (CLASS_ASCP, argc, argv, NULL, NULL, 0 );
	for( i = 1 ; i < argc ; ++i ) 
		if( argv[i] == NULL ) 
			argv[i] = strdup(deleted_arg) ;
  	LinkAfterStepConfig();
  	InitSession();
	
	
	if( argc < 2)
	{
		usage();
		return -1;
	}
	
	params = join_with_whitespace(argc - 1, argv + 1);
	
	
	gtk_init( &argc, &argv);
	gdk_display = gdk_display_get_default();
	
	LoadColorScheme();
	ConnectXDisplay (gdk_x11_display_get_xdisplay(gdk_display), NULL, False);
	ReloadASEnvironment( NULL, NULL, NULL, False, True );
    
	ConnectAfterStep (WINDOW_CONFIG_MASK |
                      WINDOW_NAME_MASK |
                      M_END_WINDOWLIST|
				      M_NEW_DESKVIEWPORT, 0);
	
	init_gui( params, argv);
	gtk_main();
	
	free(params);
	return 0;
}
