/* 
 * Copyright (C) 2005 Sasha Vasko <sasha at aftercode.net>
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

#include <unistd.h>

#include "../include/afterbase.h"
#include "../libAfterImage/afterimage.h"
#include "../libAfterStep/asapp.h"
#include "../libAfterStep/screen.h"


#include "asgtk.h"
#include "asgtkai.h"
#include "asgtkxmleditor.h"

/*  local function prototypes  */
static void asgtk_xml_editor_class_init (ASGtkXMLEditorClass *klass);
static void asgtk_xml_editor_init (ASGtkXMLEditor *iv);
static void asgtk_xml_editor_dispose (GObject *object);
static void asgtk_xml_editor_finalize (GObject *object);
static void asgtk_xml_editor_style_set (GtkWidget *widget, GtkStyle  *prev_style);
static void check_save_changes( ASGtkXMLEditor *xe );
static void asgtk_xml_editor_tag_activate ( GtkTreeView       *tree_view,
	   									    GtkTreePath       *path,
				 							GtkTreeViewColumn *column,
				 							gpointer           user_data);
static void asgtk_xml_editor_tag_selected ( GtkTreeSelection *selection, 
											gpointer user_data);

/*  private variables  */
static GtkWindowClass *parent_class = NULL;

GType
asgtk_xml_editor_get_type (void)
{
  	static GType id_type = 0;

  	if (! id_type)
    {
    	static const GTypeInfo id_info =
      	{
        	sizeof (ASGtkXMLEditorClass),
        	(GBaseInitFunc)     NULL,
        	(GBaseFinalizeFunc) NULL,
			(GClassInitFunc)    asgtk_xml_editor_class_init,
        	NULL,           /* class_finalize */
        	NULL,           /* class_data     */
        	sizeof (ASGtkXMLEditor),
        	0,              /* n_preallocs    */
        	(GInstanceInitFunc) asgtk_xml_editor_init,
      	};

      	id_type = g_type_register_static (	GTK_TYPE_WINDOW,
        	                                "ASGtkXMLEditor",
            	                            &id_info, 0);
    }

  	return id_type;
}

static void
asgtk_xml_editor_class_init (ASGtkXMLEditorClass *klass)
{
  	GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  	parent_class = g_type_class_peek_parent (klass);

  	object_class->dispose   = asgtk_xml_editor_dispose;
  	object_class->finalize  = asgtk_xml_editor_finalize;

  	widget_class->style_set = asgtk_xml_editor_style_set;

}

static void
asgtk_xml_editor_init (ASGtkXMLEditor *id)
{
	id->entry = NULL ;
}

static void
asgtk_xml_editor_dispose (GObject *object)
{
  	ASGtkXMLEditor *xe = ASGTK_XML_EDITOR (object);
	if( xe->entry ) 
	{
		check_save_changes( xe ); 

		LOCAL_DEBUG_OUT( " entry ref_count = %d", xe->entry->ref_count );
		if( unref_asimage_list_entry(xe->entry) ) 
			LOCAL_DEBUG_OUT( " entry ref_count = %d", xe->entry->ref_count );
		xe->entry = NULL ;
	}
  	G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
asgtk_xml_editor_finalize (GObject *object)
{
  	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
asgtk_xml_editor_style_set (GtkWidget *widget,
                          GtkStyle  *prev_style)
{
  /* ASGtkXMLEditor *id = ASGTK_XML_EDITOR (widget); */

  GTK_WIDGET_CLASS (parent_class)->style_set (widget, prev_style);
}

static char *
get_xml_editor_text( ASGtkXMLEditor *xe, Bool selection_only )
{
	GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (xe->text_view));
	GtkTextIter start, end ;

	if( selection_only ) 
		selection_only = gtk_text_buffer_get_selection_bounds( buffer, &start, &end );
		   
	if( !selection_only ) 
	{	
		gtk_text_buffer_get_start_iter( buffer, &start );
		gtk_text_buffer_get_end_iter( buffer, &end );
	}
	return gtk_text_buffer_get_text( buffer, &start, &end, TRUE );
}

static void 
xml_editor_warning( GtkWidget *main_window, const char *format, const char *detail1, const char *detail2 ) 	
{
	GtkWidget *dialog = gtk_message_dialog_new (GTK_WINDOW(main_window),
               				                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                  				GTK_MESSAGE_WARNING,
                                  				GTK_BUTTONS_OK,
												format, detail1, detail2 );
 	gtk_dialog_run (GTK_DIALOG (dialog));
 	gtk_widget_destroy (dialog);	
}

static char* 
get_validated_text( ASGtkXMLEditor *xe, Bool selection, Bool report_success )
{
	ASXmlBuffer xb ; 
	int char_count = 0, tags_count = 0, last_separator = 0 ;
	char *text = get_xml_editor_text(xe, selection);
	Bool success = False ;
	GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (xe->text_view));
	GtkTextIter start, end ;
	
	memset( &xb, 0x00, sizeof(xb));
 	do
	{	
		reset_xml_buffer( &xb );
		while( text[char_count] != '\0' ) 
		{
			char cc = text[char_count]; 
			while( xb.state >= 0 && spool_xml_tag( &xb, &cc, 1 ) <= 0)
			{	
				LOCAL_DEBUG_OUT("[%c] : state=%d, tags_count=%d, level = %d, tag_type = %d", 
									cc, xb.state, xb.tags_count, xb.level, xb.tag_type );
			}
			LOCAL_DEBUG_OUT("[%c] : state=%d, tags_count=%d, level = %d, tag_type = %d", 
							cc, xb.state, xb.tags_count, xb.level, xb.tag_type );

			if( xb.state >= 0 ) 
			{	
				if( isspace(text[char_count]) || 
					text[char_count] == '<' ||
					text[char_count] == '>' ||
					text[char_count] == '=' ||
					text[char_count] == '\"' ||
					text[char_count] == '/'
					) 
					last_separator = char_count ; 
			}
			++char_count ;
			if( ( xb.state == ASXML_Start && xb.tags_count > 0 && xb.level == 0) || 
			  	xb.state < 0 ) 
				break;
		}		   
		tags_count += xb.tags_count ;
		if( xb.state < 0 ) 
		{
			switch( xb.state ) 
			{
				case ASXML_BadStart : 	xml_editor_warning( GTK_WIDGET(xe), "Text encountered before opening tag bracket - not XML format", NULL, NULL ); break;
				case ASXML_BadTagName : xml_editor_warning( GTK_WIDGET(xe), "Invalid characters in tag name", NULL, NULL  );break;
				case ASXML_UnexpectedSlash : xml_editor_warning( GTK_WIDGET(xe), "Unexpected '/' encountered", NULL, NULL );break;
				case ASXML_UnmatchedClose :  xml_editor_warning( GTK_WIDGET(xe), "Closing tag encountered without opening tag", NULL, NULL  );break;
				case ASXML_BadAttrName :   xml_editor_warning( GTK_WIDGET(xe), "Invalid characters in attribute name", NULL, NULL  );break;
				case ASXML_MissingAttrEq : xml_editor_warning( GTK_WIDGET(xe), "Attribute name not followed by '=' character", NULL, NULL  );break;
				default:
					xml_editor_warning( GTK_WIDGET(xe), "Premature end of the input", NULL, NULL );break;
			}
			if( gtk_text_buffer_get_selection_bounds( buffer, &start, &end ) ) 
			{	
				int offset = gtk_text_iter_get_offset (&start);
				char_count += offset ;
				last_separator += offset ;
			}
			if( char_count > last_separator ) 
			{	
				gtk_text_buffer_get_iter_at_offset( buffer, &start, last_separator);
				gtk_text_buffer_get_iter_at_offset( buffer, &end, char_count);
			}else
			{	  
				gtk_text_buffer_get_iter_at_offset( buffer, &start, char_count-1);
				gtk_text_buffer_get_iter_at_offset( buffer, &end, char_count);
			}
			gtk_text_buffer_select_range( buffer, &start, &end );
			break;
		}else if( text[char_count] == '\0' ) 
		{	
			if( xb.state == ASXML_Start && tags_count == 0 ) 
			{
				xml_editor_warning( GTK_WIDGET(xe), "Script contains no valid XML tags", NULL, NULL );
			}else if( xb.state >= 0 && xb.state != ASXML_Start ) 
			{
				xml_editor_warning( GTK_WIDGET(xe), "XML Script is not properly terminated", NULL, NULL );
			}else if( xb.state == ASXML_Start && xb.level > 0 ) 
			{
				xml_editor_warning( GTK_WIDGET(xe), "XML Script contains unterminated XML tags", NULL, NULL );
			}else
			{	
				if( report_success ) 
				{	
					if( gtk_text_buffer_get_selection_bounds( buffer, &start, &end ) ) 
						xml_editor_warning( GTK_WIDGET(xe), "Selected XML seems to be syntactically valid", NULL, NULL );
					else
						xml_editor_warning( GTK_WIDGET(xe), "XML Script seems to be syntactically valid", NULL, NULL );
				}
				success = True ;
			}
		}
	}while( text[char_count] != '\0' );		
	reset_xml_buffer( &xb );
	if( !success ) 
	{	
		g_free( text );
		text = NULL ;
	}
	return text;
}
	

static void
on_refresh_clicked(GtkButton *button, gpointer user_data)
{
	ASGtkXMLEditor *xe = ASGTK_XML_EDITOR(user_data);
	ASVisual *asv = get_screen_visual(NULL);

	if( xe->entry ) 
	{	
		char *text = get_validated_text( xe, (button == GTK_BUTTON(xe->render_selection_btn)), False );
		ASImage *im ;

		if( text == NULL ) 
			return ;   
		im = compose_asimage_xml(asv, get_screen_image_manager(NULL), get_screen_font_manager(NULL), text, ASFLAGS_EVERYTHING, False, None, NULL);
		g_free( text );
		if( im ) 
		{
			if( xe->entry->preview ) 
				safe_asimage_destroy(xe->entry->preview);	  
			xe->entry->preview = im ;
			asgtk_image_view_refresh( xe->image_view, False );
		}else
		{
			xml_editor_warning( GTK_WIDGET(xe), "Failed to render image from changed xml.", NULL, NULL ); 	   				
		}		 
	}
	gtk_widget_set_sensitive( xe->refresh_btn, ( button == GTK_BUTTON(xe->render_selection_btn) ) );
}

static void
on_validate_clicked(GtkButton *button, gpointer user_data)
{
	ASGtkXMLEditor *xe = ASGTK_XML_EDITOR(user_data);
	char *text = get_validated_text( xe, True, True );
	if( text ) 
		g_free( text );
}


static Bool
save_text_buffer_to_file( ASGtkXMLEditor *xe, const char *filename ) 
{
	char *text = get_xml_editor_text(xe, FALSE);
	FILE *fp = fopen( filename, "wb" );
	Bool result = False ; 
	if( fp ) 
	{	
		int len = strlen(text);
		if( fwrite( text, 1, len, fp ) == len ) 
			result = True;
		else
			xml_editor_warning( GTK_WIDGET(xe), "Failed to write to file \"%s\" : %s.", filename, g_strerror (errno) ); 	   				   		   
		fclose( fp ) ;
		result = True ; 
	}else
	{
		xml_editor_warning( GTK_WIDGET(xe), "Failed to open file \"%s\" : %s.", filename, g_strerror (errno) ); 	   				   		
	}	 
	free( text );
	return result ;
}


static void
on_save_clicked(GtkButton *button, gpointer user_data)
{
	ASGtkXMLEditor *xe = ASGTK_XML_EDITOR(user_data);
	char *filename = xe->entry->fullfilename ; 
	Bool signal_file_change = False ;
	Bool new_file = False;

	if( button == GTK_BUTTON(xe->save_as_btn) ) 
	{
		GtkWidget *filesel = gtk_file_selection_new( "Save as : ");
	 	gtk_file_selection_set_filename( GTK_FILE_SELECTION(filesel), filename );
		if( gtk_dialog_run( GTK_DIALOG(filesel)) == GTK_RESPONSE_CANCEL )
			filename = NULL ; 
		else
			filename = mystrdup(gtk_file_selection_get_filename( GTK_FILE_SELECTION(filesel)));
		gtk_widget_destroy( filesel );
	}	 
	if( filename ) 
	{	
		if( filename != xe->entry->fullfilename && 
			strcmp(filename, xe->entry->fullfilename ) != 0 ) 
		{
			if( CheckFile( filename ) == 0 ) 
			{	
				if( asgtk_yes_no_question1( GTK_WIDGET(xe), "File with the name \"%s\" already exists. Overwrite it ?", filename ) )
				{
					unlink( filename );
				}else
				{
					free( filename );
					return;	  
				}						   
			}
			new_file = True ;
		}
		if( save_text_buffer_to_file( xe, filename ) )
		{	
			xe->dirty = False ;
			gtk_widget_set_sensitive( xe->save_btn, FALSE );
			signal_file_change = True ;
		}
		if( filename != xe->entry->fullfilename ) 
			free( filename );
	}

	if( signal_file_change ) 
		if( xe->file_change_handler ) 
			xe->file_change_handler(xe, xe->file_change_user_data, new_file);

}

static void        
on_text_changed  (GtkTextBuffer *textbuffer, gpointer user_data)
{
	ASGtkXMLEditor *xe = ASGTK_XML_EDITOR(user_data);
	
	xe->dirty = True ; 
	gtk_widget_set_sensitive( xe->refresh_btn, TRUE );
	gtk_widget_set_sensitive( xe->save_btn, TRUE );
}

static void 
asgtk_xml_editor_screen_aspect_toggle( GtkWidget *checkbutton, gpointer data )
{
  	ASGtkXMLEditor *xe = ASGTK_XML_EDITOR (data);
	if( GTK_TOGGLE_BUTTON (checkbutton)->active ) 
	{	
		asgtk_image_view_set_aspect ( xe->image_view,
								   	  get_screen_width(NULL), get_screen_height(NULL) );
		asgtk_image_view_set_resize ( xe->image_view, 
									  ASGTK_IMAGE_VIEW_TILE_TO_ASPECT, 
									  ASGTK_IMAGE_VIEW_TILE_TO_ASPECT );
	}else
	{
		asgtk_image_view_set_aspect ( xe->image_view, -1, -1 );
		asgtk_image_view_set_resize ( xe->image_view, 0, ASGTK_IMAGE_VIEW_TILE_TO_ASPECT );
	}	 
}

static void 
asgtk_xml_editor_scale_to_view_toggle( GtkWidget *checkbutton, gpointer data )
{
  	ASGtkXMLEditor *xe = ASGTK_XML_EDITOR (data);
	if( GTK_TOGGLE_BUTTON (checkbutton)->active ) 
		asgtk_image_view_set_resize ( xe->image_view, ASGTK_IMAGE_VIEW_SCALE_TO_VIEW, 
									  				  ASGTK_IMAGE_VIEW_SCALE_TO_VIEW );
	else
		asgtk_image_view_set_resize ( xe->image_view, 0, ASGTK_IMAGE_VIEW_SCALE_TO_VIEW);
}

static void 
check_save_changes( ASGtkXMLEditor *xe ) 
{
	if( xe->dirty )
	{
		xe->dirty = False ;
		if( asgtk_yes_no_question1( GTK_WIDGET(xe), "Contents of the xml script \"%s\" changed. Save changes ?", xe->entry->name ) )
			save_text_buffer_to_file( xe, xe->entry->fullfilename );
	}	 
}

#define MAX_ASXML_SCRIPT_TAG	22

static char *ASXMLScriptTagsHelp[MAX_ASXML_SCRIPT_TAG] = 
{
	"renders text string into new image, using specific font, size and texture."
,
	"superimposes arbitrary number of images on top of each other. Compositing \"op\" could be one of the follwoing : \n"
	"add, alphablend, allanon, colorize, darken, diff, dissipate, hue, lighten, overlay, saturate, screen, sub, tint, value.\n"
	"All tags surrounded by this tag may have some of the common attributes in addition to their normal ones : \n"
	"crefid=\"ref_id\" (If set, percentages in \"x\" and \"y\" will be "
	"derived from the width and height of the crefid image.) "
	"x=\"child_offset_x\" y=\"child_offset_y\" align=\"right|center|left\" "
	"valign=\"top|middle|bottom\" clip_x=\"child_origin_x\" clip_y=\"child_origin_y\" "
	"clip_width=\"child_tile_width\" clip_height=\"child_tile_height\" tile=\"0|1\" tint=\"color\"."
,
	"loads image from the file. If src=\"xroot\" root background image is used."
,
	"recall previously generated and named image by its id."
,
	"releases(destroy if possible) previously generated and named image by its id."
,
	"defines symbolic name for a color and set of variables. \n"
	"In addition to defining symbolic name for the color this tag will define 7 other variables :\n"
	" domain.sym_name.red, domain.sym_name.green, domain.sym_name.blue, domain.sym_name.alpha,"
 	" domain.sym_name.hue, domain.sym_name.saturation, domain.sym_name.value ."
,
	"prints variable value to standard output.\n"
	"format_string is a Standard C format string with exactly 1 placeholder; "
	"var is a name of the variable, which value will be printed and val is a math expression to be printed."
,
	"renders multipoint gradient.\n"
	"offsets are represented as floating point values in the range of 0.0 to 1.0. If offsets are not given, a smooth"
	" stepping from 0.0 to 1.0 will be used.\n"
	" angle is given in degrees. Default is 0. Supported values are 0, 45, 90, 135, 180, 225, 270, 315."
,
	"generates image of specified size and fill it with solid or semitransparent color. Opacity ranges from 0 to 100 with 100 being completely opaque."
,
	"writes generated/loaded image into the file of one of the supported formats: jpg, bmp, xcf, png, xpm, tiff.\n"
	"compress value is relevant to jpg format (0 to 100) or png format (0 to 10)."
,
	"sets image's background color. Applies to first child only."
,
	"performs a gaussian blurr on an image. It is possible to selectively blur only some of the channels : a-alpha, r-red, g-green, b-blue. Applies to first child only."
,
	"draws solid or semitransparent bevel frame around the image. Applies to first child only."
,
	"creates new image as mirror copy of an old one."
,
	"rotates an image in 90 degree increments (flip)."
,
	"scales image to arbitrary size. \n"
	"If you want to keep image proportions while scaling - use \"proportional\" instead of specific size for particular dimention."
,
	"slices image to arbitrary size leaving corners unchanged.\n"
	"Contents of the image between x_start and x_end will be tiled horizontally. "
	"Contents of the image between y_start and y_end will be tiled vertically. "
	"This is usefull to get background images to fit the size of the text or a widget, while preserving its borders undistorted, "
	"which is the usuall result of simple scaling.\n"
	"If you want to keep image proportions while resizing-use \"proportional\" instead of specific size for particular dimention."
,
	"crops image to arbitrary area within it, optionally tinting it to specific color."
,
	"tiles an image to specified area, optionally tinting it to specific color."
,
	"adjusts Hue, Saturation and/or Value of an image and optionally tile an image to arbitrary area.\n"
	"Hue is measured in degrees from 0 to 360, saturation and value are measured from 0 to 100. One of the offsets must be not 0 for this tag to actually work."
,
	"pads an image with solid color rectangles."
,
	NULL
};
static char *ASXMLScriptTags[MAX_ASXML_SCRIPT_TAG][2] = 
{
{ 	"text", "\n<text id=\"new_id\" font=\"font\" point=\"size\" fgcolor=\"color\""
  			" bgcolor=\"color\" fgimage=\"image_id\" bgimage=\"image_id\" "
  			" spacing=\"points\">Insert Text here</text>\n" },
{	"composite","\n<composite id=\"new_id\" op=\"op_desc\" keep-transparency=\"0|1\" merge=\"0|1\">\n</composite>\n"},
{	"img","<img id=\"new_img_id\" src=\"filename\"/>"},
{	"recall","<recall id=\"new_id\" srcid=\"image_id\"/>"},
{	"release", "<release srcid=\"image_id\"/>"},
{	"color", "\n<color name=\"sym_name\" domain=\"var_domain\" argb=\"colorvalue\"/>\n" },
{	"printf", "\n<printf format=\"format_string\" var=\"variable_name\" val=\"value\"/>\n" },
{	"gradient", "\n<gradient id=\"new_id\" angle=\"degrees\" refid=\"refid\""
				" width=\"pixels\" height=\"pixels\" colors =\"color1 [...]\""
				" offsets=\"fraction1 [...]\"/>\n" },
{	"solid", "\n<solid id=\"new_id\" color=\"color\" opacity=\"opacity\""
 			 " width=\"pixels\" height=\"pixels\" refid=\"refid\"/>\n"},
{	"save", "\n<save id=\"new_id\" dst=\"filename\" format=\"format\" compress=\"value\""
 			" opacity=\"value\" replace=\"0|1\" delay=\"mlsecs\">\n</save>\n"},
{	"background", "\n<background id=\"new_id\" color=\"color\">\n</background>\n"},
{	"blur", "\n<blur id=\"new_id\" horz=\"radius\" vert=\"radius\" channels=\"argb\">\n</blur>\n"},
{	"bevel", "\n<bevel id=\"new_id\" colors=\"color1 color2\" width=\"pixels\""
			 " height=\"pixels\" refid=\"refid\" border=\"left top right bottom\" solid=0|1>\n</bevel>\n"},
{	"mirror","\n<mirror id=\"new_id\" dir=\"vertical|horizontal\" width=\"pixels\" height=\"pixels\" refid=\"refid\">\n</mirror>\n" },
{	"rotate","\n<rotate id=\"new_id\" angle=\"90|180|270\" width=\"pixels\" height=\"pixels\" refid=\"refid\">\n</rotate>\n"},
{	"scale", "\n<scale id=\"new_id\" ref_id=\"other_imag\" width=\"pixels\" height=\"pixels\">\n</scale>\n"},
{	"slice", "\n<slice id=\"new_id\" ref_id=\"other_imag\" width=\"pixels\" height=\"pixels\""
			 " x_start=\"slice_x_start\" x_end=\"slice_x_end\" y_start=\"slice_y_start\""
			 " y_end=\"slice_y_end\">\n</slice>\n"},
{	"crop",  "\n<crop id=\"new_id\" refid=\"other_image\" srcx=\"pixels\" srcy=\"pixels\""
 			 " width=\"pixels\" height=\"pixels\" tint=\"color\">\n</crop>\n"},
{	"tile",  "\n<tile id=\"new_id\" refid=\"other_image\" width=\"pixels\" height=\"pixels\""
 			 " x_origin=\"pixels\" y_origin=\"pixels\" tint=\"color\" complement=0|1>\n</tile>\n"},
{	"hsv",   "\n<hsv id=\"new_id\" refid=\"other_image\" x_origin=\"pixels\" y_origin=\"pixels\""
			 " width=\"pixels\" height=\"pixels\" affected_hue=\"degrees|color\""
			 " affected_radius=\"degrees\" hue_offset=\"degrees\""
			 " saturation_offset=\"value\" value_offset=\"value\">\n</hsv>\n"},
{	"pad",   "\n<pad id=\"new_id\" left=\"pixels\" top=\"pixels\""
		     " right=\"pixels\" bottom=\"pixels\" color=\"color\""
			 " refid=\"refid\" width=\"pixels\" height=\"pixels\">\n</pad>\n"},
{	NULL, NULL }
};	 

static GtkWidget*
make_xml_tags_list( ASGtkXMLEditor *xe )
{
 	GtkWidget *scrolled_window ; 		
	GtkTreeSelection *selection;
	GtkTreeModel *tree_model ; 
	GtkCellRenderer 	*cell;
    GtkTreeViewColumn 	*column;
	GtkTreeIter iter;
	GtkListStore *store ;
	int i ;

	
	xe->tags_list = GTK_TREE_VIEW(gtk_tree_view_new());
	tree_model = GTK_TREE_MODEL(gtk_list_store_new (3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING));
	
	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_size_request (scrolled_window, 100, 200);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
				    				GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC );
	gtk_widget_show(scrolled_window);
    gtk_container_add (GTK_CONTAINER(scrolled_window), GTK_WIDGET(xe->tags_list));
    gtk_tree_view_set_model (xe->tags_list, tree_model);
    gtk_widget_show (GTK_WIDGET(xe->tags_list));
    cell = gtk_cell_renderer_text_new ();

    column = gtk_tree_view_column_new_with_attributes ("supported tags:", cell, "text", 0, NULL);
    gtk_tree_view_append_column (xe->tags_list, GTK_TREE_VIEW_COLUMN (column));
	
	selection = gtk_tree_view_get_selection(xe->tags_list);
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
   	//g_signal_connect (selection, "changed",  G_CALLBACK (asgtk_image_dir_sel_handler), id);
	g_signal_connect ( xe->tags_list, "row_activated",
						G_CALLBACK (asgtk_xml_editor_tag_activate), xe);
	
	g_signal_connect (gtk_tree_view_get_selection (GTK_TREE_VIEW (xe->tags_list)), "changed",
		    		  G_CALLBACK (asgtk_xml_editor_tag_selected), xe);

	colorize_gtk_tree_view_window( scrolled_window );
	
	store = GTK_LIST_STORE(gtk_tree_view_get_model(xe->tags_list));
	for( i = 0 ; ASXMLScriptTags[i][0] ; ++i ) 
	{	
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, 
							0, ASXMLScriptTags[i][0], 
							1, ASXMLScriptTags[i][1], 
							2, ASXMLScriptTagsHelp[i]?ASXMLScriptTagsHelp[i]:"",
							-1);
	}

	return scrolled_window;
}

static void
insert_tag_template_at_cursor( ASGtkXMLEditor *xe, const char *text )
{
	GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (xe->text_view));
	GtkTextMark *mark ; 
	GtkTextIter start, end, line_start ;
	int text_len = strlen(text);
	int start_mark ;
	int start_line_no ;
	char *start_text, *filler = NULL ; 
	int i, curr_line_start, filler_len = 0 ;
	Bool skip_leading_eol = False  ;

	mark = gtk_text_buffer_get_insert( buffer );
	gtk_text_buffer_get_iter_at_mark( buffer, &start, mark );
	start_line_no = gtk_text_iter_get_line( &start );
	gtk_text_buffer_get_iter_at_line( buffer, &line_start, start_line_no );

	start_text = gtk_text_iter_get_text( &line_start, &start );
	if( start_text ) 
	{
		for( i = 0 ; start_text[i] != '\0' ; ++i ) 
			if( !isspace(start_text[i]) )
				break;
		if( start_text[i] == '\0' ) 
			skip_leading_eol = True ;
		if( i > 0 ) 
		{
			filler_len = i ;
			filler = mystrndup(start_text, i );
		}
		free( start_text ); 	
	}	 

	start_mark = gtk_text_iter_get_offset(&start);
	curr_line_start = 0 ;
	for( i = 0 ; text[i] != '\0' ; ++i ) 
	{
		if( text[i] == '\n' ) 
		{
			if( !skip_leading_eol || i > 0 )
			{	
				gtk_text_buffer_insert_at_cursor( buffer, &text[curr_line_start], i+1-curr_line_start);	
				if( filler ) 
				{	
					gtk_text_buffer_insert_at_cursor( buffer, filler, filler_len);	  
					text_len += filler_len ;
				}
			}
			curr_line_start = i + 1 ; 
		}	 
	}	 
	if( i > curr_line_start ) 
		gtk_text_buffer_insert_at_cursor( buffer, &text[curr_line_start], i-curr_line_start);	
	
	gtk_text_buffer_get_iter_at_offset( buffer, &start, start_mark );
	gtk_text_buffer_get_iter_at_offset( buffer, &end, start_mark+text_len );
	gtk_text_buffer_select_range( buffer, &start, &end);

}
	

static void
asgtk_xml_editor_tag_activate ( GtkTreeView       *tree_view,
	   						    GtkTreePath       *path,
				 				GtkTreeViewColumn *column,
				 				gpointer           user_data)
{
	ASGtkXMLEditor *xe = ASGTK_XML_EDITOR (user_data);
  	GtkTreeModel *model = gtk_tree_view_get_model (tree_view);
  	GtkTreeIter iter;
  	char *tag_template = NULL ;

  	gtk_tree_model_get_iter (model, &iter, path);
  	gtk_tree_model_get (model, &iter, 1, &tag_template, -1);
	if( tag_template ) 
		insert_tag_template_at_cursor( xe, tag_template );	  
}

static void
asgtk_xml_editor_tag_selected ( GtkTreeSelection *selection, gpointer user_data)
{
	ASGtkXMLEditor *xe = ASGTK_XML_EDITOR (user_data);
  	GtkTreeModel *model;
  	GtkTreeIter iter;
	char *tag_template = NULL ;
  	char *tag_help = NULL ;

  	if (gtk_tree_selection_get_selected (selection, &model, &iter)) 
	{
		GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (xe->help_view));
		int tag_template_len = 0;
	  	gtk_tree_model_get (model, &iter, 1, &tag_template, 2, &tag_help, -1);
		if( tag_template ) 
		{
			if( tag_template[0] == '\n' ) 
				++tag_template;
			tag_template_len = strlen(tag_template);
			gtk_text_buffer_set_text( buffer, tag_template, tag_template_len );
		}
		if( tag_help ) 
		{
			if( tag_template_len == 0 ) 
				gtk_text_buffer_set_text( buffer, tag_help, strlen(tag_help));
			else
			{
				GtkTextIter iter ; 
				gtk_text_buffer_get_iter_at_offset( buffer, &iter, tag_template_len );
				gtk_text_buffer_insert( buffer, &iter, tag_help, strlen(tag_help)); 	 
			}	 
		}		
	}
}


static void 
on_insert_tag_clicked( GtkWidget *button, gpointer data )
{
  	ASGtkXMLEditor *xe = ASGTK_XML_EDITOR (data);
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GtkTreeModel *model;

	selection = gtk_tree_view_get_selection(xe->tags_list);
  	if (gtk_tree_selection_get_selected (selection, &model, &iter)) 
	{
		gpointer p = NULL ;
    	gtk_tree_model_get (model, &iter, 1, &p, -1);
		if( p != NULL ) 
		{
			insert_tag_template_at_cursor( xe, p );	  
		}	
  	}
}

/*  public functions  */
GtkWidget *
asgtk_xml_editor_new ()
{
	ASGtkXMLEditor *xe;
	GtkWidget *main_vbox; 
	GtkWidget *scrolled_window; 
	GtkWidget *panes ; 
	GtkWidget *scale_check_box ;
	GtkWidget *edit_hbox, *tags_vbox, *edit_vpanes, *save_hbox; 
	GtkWidget *insert_tag_btn ;
  	
    xe = g_object_new (ASGTK_TYPE_XML_EDITOR, NULL);

	gtk_container_set_border_width( GTK_CONTAINER (xe), 3 );
	main_vbox = gtk_vbox_new (FALSE, 0);
  	gtk_widget_show (main_vbox);
	gtk_container_add (GTK_CONTAINER (xe), main_vbox);
	gtk_box_set_spacing( GTK_BOX(main_vbox), 3 );

	panes = gtk_vpaned_new();
	gtk_widget_show( panes );
	gtk_box_pack_start (GTK_BOX (main_vbox), panes, TRUE, TRUE, 0);

	xe->image_view = ASGTK_IMAGE_VIEW(asgtk_image_view_new_horizontal());
	gtk_widget_set_size_request (GTK_WIDGET(xe->image_view), 320, 240);
	gtk_paned_pack1 (GTK_PANED (panes), GTK_WIDGET(xe->image_view), TRUE, TRUE);
	gtk_widget_show (GTK_WIDGET(xe->image_view));
	asgtk_image_view_set_aspect ( xe->image_view,
							   	  get_screen_width(NULL), get_screen_height(NULL) );
	asgtk_image_view_set_resize ( xe->image_view, 
								  ASGTK_IMAGE_VIEW_SCALE_TO_VIEW|
							 	  ASGTK_IMAGE_VIEW_TILE_TO_ASPECT, 
								  ASGTK_IMAGE_VIEW_RESIZE_ALL );

	/* text editor controls : */
	edit_hbox = gtk_hbox_new (FALSE, 0);
  	gtk_widget_show (edit_hbox);
	gtk_box_set_spacing( GTK_BOX(edit_hbox), 6 );
	tags_vbox = gtk_vbox_new (FALSE, 0);
  	gtk_widget_show (tags_vbox);
	gtk_box_set_spacing( GTK_BOX(tags_vbox), 3 );
	edit_vpanes = gtk_vpaned_new ();
  	gtk_widget_show (edit_vpanes);

	gtk_box_pack_start (GTK_BOX (edit_hbox), tags_vbox, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (edit_hbox), edit_vpanes, TRUE, TRUE, 0);

	scrolled_window = make_xml_tags_list( ASGTK_XML_EDITOR(xe) );
	gtk_box_pack_start (GTK_BOX (tags_vbox), scrolled_window, TRUE, TRUE, 0);
	gtk_scrolled_window_set_shadow_type( GTK_SCROLLED_WINDOW(scrolled_window), GTK_SHADOW_IN );
	insert_tag_btn = asgtk_add_button_to_box( NULL, GTK_STOCK_ADD, "Insert Tag template", G_CALLBACK(on_insert_tag_clicked), xe );
	gtk_box_pack_end (GTK_BOX (tags_vbox), insert_tag_btn, FALSE, FALSE, 0);
		
	xe->text_view = gtk_text_view_new ();
	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_size_request (scrolled_window, 400, 200);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
				    				GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    gtk_container_add (GTK_CONTAINER(scrolled_window), GTK_WIDGET(xe->text_view));
	gtk_widget_show( scrolled_window );
	gtk_widget_show( xe->text_view );
	gtk_scrolled_window_set_shadow_type( GTK_SCROLLED_WINDOW(scrolled_window), GTK_SHADOW_IN );

	gtk_paned_pack1 (GTK_PANED (edit_vpanes), scrolled_window, TRUE, TRUE);
	
	xe->help_view = gtk_text_view_new ();
	gtk_text_view_set_wrap_mode( GTK_TEXT_VIEW(xe->help_view), GTK_WRAP_WORD );
	gtk_text_view_set_editable( GTK_TEXT_VIEW(xe->help_view), FALSE );
	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_size_request (scrolled_window, 600, 50);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
				    				GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

    gtk_container_add (GTK_CONTAINER(scrolled_window), GTK_WIDGET(xe->help_view));
	gtk_widget_show( scrolled_window );
	gtk_widget_show( xe->help_view );
	gtk_scrolled_window_set_shadow_type( GTK_SCROLLED_WINDOW(scrolled_window), GTK_SHADOW_IN );
	colorize_gtk_widget( scrolled_window, get_colorschemed_style_normal() );

	gtk_paned_pack2 (GTK_PANED (edit_vpanes), scrolled_window, FALSE, FALSE);

	gtk_paned_pack2 (GTK_PANED (panes), edit_hbox, TRUE, TRUE);

	colorize_gtk_window( GTK_WIDGET(xe) );


	/* buttons : */

	save_hbox = gtk_hbutton_box_new ();
  	gtk_widget_show (save_hbox);

	xe->render_selection_btn = asgtk_add_button_to_box( NULL, GTK_STOCK_SAVE_AS, "Render Selection", G_CALLBACK(on_refresh_clicked), xe ) ; 
	xe->refresh_btn = asgtk_add_button_to_box( NULL, GTK_STOCK_REFRESH, NULL, G_CALLBACK(on_refresh_clicked), xe );
	xe->validate_btn = asgtk_add_button_to_box( NULL, GTK_STOCK_REFRESH, "Validate XML", G_CALLBACK(on_validate_clicked), xe ) ; 
	xe->save_btn = asgtk_add_button_to_box( GTK_BOX(save_hbox), GTK_STOCK_SAVE, NULL, G_CALLBACK(on_save_clicked), xe );
	xe->save_as_btn = asgtk_add_button_to_box( GTK_BOX(save_hbox), GTK_STOCK_SAVE_AS, NULL, G_CALLBACK(on_save_clicked), xe );

	gtk_widget_set_sensitive( xe->refresh_btn, FALSE );
	gtk_widget_set_sensitive( xe->save_btn, FALSE );

	asgtk_image_view_add_tool( xe->image_view, save_hbox, 0 ); 
	asgtk_image_view_add_tool( xe->image_view, xe->validate_btn, 3 );
	asgtk_image_view_add_tool( xe->image_view, xe->render_selection_btn, 3 );
	asgtk_image_view_add_tool( xe->image_view, xe->refresh_btn, 3 );
	//asgtk_image_view_add_tool( xe->image_view, xe->save_btn, 3 );
	//asgtk_image_view_add_tool( xe->image_view, xe->save_as_btn, 0 );

   	g_signal_connect (gtk_text_view_get_buffer (GTK_TEXT_VIEW (xe->text_view)), 
					  "changed",  G_CALLBACK (on_text_changed), xe);


	scale_check_box = gtk_check_button_new_with_label( "Use screen aspect ratio" );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(scale_check_box), TRUE );
	gtk_widget_show (scale_check_box);
	colorize_gtk_widget( scale_check_box, get_colorschemed_style_normal() );  
	g_signal_connect (G_OBJECT (scale_check_box), "toggled",
	              	  G_CALLBACK (asgtk_xml_editor_screen_aspect_toggle), (gpointer) xe);
	gtk_button_set_alignment( GTK_BUTTON(scale_check_box), 1.0, 0.5);
	asgtk_image_view_add_detail( xe->image_view, scale_check_box, 0 );

	scale_check_box = gtk_check_button_new_with_label( "Scale to fit this view" );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(scale_check_box), TRUE );
	gtk_widget_show (scale_check_box);
	colorize_gtk_widget( scale_check_box, get_colorschemed_style_normal() );  
	g_signal_connect (G_OBJECT (scale_check_box), "toggled",
	              	  G_CALLBACK (asgtk_xml_editor_scale_to_view_toggle), (gpointer) xe);
	gtk_button_set_alignment( GTK_BUTTON(scale_check_box), 1.0, 0.5);
	asgtk_image_view_add_detail( xe->image_view, scale_check_box, 0 );
	LOCAL_DEBUG_OUT( "created image ASGtkXMLEditor object %p", xe );	
	return GTK_WIDGET (xe);
}

void   
asgtk_xml_editor_set_entry ( ASGtkXMLEditor *xe,
                             struct ASImageListEntry *entry)
{
	GtkTextBuffer *buffer;

	g_return_if_fail (ASGTK_IS_XML_EDITOR (xe));

	LOCAL_DEBUG_OUT( " ASGtk XML Editor object's %p entry to %p", xe, entry );
	check_save_changes( xe ); 
	
	unref_asimage_list_entry(xe->entry);

	if( entry && entry->type != ASIT_XMLScript )
		entry = NULL ; 
	
	LOCAL_DEBUG_OUT( " entry ref_count = %d", entry->ref_count );
	xe->entry = ref_asimage_list_entry(entry) ;
	LOCAL_DEBUG_OUT( " entry ref_count = %d", entry->ref_count );
	asgtk_image_view_set_entry ( xe->image_view, entry );
	LOCAL_DEBUG_OUT( " entry ref_count = %d", entry->ref_count );

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (xe->text_view));
	if (xe->entry)
    {
#if 1	
		char * text = load_file( xe->entry->fullfilename );
		gtk_text_buffer_set_text (buffer, text, -1);
#endif
	}else
	{	
		gtk_text_buffer_set_text (buffer, NULL, 0);
	}

	xe->dirty = False ; 
	gtk_widget_set_sensitive( xe->refresh_btn, FALSE );
	gtk_widget_set_sensitive( xe->save_btn, FALSE );

}

void  		
asgtk_xml_editor_file_change_handler( ASGtkXMLEditor *xe, 
									  _ASGtkXMLEditor_handler change_handler, gpointer user_data )
{
	g_return_if_fail (ASGTK_IS_XML_EDITOR (xe));

	xe->file_change_handler = change_handler ; 
	xe->file_change_user_data = user_data ;
}



