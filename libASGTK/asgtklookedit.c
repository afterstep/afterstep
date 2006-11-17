/* 
 * Copyright (C) 2006 Sasha Vasko <sasha at aftercode.net>
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

#define LOCAL_DEBUG
#include "../configure.h"

#include "../include/afterbase.h"
#include "../libAfterImage/afterimage.h"
#include "../libAfterStep/asapp.h"
#include "../libAfterStep/screen.h"
#include "../libAfterStep/parser.h"
#include "../libAfterStep/freestor.h"
#include "../libAfterConf/afterconf.h"

#include <unistd.h>		   

#include "asgtk.h"
#include "asgtkai.h"
#include "asgtklookedit.h"
#include "asgtkcframe.h"

/*  local function prototypes  */
static void asgtk_look_edit_class_init (ASGtkLookEditClass *klass);
static void asgtk_look_edit_init (ASGtkLookEdit *iv);
static void asgtk_look_edit_dispose (GObject *object);
static void asgtk_look_edit_finalize (GObject *object);
static void asgtk_look_edit_style_set (GtkWidget *widget, GtkStyle  *prev_style);


/*  private variables  */
static GtkVBoxClass *parent_class = NULL;

GType
asgtk_look_edit_get_type (void)
{
  	static GType id_type = 0;

  	if (! id_type)
    {
    	static const GTypeInfo id_info =
      	{
        	sizeof (ASGtkLookEditClass),
        	(GBaseInitFunc)     NULL,
        	(GBaseFinalizeFunc) NULL,
			(GClassInitFunc)    asgtk_look_edit_class_init,
        	NULL,           /* class_finalize */
        	NULL,           /* class_data     */
        	sizeof (ASGtkLookEdit),
        	0,              /* n_preallocs    */
        	(GInstanceInitFunc) asgtk_look_edit_init,
      	};

      	id_type = g_type_register_static (	GTK_TYPE_VBOX,
        	                                "ASGtkLookEdit",
            	                            &id_info, 0);
    }

  	return id_type;
}

static void
asgtk_look_edit_class_init (ASGtkLookEditClass *klass)
{
  	GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  	parent_class = g_type_class_peek_parent (klass);

  	object_class->dispose   = asgtk_look_edit_dispose;
  	object_class->finalize  = asgtk_look_edit_finalize;

  	widget_class->style_set = asgtk_look_edit_style_set;

}

static void
asgtk_look_edit_init (ASGtkLookEdit *self)
{
	self->configfilename = NULL ;
}

static void
asgtk_look_edit_dispose (GObject *object)
{
  	ASGtkLookEdit *self = ASGTK_LOOK_EDIT (object);
	destroy_string( &(self->configfilename) ); 
  	G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
asgtk_look_edit_finalize (GObject *object)
{
  	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
asgtk_look_edit_style_set (GtkWidget *widget,
                              GtkStyle  *prev_style)
{
  /* ASGtkImageDir *id = ASGTK_IMAGE_DIR (widget); */

  GTK_WIDGET_CLASS (parent_class)->style_set (widget, prev_style);
}


/*  public functions  */
GtkWidget *
asgtk_look_edit_new (const char *myname, struct SyntaxDef *syntax)
{
	ASGtkLookEdit *self = g_object_new( ASGTK_TYPE_LOOK_EDIT, NULL );
	
	
	self->myname = mystrdup(myname? myname:MyName) ;
	self->syntax = syntax? syntax:&LookSyntax ;
	
	self->mystyles_frame = GTK_WIDGET(asgtk_collapsing_frame_new("MyStyles - basic drawing styles",NULL));
	self->myframes_frame = GTK_WIDGET(asgtk_collapsing_frame_new("MyFrames - window frame config",NULL)) ;
	self->balloons_frame = GTK_WIDGET(asgtk_collapsing_frame_new("Balloons",NULL)) ; 		
	self->buttons_frame = GTK_WIDGET(asgtk_collapsing_frame_new("Titlebar Buttons",NULL)) ;	
	self->backgrounds_frame = GTK_WIDGET(asgtk_collapsing_frame_new("Root backgrounds config",NULL));
	self->look_frame = GTK_WIDGET(asgtk_collapsing_frame_new("Main Look config",NULL)) ;

	gtk_box_pack_start (GTK_BOX (self), self->mystyles_frame, TRUE, TRUE, 5);
	gtk_box_pack_start (GTK_BOX (self), self->myframes_frame, TRUE, TRUE, 5);
	gtk_box_pack_start (GTK_BOX (self), self->balloons_frame, TRUE, TRUE, 5);
	gtk_box_pack_start (GTK_BOX (self), self->buttons_frame, TRUE, TRUE, 5);
	gtk_box_pack_start (GTK_BOX (self), self->backgrounds_frame, TRUE, TRUE, 5);
	gtk_box_pack_start (GTK_BOX (self), self->look_frame, TRUE, TRUE, 5);
	
	
	colorize_gtk_tree_view_window( GTK_WIDGET(self) );
  	gtk_widget_show_all (GTK_WIDGET(self));

	LOCAL_DEBUG_OUT( "created image ASGtkLookEdit object %p", self );	
	return GTK_WIDGET (self);
}


void  
asgtk_look_edit_set_configfile( ASGtkLookEdit *self, char *fulldirname )
{
	g_return_if_fail (ASGTK_IS_LOOK_EDIT (self));
	
	if( self->configfilename == NULL && fulldirname == NULL ) 
		return;
	if( self->configfilename && fulldirname && strcmp(self->configfilename, fulldirname)== 0  ) 
		return;
	if( self->configfilename  ) 
		free( self->configfilename );
	
	self->configfilename = fulldirname?mystrdup(fulldirname):NULL;
	
	asgtk_look_edit_reload( self );		 
}	 

void 
asgtk_look_edit_reload( ASGtkLookEdit *self )
{
	g_return_if_fail (ASGTK_IS_LOOK_EDIT (self));
	
	if( self->free_store != NULL ) 
 		DestroyFreeStorage (&(self->free_store));

	if( self->configfilename != NULL ) 
	{
		ConfigData    cd ;
		ConfigDef    *ConfigReader;

		cd.filename = self->configfilename ;
		ConfigReader = InitConfigReader (self->myname, self->syntax, CDT_Filename, cd, NULL);
		if ( ConfigReader != NULL )
		{
			/* set_flags( ConfigReader->flags, parser_flags ); */
			ParseConfig (ConfigReader, &(self->free_store));
			DestroyConfig (ConfigReader);
    	    show_progress("configuration loaded from \"%s\" ...", cd.filename);
		}
	}	
}

#ifdef LOOK_EDITOR_APP
void
on_destroy(GtkWidget *widget, gpointer user_data)
{
	gtk_main_quit();
}


int
main (int argc, char *argv[])
{
	GtkWidget *main_window ; 
	GtkWidget *look_edit ;
	init_asgtkapp( argc, argv, "AS_LOOK_EDITOR", NULL, 0);
	ConnectAfterStep(0,0);

	main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	look_edit = asgtk_look_edit_new (NULL, NULL);
	gtk_container_add (GTK_CONTAINER (main_window), look_edit);

	if( argc > 1 ) 
		asgtk_look_edit_set_configfile( ASGTK_LOOK_EDIT(look_edit), PutHome(argv[1]) );
	
	g_signal_connect (G_OBJECT (main_window), "destroy", G_CALLBACK (on_destroy), NULL);
  	gtk_widget_show (main_window);

  gtk_main ();
  return 0;
}

#endif
