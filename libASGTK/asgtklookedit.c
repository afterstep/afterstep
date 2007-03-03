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
static GtkVBox *mystyle_edit_parent_class = NULL;

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

      	id_type = g_type_register_static (	GTK_TYPE_VBOX,
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
            "3 - shade above the text",
            "4 - shade below the text",
            "5 - thick embossed 3D text",
            "6 - thick sunken 3D text",
            "7 - outlined on upper edge",
            "8 - outlined on bottom edge",
            "9 - outlined all around",
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

	self->free_store = storage ; 
#if 0
	ASGtkSimpleList *list = ASGTK_SIMPLE_LIST(self->inherit_list);
	item.memory = NULL;
	asgtk_simple_list_purge( list );
	gtk_combo_box_set_active( GTK_COMBO_BOX(self->text_style), AST_3DTypes );
	gtk_entry_set_text( GTK_ENTRY(self->font), NULL );
	gtk_entry_set_text( GTK_ENTRY(self->fore_color), NULL );
	gtk_entry_set_text( GTK_ENTRY(self->back_color), NULL );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(self->overlay), FALSE );
	gtk_combo_box_set_active( GTK_COMBO_BOX(self->overlay_mystyle), -1 );
	gtk_widget_set_sensitive( self->overlay_mystyle, FALSE ); 				
//	asgtk_collapsing_frame_set_open(ASGTK_COLLAPSING_FRAME(self->backpixmap_frame), FALSE);

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
				}else if( curr->term->id == MYSTYLE_BACKPIXMAP_ID ) 
				{
					LOCAL_DEBUG_OUT( "item.data.string = \"%s\", index = %d", item.data.string, item.index );
					if( item.data.string != NULL )
						gtk_entry_set_text( GTK_ENTRY(self->pixmap_filename), item.data.string );
					asgtk_collapsing_frame_set_open(ASGTK_COLLAPSING_FRAME(self->backpixmap_frame), TRUE);
				}
				item.ok_to_free = True;
			}
			
			curr = curr->next ; 
		}
		ReadConfigItem (&item, NULL);
	}
#endif
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
			
		gtk_combo_box_set_model( GTK_COMBO_BOX(self->combo_overlay_mystyle), self->mystyles_list );
	}
}

static void 
on_mystyle_overlay_clicked(GtkWidget *widget, gpointer data )
{
  	ASGtkMyStyleEdit *self = ASGTK_MYSTYLE_EDIT (data);
	Bool active = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(widget));
#if 0
	gtk_widget_set_sensitive( self->overlay_mystyle, active ); 				
#endif

}

void 
asgtk_mystyle_edit_set_name_visible(ASGtkMyStyleEdit *self, Bool visible)
{
	if( !visible ) 
	{
		if( get_flags( self->flags, ASGTK_MYSTYLE_EDIT_NAME_VISIBLE ) )
		{
		    gtk_container_remove (GTK_CONTAINER (self), self->hbox1_mystyle_name);
			clear_flags( self->flags, ASGTK_MYSTYLE_EDIT_NAME_VISIBLE );
		}
	}else
	{
		if( !get_flags( self->flags, ASGTK_MYSTYLE_EDIT_NAME_VISIBLE ) )
		{
		  	gtk_box_pack_start (GTK_BOX(self), self->hbox1_mystyle_name, TRUE, TRUE, 0);
			gtk_box_reorder_child(GTK_BOX(self), self->hbox1_mystyle_name, 0); 	
			set_flags( self->flags, ASGTK_MYSTYLE_EDIT_NAME_VISIBLE );
		}
	}	
}

void 
asgtk_mystyle_edit_set_background_type(ASGtkMyStyleEdit *self, int type)
{
	if( type == 0 ) 
	{
	
	}else if( type > 0 && type <= TEXTURE_GRADIENT_END ) 
	{
		/* to show : hbox8_grad_details	*/
	}else if( type >= TEXTURE_TEXTURED_START && type <= TEXTURE_TEXTURED_END )  
	{
		/* to show : hbox10_texture_blend_type, hbox9_texture_file, table1_texture_slicing	*/
	}else
	{
	
	}	
}


GtkWidget*
create_mystyle_editor_interface (GtkWidget *mystyle_editor /* assumed descendand from vbox */);


/*  public functions  */
GtkWidget *
asgtk_mystyle_edit_new ()
{
	ASGtkMyStyleEdit *self = g_object_new( ASGTK_TYPE_MYSTYLE_EDIT, NULL );
	GtkWidget *wself = GTK_WIDGET(self); /* so we don't have to do typecasting a hundred of times */
	GtkWidget *table ; 
	GtkWidget *gradient_table;
	GtkWidget *pixmap_table;
	GtkWidget *pixmap_slice_table;
	

	colorize_gtk_widget( wself, get_colorschemed_style_normal() );
	set_flags( self->flags, ASGTK_MYSTYLE_EDIT_ALL_VISIBLE );

	self->syntax = &MyStyleSyntax ;

	create_mystyle_editor_interface( wself );
#if 0
	table = gtk_table_new( 4, 11, FALSE );
	self->inherit_frame = gtk_frame_new( NULL );
	self->inherit_box = gtk_hbox_new(FALSE, 5);
	self->inherit_list = asgtk_simple_list_new( "Inherited MyStyles : " );
	self->inherit_list_window  = ASGTK_SCROLLED_WINDOW(GTK_POLICY_NEVER, GTK_POLICY_ALWAYS,GTK_SHADOW_IN);
	self->inherit_list_btn_box= gtk_vbutton_box_new(); 	

	self->font = gtk_entry_new();
	self->fore_color = gtk_entry_new();
	self->back_color = gtk_entry_new();
	self->text_style = gtk_combo_box_new_text();
	fill_text_style_combo_box( self->text_style ); 
	
	self->overlay = gtk_check_button_new_with_label("Overlay with : ");
	gtk_button_set_relief(GTK_BUTTON(self->overlay), GTK_RELIEF_NONE );
   	g_signal_connect ((gpointer) self->overlay, "clicked", G_CALLBACK (on_mystyle_overlay_clicked), self);
	self->overlay_mystyle = gtk_combo_box_new_text();
	
	self->backpixmap_frame = asgtk_collapsing_frame_new(" Background texture : ","check here if no texture") ; 		
	gtk_frame_set_shadow_type( GTK_FRAME(self->backpixmap_frame), GTK_SHADOW_IN);

	self->backpixmap_notebook = gtk_notebook_new();

	pixmap_table = gtk_table_new (8, 4, FALSE);
	gtk_notebook_append_page( GTK_NOTEBOOK(self->backpixmap_notebook), pixmap_table, gtk_label_new("Pixmap"));
	gradient_table = gtk_table_new (8, 4, FALSE);
	gtk_notebook_append_page( GTK_NOTEBOOK(self->backpixmap_notebook), gradient_table, gtk_label_new("Gradient"));
	gtk_widget_show_all (self->backpixmap_notebook);

	self->pixmap_filename = gtk_entry_new ();
	self->pixmap_browse_btn = asgtk_new_button(GTK_STOCK_FLOPPY, "Browse");

	self->pixmap_preview = asgtk_image_view_new();
 	gtk_widget_set_size_request (self->pixmap_preview, 128, 128);


	self->pixmap_slice_margins = gtk_expander_new ("Slicing margins ... ");
	self->pixmap_slice_frame = gtk_frame_new (NULL);
	self->pixmap_slice_x_start_adj = gtk_adjustment_new (1, 0, 100, 1, 10, 10);
	self->pixmap_slice_x_start = gtk_spin_button_new (GTK_ADJUSTMENT (self->pixmap_slice_x_start_adj), 1, 0);
	self->pixmap_slice_x_end_adj = gtk_adjustment_new (1, 0, 100, 1, 10, 10);
	self->pixmap_slice_x_end = gtk_spin_button_new (GTK_ADJUSTMENT (self->pixmap_slice_x_end_adj), 1, 0);
	self->pixmap_slice_y_start_adj = gtk_adjustment_new (1, 0, 100, 1, 10, 10);
	self->pixmap_slice_y_start = gtk_spin_button_new (GTK_ADJUSTMENT (self->pixmap_slice_y_start_adj), 1, 0);
	self->pixmap_slice_y_end_adj = gtk_adjustment_new (1, 0, 100, 1, 10, 10);
	self->pixmap_slice_y_end = gtk_spin_button_new (GTK_ADJUSTMENT (self->pixmap_slice_y_end_adj), 1, 0);
	
	self->pixmap_pseudo_transp = gtk_radio_button_new_with_mnemonic (NULL, "Pseudo-transparent");
	gtk_button_set_focus_on_click (GTK_BUTTON (self->pixmap_pseudo_transp), FALSE);
	self->pixmap_pseudo_transp_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (self->pixmap_pseudo_transp));
	self->pixmap_shaped = gtk_radio_button_new_with_mnemonic (self->pixmap_pseudo_transp_group, "Shaped");

	self->pixmap_blend_type = gtk_combo_box_new_text ();

	self->pixmap_tiled = gtk_radio_button_new_with_mnemonic (NULL, "Tiled");
	self->pixmap_tiled_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (self->pixmap_tiled));
  	self->pixmap_scaled = gtk_radio_button_new_with_mnemonic (self->pixmap_tiled_group, "Scaled");
  	self->pixmap_sliced = gtk_radio_button_new_with_mnemonic (self->pixmap_tiled_group, "Sliced");

	pixmap_slice_table = gtk_table_new (6, 8, FALSE);
	
	ASGTK_PACK_BEGIN(self->inherit_box);
		ASGTK_PACK_TO_START(self->inherit_list_window, TRUE, TRUE, 0);
		ASGTK_PACK_TO_END(self->inherit_list_btn_box, FALSE, FALSE, 0);
	ASGTK_PACK_END;

	ASGTK_PACK_BEGIN(self->inherit_list_btn_box);
		self->inherit_list_add_btn 	= asgtk_add_button_to_box( ASGTK_PACK_BOX, GTK_STOCK_ADD, "Add", G_CALLBACK(on_add_inherit_mystyle_btn_clicked), self ); 	
		self->inherit_list_del_btn 	= asgtk_add_button_to_box( ASGTK_PACK_BOX, GTK_STOCK_DELETE, "Delete", G_CALLBACK(on_add_inherit_mystyle_btn_clicked), self ); 	
	ASGTK_PACK_END;

	ASGTK_TABLE_BEGIN(pixmap_slice_table);
		ASGTK_ROW_BEGIN;
			ASGTK_TABLE_CELL_SKIP;
			ASGTK_TABLE_CELL(ASGTK_ALIGNED_LABEL("X", 0, 0.5));
			ASGTK_TABLE_CELL(ASGTK_ALIGNED_LABEL("Start", 0, 0.5));
			ASGTK_TABLE_CELL(self->pixmap_slice_x_start);
			ASGTK_TABLE_CELL_SKIP;
			ASGTK_TABLE_CELL(ASGTK_ALIGNED_LABEL("End", 0, 0.5));
			ASGTK_TABLE_CELL(self->pixmap_slice_x_end);
		ASGTK_ROW_END;
		ASGTK_ROW_BEGIN;
			ASGTK_TABLE_CELL(ASGTK_ALIGNED_LABEL("Y", 0, 0.5));
			ASGTK_TABLE_CELL_SKIP;
			ASGTK_TABLE_CELL(ASGTK_ALIGNED_LABEL("intact", 1, 1));
			ASGTK_TABLE_CELL_SPAN2D(gtk_vseparator_new (),1,5);
			ASGTK_TABLE_CELL_SPAN(ASGTK_ALIGNED_LABEL("scaled horizontally", 0, 1),2);
			ASGTK_TABLE_CELL_SPAN2D(gtk_vseparator_new (),1,5);
			ASGTK_TABLE_CELL(ASGTK_ALIGNED_LABEL("intact", 0, 1));
		ASGTK_ROW_END;
		ASGTK_ROW_BEGIN;
			ASGTK_TABLE_CELL(ASGTK_ALIGNED_LABEL("Start", 0, 0.5));
			ASGTK_TABLE_CELL(self->pixmap_slice_y_start);
			ASGTK_TABLE_CELL_SPAN(gtk_hseparator_new (),6);
		ASGTK_ROW_END;
		ASGTK_ROW_BEGIN;
			ASGTK_TABLE_CELL_SKIP;
			ASGTK_TABLE_CELL_SKIP;
			ASGTK_TABLE_CELL(ASGTK_ALIGNED_LABEL(" scaled\n vertically", 1, 0.5));
			ASGTK_TABLE_CELL_SKIP;
			ASGTK_TABLE_CELL_SPAN(ASGTK_ALIGNED_LABEL("scaled both ways", 0.5, 0.5),2);
			ASGTK_TABLE_CELL_SKIP;
			ASGTK_TABLE_CELL(ASGTK_ALIGNED_LABEL("scaled\nvertically", 0, 0.5));
		ASGTK_ROW_END;
		ASGTK_ROW_BEGIN;
			ASGTK_TABLE_CELL(ASGTK_ALIGNED_LABEL("End", 0, 0.5));
			ASGTK_TABLE_CELL(self->pixmap_slice_y_end);
			ASGTK_TABLE_CELL_SPAN(gtk_hseparator_new (),6);
		ASGTK_ROW_END;
		ASGTK_ROW_BEGIN;
			ASGTK_TABLE_CELL_SKIP;
			ASGTK_TABLE_CELL_SKIP;
			ASGTK_TABLE_CELL(ASGTK_ALIGNED_LABEL("intact", 1, 0));
			ASGTK_TABLE_CELL_SKIP;
			ASGTK_TABLE_CELL_SPAN(ASGTK_ALIGNED_LABEL("scaled horizontally", 0.5, 0),2);
			ASGTK_TABLE_CELL_SKIP;
			ASGTK_TABLE_CELL(ASGTK_ALIGNED_LABEL("intact", 0, 0));
		ASGTK_ROW_END;
	ASGTK_TABLE_END;

	ASGTK_CONTAINER_ADD(self->pixmap_slice_frame, ASGTK_ALIGN(pixmap_slice_table,0.5, 0.5, 1, 1,3,0,3,0));
	ASGTK_CONTAINER_ADD(self->pixmap_slice_margins, self->pixmap_slice_frame);

	ASGTK_TABLE_BEGIN(pixmap_table);
		ASGTK_ROW_BEGIN;
			ASGTK_TABLE_CELL_SPAN(ASGTK_ALIGNED_LABEL("Pixmap file name :", 0, 0.5),2);
			ASGTK_TABLE_CELL(self->pixmap_browse_btn);
			ASGTK_TABLE_CELL_SPAN2D(self->pixmap_preview,1,7);
		ASGTK_ROW_END;
		ASGTK_ROW_BEGIN;
			ASGTK_TABLE_CELL_SPAN(self->pixmap_filename,3);
		ASGTK_ROW_END;
		ASGTK_ROW_BEGIN;
			ASGTK_TABLE_CELL_SPAN(gtk_hseparator_new (),3);
		ASGTK_ROW_END;
		ASGTK_ROW_BEGIN;
			ASGTK_TABLE_CELL_SPAN(self->pixmap_pseudo_transp,2);
			ASGTK_TABLE_CELL(self->pixmap_shaped);
		ASGTK_ROW_END;
		ASGTK_ROW_BEGIN;
			ASGTK_TABLE_CELL(ASGTK_ALIGNED_LABEL("Blending type :", 0, 0.5));
			ASGTK_TABLE_CELL_SPAN(self->pixmap_blend_type,2);
		ASGTK_ROW_END;
		ASGTK_ROW_BEGIN;
			ASGTK_TABLE_CELL_SPAN(gtk_hseparator_new (),3);
		ASGTK_ROW_END;
		ASGTK_ROW_BEGIN;
			ASGTK_TABLE_CELL(self->pixmap_tiled);
			ASGTK_TABLE_CELL(self->pixmap_scaled);
			ASGTK_TABLE_CELL(self->pixmap_sliced);
		ASGTK_ROW_END;
		ASGTK_ROW_BEGIN;
			ASGTK_TABLE_CELL_SPAN(self->pixmap_slice_margins,4);
		ASGTK_ROW_END;
	ASGTK_TABLE_END;
	
	ASGTK_CONTAINER_ADD(self->backpixmap_frame, self->backpixmap_notebook);

	ASGTK_TABLE_BEGIN(table);
		ASGTK_ROW_BEGIN;
			ASGTK_TABLE_CELL_SPAN(self->inherit_frame,2);
			ASGTK_TABLE_CELL_SPAN2D(ASGTK_ALIGN(self->backpixmap_frame,0.5,0.5,1,1,0,0,5,0),2,6);
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
		ASGTK_ROW_END;
		ASGTK_ROW_BEGIN;
			ASGTK_TABLE_CELL(self->overlay);
			ASGTK_TABLE_CELL(self->overlay_mystyle);
		ASGTK_ROW_END;
	ASGTK_TABLE_END;

//	gtk_widget_set_size_request(self->text_style,INHERIT_LIST_WIDTH,);	

	ASGTK_CONTAINER_CONFIG(self->inherit_list_window, INHERIT_LIST_WIDTH, INHERIT_LIST_HEIGHT,0);
	
	ASGTK_CONTAINER_ADD(self->inherit_list_window, self->inherit_list);
	ASGTK_CONTAINER_ADD(self->inherit_frame, self->inherit_box);
	gtk_container_set_border_width( GTK_CONTAINER(table), 3 );
	ASGTK_CONTAINER_ADD(self, table);

	gtk_widget_hide( ASGTK_IMAGE_VIEW(self->pixmap_preview)->details_frame );
#endif

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
	panel->expander 	= gtk_expander_new ("MyStyles - basic drawing styles");
	panel->frame 		= gtk_frame_new(NULL);
	gtk_frame_set_shadow_type( GTK_FRAME(panel->frame), GTK_SHADOW_NONE);
	 
	panel->hbox 		= gtk_hbox_new(FALSE, 5);
	panel->list_vbox 	= gtk_vbox_new(FALSE, 5);
	panel->list 		= asgtk_simple_list_new( "Available MyStyles : " );
	panel->list_window  = ASGTK_SCROLLED_WINDOW(GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC,GTK_SHADOW_IN);
	panel->list_hbtn_box= gtk_hbutton_box_new(); 	
	panel->mystyle_frame= gtk_frame_new(NULL); 	
	panel->mystyle_editor= asgtk_mystyle_edit_new(); 	
	asgtk_mystyle_edit_set_mystyles_list( ASGTK_MYSTYLE_EDIT(panel->mystyle_editor), ASGTK_SIMPLE_LIST(panel->list)->tree_model ); 
	asgtk_mystyle_edit_set_name_visible(  ASGTK_MYSTYLE_EDIT(panel->mystyle_editor), False );
	
	asgtk_simple_list_set_sel_handling( ASGTK_SIMPLE_LIST(panel->list), G_OBJECT(panel->mystyle_editor), mystyle_panel_sel_handler ); 

	gtk_container_add( GTK_CONTAINER(panel->mystyle_frame), panel->mystyle_editor );
	ASGTK_PACK_BEGIN(panel->hbox);
		ASGTK_PACK_TO_START(panel->list_vbox, FALSE, FALSE, 5);
		ASGTK_PACK_TO_START(panel->mystyle_frame, TRUE, TRUE, 0);
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
	ASGTK_CONTAINER_ADD(panel->expander, panel->frame);
	

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
		ASGTK_PACK_TO_START( self->mystyles->expander	, FALSE, FALSE, 5);
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
