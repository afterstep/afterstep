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
#include "../libAfterImage/asvisual.h"
#include "../libAfterStep/asapp.h"
#include "../libAfterStep/screen.h"
#include "../libAfterStep/session.h"
#include "../libAfterStep/parser.h"
#include "../libAfterStep/freestor.h"
#include "../libAfterStep/mystyle.h"
#include "../libAfterStep/session.h"
#include "../libAfterConf/afterconf.h"

#include <unistd.h>		   

#include "asgtk.h"
#include "asgtkai.h"
#include "asgtklistviews.h"
#include "asgtklookedit.h"
#include "asgtkcframe.h"

#define MYSTYLE_LIST_WIDTH   	100
#define MYSTYLE_LIST_HEIGHT  	300
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
	self->style_def = NULL ;
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


void  
color2button_image( GtkImage *btn, const char *color )
{
	ARGB32 argb = 0xFFFFFFFF ;
	GdkPixbuf *pb ;

	if( color && color[0] != '\0')
		parse_argb_color( color, &argb );

	pb = solid_color2GdkPixbuf( argb, 16, 16 );
	if( pb ) 
	{	
		gtk_image_set_from_pixbuf( GTK_IMAGE(btn), pb );
		gdk_pixbuf_unref( pb ); 		   
	}
}	

typedef enum
{
	ASGtkMSO_Name = 0,
	ASGtkMSO_Overlay,
	ASGtkMSO_Inherits,	
	ASGtkMSO_Font,	
	ASGtkMSO_Colors,	
	ASGtkMSO_Shadow,
	ASGtkMSO_Background,
	ASGtkMSO_Options
}ASGtkMyStyleOptions ;

void set_container_child_sensitive( GtkWidget *child, gpointer data)
{
	GtkWidget *to_skip = data?GTK_WIDGET(data):NULL;
	if( child != to_skip && !GTK_IS_LABEL(child) )
		gtk_widget_set_sensitive( child, TRUE);
}

void set_container_child_insensitive( GtkWidget *child, gpointer data)
{
	GtkWidget *to_skip = data?GTK_WIDGET(data):NULL;
	if( child != to_skip && !GTK_IS_LABEL(child) )
		gtk_widget_set_sensitive( child, FALSE);
}

void
asgtk_mystyle_edit_set_line_enabled(ASGtkMyStyleEdit *self, ASGtkMyStyleOptions opt, Bool enabled)
{

	GtkToggleButton *toggle = NULL ; 
	GtkContainer *container = NULL ; 
	switch( opt ) 
	{
		case ASGtkMSO_Name : 		container = GTK_CONTAINER(self->hbox1_mystyle_name) ; break ;
		case ASGtkMSO_Overlay : 	toggle = GTK_TOGGLE_BUTTON(self->tgl2_overlay) ; 
									container = GTK_CONTAINER(self->hbox2_overlay) ; break ;
		case ASGtkMSO_Inherits : 	toggle = GTK_TOGGLE_BUTTON(self->tgl3_inherit) ; 
									container = GTK_CONTAINER(self->hbox3_inherit) ; break ;
		case ASGtkMSO_Font : 		toggle = GTK_TOGGLE_BUTTON(self->tgl4_font) ; 
									container = GTK_CONTAINER(self->hbox4_font) ; break ;
		case ASGtkMSO_Colors : 		toggle = GTK_TOGGLE_BUTTON(self->tgl5_colors) ; 
									container = GTK_CONTAINER(self->hbox5_colors) ; break ;
		case ASGtkMSO_Shadow : 		toggle = GTK_TOGGLE_BUTTON(self->tgl5_shadow) ; 
									container = GTK_CONTAINER(self->hbox6_shadow) ; break ;
		case ASGtkMSO_Background : 	toggle = GTK_TOGGLE_BUTTON(self->tgl7_background) ; 
									container = GTK_CONTAINER(self->hbox7_background) ; break ;
		case ASGtkMSO_Options :
		default : 
			return;
	}

	if( toggle ) 
	{
		gtk_toggle_button_set_active( toggle, enabled );
#if (GTK_MAJOR_VERSION>=2) && (GTK_MINOR_VERSION>=6)	   
		gtk_button_set_label( GTK_BUTTON(toggle), "" );
		gtk_button_set_image( GTK_BUTTON(toggle), gtk_image_new_from_stock(enabled?GTK_STOCK_YES:GTK_STOCK_NO, GTK_ICON_SIZE_BUTTON) );
		gtk_toggle_button_set_mode( toggle, False );
		gtk_button_set_relief (GTK_BUTTON (toggle), GTK_RELIEF_NONE);
		
#else
		gtk_button_set_label( GTK_BUTTON(toggle), enabled?"[-]":"[+]" );
#endif
	}
	
	if( container ) 
		gtk_container_forall( container, enabled?set_container_child_sensitive:set_container_child_insensitive, toggle );
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

typedef enum {
	ASGtkMSB_SolidColor = 0,
	ASGtkMSB_GradientH,
	ASGtkMSB_GradientV,
	ASGtkMSB_GradientLT2RB,
	ASGtkMSB_GradientRT2LB,
	ASGtkMSB_ScaledTexture,
	ASGtkMSB_TiledTexture,
	ASGtkMSB_Tint,
	ASGtkMSB_ScaledShaped,
	ASGtkMSB_SlicedShaped,
	ASGtkMSB_TiledShaped,
	ASGtkMSB_TintTwoWay,
	ASGtkMSB_TiledPseudoTransp,
	ASGtkMSB_ScaledPseudoTransp,
	ASGtkMSB_SlicedPseudoTransp,
	ASGtkMSB_BackgroundTypes
}ASGtkMyStyleBackgroundTypes;
	

void 
asgtk_mystyle_edit_set_background_type(ASGtkMyStyleEdit *self, int type)
{
	ASGtkMyStyleBackgroundTypes back_type = ASGtkMSB_BackgroundTypes;

	gtk_widget_hide( self->hbox8_grad_details );
	gtk_widget_hide( self->hbox9_texture_file );
	gtk_widget_hide( self->hbox10_texture_blend_type );
	gtk_widget_hide( self->table1_texture_slicing );
	gtk_combo_box_set_active( GTK_COMBO_BOX(self->combo_background_type), -1 );

	if( type == 0 ) 
	{
		back_type = ASGtkMSB_SolidColor ;
	}else if( type > 0 && type <= TEXTURE_GRADIENT_END ) 
	{
		switch( type ) 
		{
			case     TEXTURE_GRADIENT : 	back_type = ASGtkMSB_GradientLT2RB ; break ;

			case     TEXTURE_HGRADIENT :
			case     TEXTURE_HCGRADIENT :   back_type = ASGtkMSB_GradientH ; break ;
			case     TEXTURE_VGRADIENT :
			case     TEXTURE_VCGRADIENT :	back_type = ASGtkMSB_GradientV ; break ;
			
			case     TEXTURE_GRADIENT_TL2BR	:  	back_type = ASGtkMSB_GradientLT2RB ; break ;
			case     TEXTURE_GRADIENT_BL2TR :	back_type = ASGtkMSB_GradientRT2LB ; break ;
			case     TEXTURE_GRADIENT_T2B	:	back_type = ASGtkMSB_GradientV ; break ;
			case     TEXTURE_GRADIENT_L2R	:	back_type = ASGtkMSB_GradientH ; break ;
		}
		/* to show : hbox8_grad_details	*/
		gtk_widget_show( self->hbox8_grad_details );

	}else if( type >= TEXTURE_TEXTURED_START && type <= TEXTURE_TEXTURED_END )  
	{
		/* to show : hbox10_texture_blend_type, hbox9_texture_file, table1_texture_slicing	*/
		if( type == TEXTURE_SHAPED_SCALED_PIXMAP ) 
			back_type = get_flags( self->flags, ASGTK_MYSTYLE_SLICED)?
							ASGtkMSB_SlicedShaped:ASGtkMSB_ScaledShaped ; 
		else if( type == TEXTURE_SHAPED_PIXMAP ) 
			back_type = ASGtkMSB_TiledShaped ;
		else if( type == TEXTURE_SCALED_PIXMAP ) 
			back_type = ASGtkMSB_TiledShaped ;
		else if( type == TEXTURE_PIXMAP ) 
			back_type = ASGtkMSB_TiledShaped ;
		else if( type == TEXTURE_TRANSPARENT ) 
			back_type = ASGtkMSB_Tint ;
		else if( type == TEXTURE_TRANSPARENT_TWOWAY ) 
			back_type = ASGtkMSB_TintTwoWay ;
		else if( type >= TEXTURE_TRANSPIXMAP && type < TEXTURE_TRANSPIXMAP_END )
			back_type = ASGtkMSB_TiledPseudoTransp ;
		else if( type >= TEXTURE_SCALED_TRANSPIXMAP && type < TEXTURE_SCALED_TRANSPIXMAP_END )
			back_type = get_flags( self->flags, ASGTK_MYSTYLE_SLICED)?
							ASGtkMSB_SlicedPseudoTransp:ASGtkMSB_ScaledPseudoTransp ;
		
		gtk_widget_show( self->hbox9_texture_file );
		gtk_widget_show( self->hbox10_texture_blend_type );
		if( get_flags( self->flags, ASGTK_MYSTYLE_SLICED) );
			gtk_widget_show( self->table1_texture_slicing );
	}	

	if( back_type < ASGtkMSB_BackgroundTypes ) 
	{
		asgtk_mystyle_edit_set_line_enabled(self, ASGtkMSO_Background, True);		
		gtk_combo_box_set_active( GTK_COMBO_BOX(self->combo_background_type), back_type );
	}
	self->background_type = back_type ;

}


void 
asgtk_mystyle_edit_set_sliced(ASGtkMyStyleEdit *self, Bool sliced)
{
	ASGtkMyStyleBackgroundTypes old_type = self->background_type ;
	
	if( sliced && !get_flags( self->flags, ASGTK_MYSTYLE_SLICED)) 
	{
		set_flags( self->flags, ASGTK_MYSTYLE_SLICED);
		gtk_widget_show( self->table1_texture_slicing );
		if( self->background_type == ASGtkMSB_ScaledPseudoTransp ) 
			self->background_type =  ASGtkMSB_SlicedPseudoTransp ;
		else if( self->background_type == ASGtkMSB_ScaledShaped ) 
			self->background_type =  ASGtkMSB_SlicedShaped ;
	}else if( !sliced && get_flags( self->flags, ASGTK_MYSTYLE_SLICED)) 
	{
		clear_flags( self->flags, ASGTK_MYSTYLE_SLICED);
		gtk_widget_hide( self->table1_texture_slicing );
		if( self->background_type == ASGtkMSB_SlicedPseudoTransp ) 
			self->background_type =  ASGtkMSB_ScaledPseudoTransp ;
		else if( self->background_type == ASGtkMSB_SlicedShaped ) 
			self->background_type =  ASGtkMSB_ScaledShaped ;
	}
	if( old_type != self->background_type ) 
		gtk_combo_box_set_active( GTK_COMBO_BOX(self->combo_background_type), self->background_type );
}

static void 
asgtk_mystyle_edit_set_style_def (MyStyleDefinition *msd, ASGtkMyStyleEdit *self) 
{
	ASGtkSimpleList *inherit_list = ASGTK_SIMPLE_LIST(self->tw_inherit_list);
	int i;

	self->style_def = msd ; 
	
	asgtk_simple_list_purge (inherit_list);

	gtk_button_set_label( GTK_BUTTON(self->btn_font_name), NULL );
	gtk_button_set_label( GTK_BUTTON(self->btn_fore_color), NULL );
	gtk_button_set_label( GTK_BUTTON(self->btn_back_color), NULL );
	gtk_combo_box_set_active( GTK_COMBO_BOX(self->combo_shadow_type), AST_3DTypes );
	gtk_combo_box_set_active( GTK_COMBO_BOX(self->combo_overlay_mystyle), -1 );
	for( i = 0 ; i < ASGtkMSO_Options ; ++i ) 
		asgtk_mystyle_edit_set_line_enabled(self, i, False);		

	set_flags( self->flags, ASGTK_MYSTYLE_SLICED);
	asgtk_mystyle_edit_set_sliced(self, False);	
	asgtk_mystyle_edit_set_background_type(self, -1);	
	
	if (msd == NULL)
		return;
		
	asgtk_mystyle_edit_set_background_type (self, msd->texture_type);

	for (i = 0 ; i < msd->inherit_num; ++i)
		asgtk_simple_list_append( inherit_list, msd->inherit[i], NULL );

	if (i > 0)
		asgtk_mystyle_edit_set_line_enabled(self, ASGtkMSO_Inherits, True);		

	if (get_flags (msd->set_flags, MYSTYLE_Font))
	{
		gtk_button_set_label( GTK_BUTTON(self->btn_font_name), msd->Font );
		asgtk_mystyle_edit_set_line_enabled(self, ASGtkMSO_Font, True);		
	}		
	if (get_flags (msd->set_flags, MYSTYLE_ForeColor))
	{
		char *c = msd->ForeColor?msd->ForeColor:"Inactive1Text";
		color2button_image (GTK_IMAGE (self->img_fore_color), c);
		gtk_button_set_label (GTK_BUTTON (self->btn_fore_color), c);
	}		
	if (get_flags (msd->set_flags, MYSTYLE_BackColor))
	{
		char *c = msd->BackColor?msd->BackColor:"Inactive1";
		color2button_image (GTK_IMAGE (self->img_back_color), c);
		gtk_button_set_label (GTK_BUTTON (self->btn_back_color), c);
	}		

	if (get_flags (msd->set_flags, MYSTYLE_ForeColor|MYSTYLE_BackColor))
		asgtk_mystyle_edit_set_line_enabled(self, ASGtkMSO_Colors, True);		

	if (get_flags (msd->set_flags, MYSTYLE_TextStyle))
	{
		gtk_combo_box_set_active( GTK_COMBO_BOX(self->combo_shadow_type), msd->TextStyle);
		asgtk_mystyle_edit_set_line_enabled(self, ASGtkMSO_Shadow, True);		
	}

	if (get_flags (msd->set_flags, MYSTYLE_SliceXStart))
		gtk_spin_button_set_value( GTK_SPIN_BUTTON(self->spin_texture_slicing_x_start), msd->SliceXStart ); 
	if (get_flags (msd->set_flags, MYSTYLE_SliceXEnd))
		gtk_spin_button_set_value( GTK_SPIN_BUTTON(self->spin_texture_slicing_x_end), msd->SliceXEnd ); 
	if (get_flags (msd->set_flags, MYSTYLE_SliceYStart))
		gtk_spin_button_set_value( GTK_SPIN_BUTTON(self->spin_texture_slicing_y_start), msd->SliceYStart ); 
	if (get_flags (msd->set_flags, MYSTYLE_SliceYEnd))
		gtk_spin_button_set_value( GTK_SPIN_BUTTON(self->spin_texture_slicing_y_end), msd->SliceYEnd ); 

	if (get_flags (msd->set_flags, MYSTYLE_SLICE_SET))
		asgtk_mystyle_edit_set_sliced(self, True);

	if (msd->overlay_type >= TEXTURE_TRANSPIXMAP)
	{
		asgtk_mystyle_edit_set_line_enabled(self, ASGtkMSO_Overlay, True);		

#if 0 // TODO:
			GtkTreeIter iter ; 
			// gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(self->overlay), TRUE );
			if( gtk_tree_model_get_iter_first( self->mystyles_list, &iter ) )
			{
				char *tmp = NULL ; 
				do
				{
					gtk_tree_model_get (self->mystyles_list, &iter, 0, &tmp, -1);
					if( mystrcasecmp( tmp, item.data.string ) == 0 ) 
					{
						gtk_combo_box_set_active_iter( GTK_COMBO_BOX(self->combo_overlay_mystyle), &iter );
						break ;
					}
				}while( gtk_tree_model_iter_next( self->mystyles_list, &iter ) );
			}
#endif
	}
	if (msd->back_pixmap) 
	{
		// TODO
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
	
	gtk_box_set_spacing (GTK_BOX(wself), 5);

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

static void
on_add_to_library_mystyle_btn_clicked(GtkButton *button, gpointer user_data)
{
	ASGtkMyStylesPanel *panel = (ASGtkMyStylesPanel *)user_data;
	ASGtkMyStyleEdit *msedit = ASGTK_MYSTYLE_EDIT(panel->mystyle_editor);
	MyStyleDefinition *msd = msedit->style_def ; 
	
	if (msd)
	{
		char *filename;
#if 0		
		filename = make_session_data_file  (Session, False, 0, MYSTYLES_DIR);
		if (filename)
		{
    		CheckOrCreate (filename);
		    free (filename);
		}
#endif
		filename = make_session_data_file  (Session, False, 0, MYSTYLES_DIR, msd->Name, NULL );
		if (filename)
		{
			FreeStorageElem *fs = MyStyleDefinitionsList2free_storage (msd, &LookSyntax);
			if (fs)
			{
				WriteFreeStorageToFile (filename, "afterstep", &MyStyleSyntax, fs->sub, 0);
				DestroyFreeStorage (&fs);
			}
			
			free (filename);
		}
	}
	
}

static void
on_save_as_mystyle_btn_clicked(GtkButton *button, gpointer user_data)
{
	ASGtkLookEdit *self = ASGTK_LOOK_EDIT(user_data);
}


void mystyle_panel_sel_handler(GObject *selection_owner, gpointer user_data)
{
	ASGtkMyStyleEdit *mystyle_edit = ASGTK_MYSTYLE_EDIT(selection_owner);
	MyStyleDefinition *style_def = (MyStyleDefinition *)user_data ;
	
	if( mystyle_edit == NULL ) 
		return ;
	LOCAL_DEBUG_OUT( "storage = %p, edit = %p", style_def, mystyle_edit );
	asgtk_mystyle_edit_set_style_def (style_def, mystyle_edit);
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
	panel->list_hbtn_box2= gtk_hbutton_box_new(); 	
	panel->mystyle_frame= gtk_frame_new(NULL); 	
	panel->mystyle_editor= asgtk_mystyle_edit_new(); 	

fprintf( stderr, "$$$$$$ panel = %p, msedit = %p\n", panel, panel->mystyle_editor);
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
		ASGTK_PACK_TO_END(panel->list_hbtn_box2, TRUE, FALSE, 0);
		ASGTK_PACK_TO_END(panel->list_hbtn_box, TRUE, FALSE, 0);
	ASGTK_PACK_END;

	ASGTK_PACK_BEGIN(panel->list_hbtn_box);
		panel->list_add_btn 	= asgtk_add_button_to_box( ASGTK_PACK_BOX, GTK_STOCK_ADD, "Add", G_CALLBACK(on_add_mystyle_btn_clicked), panel ); 	
		panel->list_del_btn 	= asgtk_add_button_to_box( ASGTK_PACK_BOX, GTK_STOCK_DELETE, "Delete", G_CALLBACK(on_add_mystyle_btn_clicked), panel ); 	
		panel->list_rename_btn 	= asgtk_add_button_to_box( ASGTK_PACK_BOX, GTK_STOCK_PREFERENCES, "Rename", G_CALLBACK(on_add_mystyle_btn_clicked), panel );
	ASGTK_PACK_END;
	ASGTK_PACK_BEGIN(panel->list_hbtn_box2);
		panel->list_tolib_btn 	= asgtk_add_button_to_box( ASGTK_PACK_BOX, GTK_STOCK_CONVERT, "Add to Library", G_CALLBACK(on_add_to_library_mystyle_btn_clicked), panel ); 	
		panel->list_saveas_btn 	= asgtk_add_button_to_box( ASGTK_PACK_BOX, GTK_STOCK_SAVE_AS, "Save As", G_CALLBACK(on_save_as_mystyle_btn_clicked), panel ); 	
	ASGTK_PACK_END;
	InitSession();

	ASGTK_CONTAINER_CONFIG(panel->list_window,MYSTYLE_LIST_WIDTH, MYSTYLE_LIST_HEIGHT,0);

	ASGTK_CONTAINER_ADD(panel->list_window, panel->list);
	ASGTK_CONTAINER_ADD(panel->frame, panel->hbox);
	ASGTK_CONTAINER_ADD(panel->expander, panel->frame);
	

}


static void 
MyStyleDefs2MyStylesPanel( MyStyleDefinition *style_defs, ASGtkMyStylesPanel *panel ) 
{
	MyStyleDefinition *curr = style_defs ;
	ASGtkSimpleList *list = ASGTK_SIMPLE_LIST(panel->list);
	
	asgtk_simple_list_purge( list );
	while( curr != NULL ) 
	{
		asgtk_simple_list_append( list, curr->Name, curr );
		curr = curr->next ; 
	}
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
	
	if( self->config != NULL ) 
	{
		MyStyleDefs2MyStylesPanel( NULL, self->mystyles );	
 		DestroyLookConfig (self->config);
		self->config = NULL;
	}

	if( self->configfilename != NULL ) 
	{
		self->config = ParseLookOptions (self->configfilename, self->myname);
   	    show_progress("configuration loaded from \"%s\" ...", self->configfilename);
	}	
	if (self->config != NULL)
	{
		MyStyleDefs2MyStylesPanel (self->config->style_defs, self->mystyles);	
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

	asgtk_look_edit_set_configfile( ASGTK_LOOK_EDIT(look_edit), PutHome("~/.afterstep/looks/look.Mine") );
	
	g_signal_connect (G_OBJECT (main_window), "destroy", G_CALLBACK (on_destroy), NULL);
  	gtk_widget_show (main_window);

  gtk_main ();
  return 0;
}

#endif
