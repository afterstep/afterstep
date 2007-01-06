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
#include "asgtklistviews.h"
#include "asgtklookedit.h"
#include "asgtkcframe.h"

#define MYSTYLE_LIST_WIDTH   	100
#define MYSTYLE_LIST_HEIGHT  	200
#define INHERIT_LIST_WIDTH   	MYSTYLE_LIST_WIDTH
#define INHERIT_LIST_HEIGHT  	(MYSTYLE_LIST_HEIGHT>>2)
#define PREVIEW_WIDTH  			240
#define PREVIEW_HEIGHT 			180

/*****************************************************************************/
/*   MyStyle Editor                                                          */
/*****************************************************************************/
/*  local function prototypes  */
static void asgtk_mystyle_edit_class_init (ASGtkMyStyleEditClass *klass);
static void asgtk_mystyle_edit_init (ASGtkMyStyleEdit *self);
static void asgtk_mystyle_edit_dispose (GObject *object);
static void asgtk_mystyle_edit_finalize (GObject *object);
static void asgtk_mystyle_edit_style_set (GtkWidget *widget, GtkStyle  *prev_style);


/*  private variables  */
static GtkFrame *mystyle_edit_parent_class = NULL;

GType
asgtk_mystyle_edit_get_type (void)
{
  	static GType id_type = 0;

  	if (! id_type)
    {
    	static const GTypeInfo id_info =
      	{
        	sizeof (ASGtkMyStyleEditClass),
        	(GBaseInitFunc)     NULL,
        	(GBaseFinalizeFunc) NULL,
			(GClassInitFunc)    asgtk_mystyle_edit_class_init,
        	NULL,           /* class_finalize */
        	NULL,           /* class_data     */
        	sizeof (ASGtkMyStyleEdit),
        	0,              /* n_preallocs    */
        	(GInstanceInitFunc) asgtk_mystyle_edit_init,
      	};

      	id_type = g_type_register_static (	GTK_TYPE_FRAME,
        	                                "ASGtkMyStyleEdit",
            	                            &id_info, 0);
    }

  	return id_type;
}

static void
asgtk_mystyle_edit_class_init (ASGtkMyStyleEditClass *klass)
{
  	GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  	mystyle_edit_parent_class = g_type_class_peek_parent (klass);

  	object_class->dispose   = asgtk_mystyle_edit_dispose;
  	object_class->finalize  = asgtk_mystyle_edit_finalize;

  	widget_class->style_set = asgtk_mystyle_edit_style_set;

}

static void
asgtk_mystyle_edit_init (ASGtkMyStyleEdit *self)
{
	self->free_store = NULL ;
	self->syntax = NULL;
}

static void
asgtk_mystyle_edit_dispose (GObject *object)
{
  	ASGtkMyStyleEdit *self = ASGTK_MYSTYLE_EDIT (object);
  	G_OBJECT_CLASS (mystyle_edit_parent_class)->dispose (object);
}

static void
asgtk_mystyle_edit_finalize (GObject *object)
{
  	G_OBJECT_CLASS (mystyle_edit_parent_class)->finalize (object);
}

static void
asgtk_mystyle_edit_style_set (GtkWidget *widget,
                              GtkStyle  *prev_style)
{
  /* ASGtkImageDir *id = ASGTK_IMAGE_DIR (widget); */

  GTK_WIDGET_CLASS (mystyle_edit_parent_class)->style_set (widget, prev_style);
}

static void
on_add_inherit_mystyle_btn_clicked(GtkButton *button, gpointer user_data)
{
	ASGtkLookEdit *self = ASGTK_LOOK_EDIT(user_data);
}

const char *MyStyleTextStyles[AST_3DTypes+2] = 
		{	"0 - normal text",
	      	"1 - embossed 3D text",
            "2 - sunken 3D text",
            "3 - text has shade above the text",
            "4 - text has shade below the text",
            "5 - thick embossed 3D text",
            "6 - thick sunken 3D text",
            "7 - outlined on upper edge of a glyph",
            "8 - outlined on bottom edge of a glyph",
            "9 - outlined all around a glyph",
            "unset - use default",
			NULL };

static void fill_text_style_combo_box( GtkWidget *w ) 
{
	int i = 0 ;
	GtkComboBox *cbox = GTK_COMBO_BOX(w);
	if( cbox )
		while( MyStyleTextStyles[i] ) 
			gtk_combo_box_append_text( cbox, MyStyleTextStyles[i++] );
}

static void 
FreeStorage2MyStyleEdit( FreeStorageElem *storage, ASGtkMyStyleEdit *self ) 
{
	FreeStorageElem *curr = storage ;
	ConfigItem    item;
	ASGtkSimpleList *list = ASGTK_SIMPLE_LIST(self->inherit_list);

	self->free_store = storage ; 

	item.memory = NULL;
	asgtk_simple_list_purge( list );
	gtk_combo_box_set_active( GTK_COMBO_BOX(self->text_style), AST_3DTypes );
	gtk_entry_set_text( GTK_ENTRY(self->font), NULL );
	gtk_entry_set_text( GTK_ENTRY(self->fore_color), NULL );
	gtk_entry_set_text( GTK_ENTRY(self->back_color), NULL );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(self->overlay), FALSE );
	gtk_combo_box_set_active( GTK_COMBO_BOX(self->overlay_mystyle), -1 );
	gtk_widget_set_sensitive( self->overlay_mystyle, FALSE ); 				

	if( curr && curr->term->id == MYSTYLE_START_ID ) 
		curr = curr->sub ;

	if( curr ) 
	{
		while( curr != NULL ) 
		{
			if( ReadConfigItem (&item, curr) ) 
			{
				if( curr->term->id == MYSTYLE_INHERIT_ID ) 
					asgtk_simple_list_append( list, item.data.string, curr );
				else if( curr->term->id == MYSTYLE_FONT_ID ) 
					gtk_entry_set_text( GTK_ENTRY(self->font), item.data.string );
				else if( curr->term->id == MYSTYLE_FORECOLOR_ID ) 
					gtk_entry_set_text( GTK_ENTRY(self->fore_color), item.data.string );
				else if( curr->term->id == MYSTYLE_BACKCOLOR_ID ) 
					gtk_entry_set_text( GTK_ENTRY(self->back_color), item.data.string );
				else if( curr->term->id == MYSTYLE_TEXTSTYLE_ID ) 
				{
					if( item.data.integer > AST_3DTypes ) 
						item.data.integer = AST_3DTypes ;
					gtk_combo_box_set_active( GTK_COMBO_BOX(self->text_style), item.data.integer );
				}else if( curr->term->id == MYSTYLE_Overlay_ID ) 
				{
					GtkTreeIter iter ; 
					gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(self->overlay), TRUE );
					if( gtk_tree_model_get_iter_first( self->mystyles_list, &iter ) )
					{
						char *tmp = NULL ; 
						do
						{
					    	gtk_tree_model_get (self->mystyles_list, &iter, 0, &tmp, -1);
							if( mystrcasecmp( tmp, item.data.string ) == 0 ) 
							{
								gtk_combo_box_set_active_iter( GTK_COMBO_BOX(self->overlay_mystyle), &iter );
								break ;
							}
						}while( gtk_tree_model_iter_next( self->mystyles_list, &iter ) );
					}
					gtk_widget_set_sensitive( self->overlay_mystyle, TRUE ); 				
				}
				item.ok_to_free = True;
			}
			
			curr = curr->next ; 
		}
		ReadConfigItem (&item, NULL);
	}

}

void 
asgtk_mystyle_edit_set_mystyles_list( ASGtkMyStyleEdit *self, GtkTreeModel *list ) 
{
	if( self )
	{
		if( self->mystyles_list ) 
			g_object_unref( self->mystyles_list ); 
		self->mystyles_list = list ; 
		if( self->mystyles_list ) 
			g_object_ref( self->mystyles_list ); 
		gtk_combo_box_set_model( GTK_COMBO_BOX(self->overlay_mystyle), self->mystyles_list );
	}
}

static void 
on_mystyle_overlay_clicked(GtkWidget *widget, gpointer data )
{
  	ASGtkMyStyleEdit *self = ASGTK_MYSTYLE_EDIT (data);
	Bool active = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(widget));
	gtk_widget_set_sensitive( self->overlay_mystyle, active ); 				

}


/*  public functions  */
GtkWidget *
asgtk_mystyle_edit_new ()
{
	ASGtkMyStyleEdit *self = g_object_new( ASGTK_TYPE_MYSTYLE_EDIT, NULL );
	GtkWidget *wself = GTK_WIDGET(self); /* so we don't have to do typecasting a hundred of times */
	GtkWidget *table ; 
	GtkWidget *background_frame ; 

	colorize_gtk_widget( wself, get_colorschemed_style_normal() );

	self->syntax = &MyStyleSyntax ;
	table = gtk_table_new( 4, 10, FALSE );
	self->inherit_frame = gtk_frame_new( NULL );
	self->inherit_vbox = gtk_vbox_new(FALSE, 5);
	self->inherit_list = asgtk_simple_list_new( "Inherited MyStyles : " );
	self->inherit_list_window  = ASGTK_SCROLLED_WINDOW(GTK_POLICY_NEVER, GTK_POLICY_ALWAYS,GTK_SHADOW_IN);
	self->inherit_list_hbtn_box= gtk_hbutton_box_new(); 	

	self->font = gtk_entry_new();
	self->fore_color = gtk_entry_new();
	self->back_color = gtk_entry_new();
	self->text_style = gtk_combo_box_new_text();
	fill_text_style_combo_box( self->text_style ); 
	
	self->overlay = gtk_check_button_new_with_label("Overlay with : ");
	gtk_button_set_relief(GTK_BUTTON(self->overlay), GTK_RELIEF_NONE );
   	g_signal_connect ((gpointer) self->overlay, "clicked", G_CALLBACK (on_mystyle_overlay_clicked), self);
	self->overlay_mystyle = gtk_combo_box_new_text();
	
	background_frame = gtk_frame_new( " Background : " ); 
	
	ASGTK_PACK_BEGIN(self->inherit_vbox);
		ASGTK_PACK_TO_START(self->inherit_list_window, TRUE, TRUE, 0);
		ASGTK_PACK_TO_END(self->inherit_list_hbtn_box, FALSE, FALSE, 0);
	ASGTK_PACK_END;

	ASGTK_PACK_BEGIN(self->inherit_list_hbtn_box);
		self->inherit_list_add_btn 	= asgtk_add_button_to_box( ASGTK_PACK_BOX, GTK_STOCK_ADD, "Add", G_CALLBACK(on_add_inherit_mystyle_btn_clicked), self ); 	
		self->inherit_list_del_btn 	= asgtk_add_button_to_box( ASGTK_PACK_BOX, GTK_STOCK_DELETE, "Delete", G_CALLBACK(on_add_inherit_mystyle_btn_clicked), self ); 	
	ASGTK_PACK_END;

	ASGTK_TABLE_BEGIN(table);
		ASGTK_ROW_BEGIN;
			ASGTK_TABLE_CELL_SPAN(self->inherit_frame,2);
			ASGTK_TABLE_CELL_SPAN2D(background_frame,2,4);
		ASGTK_ROW_END;
		ASGTK_ROW_BEGIN;
			ASGTK_TABLE_CELL(ASGTK_ALIGNED_LABEL("Font : ", 1.0, 0.5));
			ASGTK_TABLE_CELL(self->font);
		ASGTK_ROW_END;
		ASGTK_ROW_BEGIN;
			ASGTK_TABLE_CELL(ASGTK_ALIGNED_LABEL("Text Style : ", 1.0, 0.5));
			ASGTK_TABLE_CELL(self->text_style);
		ASGTK_ROW_END;
		ASGTK_ROW_BEGIN;
			ASGTK_TABLE_CELL(ASGTK_ALIGNED_LABEL("Fore Color : ", 1.0, 0.5));
			ASGTK_TABLE_CELL(self->fore_color);
		ASGTK_ROW_END;
		ASGTK_ROW_BEGIN;
			ASGTK_TABLE_CELL(ASGTK_ALIGNED_LABEL("Back Color : ", 1.0, 0.5));
			ASGTK_TABLE_CELL(self->back_color);
			ASGTK_TABLE_CELL(self->overlay);
			ASGTK_TABLE_CELL(self->overlay_mystyle);
		ASGTK_ROW_END;
	ASGTK_TABLE_END;

	gtk_widget_set_size_request(self->text_style,INHERIT_LIST_WIDTH,0);	

	ASGTK_CONTAINER_CONFIG(self->inherit_list_window, INHERIT_LIST_WIDTH, INHERIT_LIST_HEIGHT,0);
	
	ASGTK_CONTAINER_ADD(self->inherit_list_window, self->inherit_list);
	ASGTK_CONTAINER_ADD(self->inherit_frame, self->inherit_vbox);

	ASGTK_CONTAINER_ADD(self, table);

	LOCAL_DEBUG_OUT( "created image ASGtkMyStyleEdit object %p", self );	
	return wself;
}

/*****************************************************************************/
/*   Look Editor                                                             */
/*****************************************************************************/


/*  local function prototypes  */
static void asgtk_look_edit_class_init (ASGtkLookEditClass *klass);
static void asgtk_look_edit_init (ASGtkLookEdit *iv);
static void asgtk_look_edit_dispose (GObject *object);
static void asgtk_look_edit_finalize (GObject *object);
static void asgtk_look_edit_style_set (GtkWidget *widget, GtkStyle  *prev_style);


/*  private variables  */
static GtkVBoxClass *look_edit_parent_class = NULL;

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

  	look_edit_parent_class = g_type_class_peek_parent (klass);

  	object_class->dispose   = asgtk_look_edit_dispose;
  	object_class->finalize  = asgtk_look_edit_finalize;

  	widget_class->style_set = asgtk_look_edit_style_set;

}

static void
asgtk_look_edit_init (ASGtkLookEdit *self)
{
	self->configfilename = NULL ;
	self->mystyles = safecalloc( 1, sizeof(ASGtkMyStylesPanel) );
}

static void
asgtk_look_edit_dispose (GObject *object)
{
  	ASGtkLookEdit *self = ASGTK_LOOK_EDIT (object);
	destroy_string( &(self->configfilename) ); 
  	G_OBJECT_CLASS (look_edit_parent_class)->dispose (object);
}

static void
asgtk_look_edit_finalize (GObject *object)
{
  	G_OBJECT_CLASS (look_edit_parent_class)->finalize (object);
}

static void
asgtk_look_edit_style_set (GtkWidget *widget,
                              GtkStyle  *prev_style)
{
  /* ASGtkImageDir *id = ASGTK_IMAGE_DIR (widget); */

  GTK_WIDGET_CLASS (look_edit_parent_class)->style_set (widget, prev_style);
}

static void
on_add_mystyle_btn_clicked(GtkButton *button, gpointer user_data)
{
	ASGtkLookEdit *self = ASGTK_LOOK_EDIT(user_data);
}

void mystyle_panel_sel_handler(GObject *selection_owner, gpointer user_data)
{
	ASGtkMyStyleEdit *mystyle_edit = ASGTK_MYSTYLE_EDIT(selection_owner);
	FreeStorageElem *storage = (FreeStorageElem*)user_data ;
	
	if( mystyle_edit == NULL ) 
		return ;
	LOCAL_DEBUG_OUT( "storage = %p, edit = %p", storage, mystyle_edit );
	FreeStorage2MyStyleEdit( storage, mystyle_edit );
}

static void 
build_mystyles_panel( ASGtkMyStylesPanel *panel ) 
{
	panel->frame 		= asgtk_collapsing_frame_new("MyStyles - basic drawing styles",NULL);
	panel->hbox 		= gtk_hbox_new(FALSE, 5);
	panel->list_vbox 	= gtk_vbox_new(FALSE, 5);
	panel->list 		= asgtk_simple_list_new( "Available MyStyles : " );
	panel->list_window  = ASGTK_SCROLLED_WINDOW(GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC,GTK_SHADOW_IN);
	panel->list_hbtn_box= gtk_hbutton_box_new(); 	
	panel->mystyle_editor= asgtk_mystyle_edit_new(); 	
	asgtk_mystyle_edit_set_mystyles_list( ASGTK_MYSTYLE_EDIT(panel->mystyle_editor), ASGTK_SIMPLE_LIST(panel->list)->tree_model ); 

	asgtk_simple_list_set_sel_handling( ASGTK_SIMPLE_LIST(panel->list), G_OBJECT(panel->mystyle_editor), mystyle_panel_sel_handler ); 

	ASGTK_PACK_BEGIN(panel->hbox);
		ASGTK_PACK_TO_START(panel->list_vbox, FALSE, FALSE, 5);
		ASGTK_PACK_TO_START(panel->mystyle_editor, TRUE, TRUE, 0);
	ASGTK_PACK_END;

	ASGTK_PACK_BEGIN(panel->list_vbox);
		ASGTK_PACK_TO_START(panel->list_window, TRUE, TRUE, 0);
		ASGTK_PACK_TO_END(panel->list_hbtn_box, TRUE, FALSE, 0);
	ASGTK_PACK_END;

	ASGTK_PACK_BEGIN(panel->list_hbtn_box);
		panel->list_add_btn 	= asgtk_add_button_to_box( ASGTK_PACK_BOX, GTK_STOCK_ADD, "Add", G_CALLBACK(on_add_mystyle_btn_clicked), panel ); 	
		panel->list_del_btn 	= asgtk_add_button_to_box( ASGTK_PACK_BOX, GTK_STOCK_DELETE, "Delete", G_CALLBACK(on_add_mystyle_btn_clicked), panel ); 	
		panel->list_rename_btn 	= asgtk_add_button_to_box( ASGTK_PACK_BOX, GTK_STOCK_PREFERENCES, "Rename", G_CALLBACK(on_add_mystyle_btn_clicked), panel );
	ASGTK_PACK_END;

	ASGTK_CONTAINER_CONFIG(panel->list_window,MYSTYLE_LIST_WIDTH, MYSTYLE_LIST_HEIGHT,0);

	ASGTK_CONTAINER_ADD(panel->list_window, panel->list);
	ASGTK_CONTAINER_ADD(panel->frame, panel->hbox);
}


static void 
FreeStorage2MyStylesPanel( FreeStorageElem *storage, ASGtkMyStylesPanel *panel ) 
{
	FreeStorageElem *curr = storage ;
	ASGtkSimpleList *list = ASGTK_SIMPLE_LIST(panel->list);
	ConfigItem    item;

	item.memory = NULL;
	
	asgtk_simple_list_purge( list );
	while( curr != NULL ) 
	{
		if( curr->term->id == MYSTYLE_START_ID ) 
		{
			if (ReadConfigItem (&item, curr))
			{
				asgtk_simple_list_append( list, item.data.string, curr );
				item.ok_to_free = True;
			}
		}
		curr = curr->next ; 
	}
	ReadConfigItem (&item, NULL);

}

/*  public functions  */
GtkWidget *
asgtk_look_edit_new (const char *myname, struct SyntaxDef *syntax)
{
	ASGtkLookEdit *self = g_object_new( ASGTK_TYPE_LOOK_EDIT, NULL );

	colorize_gtk_tree_view_window( GTK_WIDGET(self) );
	
	self->myname = mystrdup(myname? myname:MyName) ;
	self->syntax = syntax? syntax:&LookSyntax ;
	
	build_mystyles_panel( self->mystyles ); 


	self->myframes_frame 	= asgtk_collapsing_frame_new("MyFrames - window frame config",NULL);
	self->balloons_frame 	= asgtk_collapsing_frame_new("Balloons",NULL) ; 		
	self->buttons_frame 	= asgtk_collapsing_frame_new("Titlebar Buttons",NULL) ;	
	self->backgrounds_frame = asgtk_collapsing_frame_new("Root backgrounds config",NULL);
	self->look_frame 		= asgtk_collapsing_frame_new("Main Look config",NULL) ;

	ASGTK_PACK_BEGIN(self);
		ASGTK_PACK_TO_START( self->mystyles->frame	, FALSE, FALSE, 5);
		ASGTK_PACK_TO_START( self->myframes_frame	, FALSE, FALSE, 5);
		ASGTK_PACK_TO_START( self->balloons_frame	, FALSE, FALSE, 5);
		ASGTK_PACK_TO_START( self->buttons_frame	, FALSE, FALSE, 5);
		ASGTK_PACK_TO_START( self->backgrounds_frame, FALSE, FALSE, 5);
		ASGTK_PACK_TO_START( self->look_frame		, TRUE , TRUE , 5);
	ASGTK_PACK_END;

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
	
	FreeStorage2MyStylesPanel( self->free_store, self->mystyles );	
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

	asgtk_look_edit_set_configfile( ASGTK_LOOK_EDIT(look_edit), PutHome("~/.afterstep/looks/look.Mine") );
	
	g_signal_connect (G_OBJECT (main_window), "destroy", G_CALLBACK (on_destroy), NULL);
  	gtk_widget_show (main_window);

  gtk_main ();
  return 0;
}

#endif
