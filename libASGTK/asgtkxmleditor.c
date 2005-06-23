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
				if( asgtk_yes_no_question1( GTK_WIDGET(xe), "File with the name \"%s\" already exists. Overwrite it ?", filename ) )
					unlink( filename );
		}
		if( save_text_buffer_to_file( xe, filename ) )
		{	
			xe->dirty = False ;
			gtk_widget_set_sensitive( xe->save_btn, FALSE );
			gtk_widget_set_sensitive( xe->save_as_btn, FALSE );
		}
		if( filename != xe->entry->fullfilename ) 
			free( filename );
	}
}

static void        
on_text_changed  (GtkTextBuffer *textbuffer, gpointer user_data)
{
	ASGtkXMLEditor *xe = ASGTK_XML_EDITOR(user_data);
	
	xe->dirty = True ; 
	gtk_widget_set_sensitive( xe->refresh_btn, TRUE );
	gtk_widget_set_sensitive( xe->save_btn, TRUE );
	gtk_widget_set_sensitive( xe->save_as_btn, TRUE );
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


/*  public functions  */
GtkWidget *
asgtk_xml_editor_new ()
{
	ASGtkXMLEditor *xe;
	GtkWidget *main_vbox; 
	GtkWidget *scrolled_window ; 
	GtkWidget *panes ; 
	GtkWidget *scale_check_box ;
  	
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
	gtk_widget_set_size_request (GTK_WIDGET(xe->image_view), 400, 300);
	gtk_paned_pack1 (GTK_PANED (panes), GTK_WIDGET(xe->image_view), TRUE, TRUE);
	gtk_widget_show (GTK_WIDGET(xe->image_view));
	asgtk_image_view_set_aspect ( xe->image_view,
							   	  get_screen_width(NULL), get_screen_height(NULL) );
	asgtk_image_view_set_resize ( xe->image_view, 
								  ASGTK_IMAGE_VIEW_SCALE_TO_VIEW|
							 	  ASGTK_IMAGE_VIEW_TILE_TO_ASPECT, 
								  ASGTK_IMAGE_VIEW_RESIZE_ALL );
	   
	xe->text_view = gtk_text_view_new ();
	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_size_request (scrolled_window, 600, 300);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
				    				GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_window), GTK_WIDGET(xe->text_view));
	gtk_widget_show( scrolled_window );
	gtk_widget_show( xe->text_view );

	gtk_paned_pack2 (GTK_PANED (panes), scrolled_window, TRUE, TRUE);
    
	colorize_gtk_window( GTK_WIDGET(xe) );



	xe->refresh_btn = asgtk_add_button_to_box( NULL, GTK_STOCK_REFRESH, NULL, G_CALLBACK(on_refresh_clicked), xe );
	xe->save_btn = asgtk_add_button_to_box( NULL, GTK_STOCK_SAVE, NULL, G_CALLBACK(on_save_clicked), xe );
	xe->save_as_btn = asgtk_add_button_to_box( NULL, GTK_STOCK_SAVE_AS, NULL, G_CALLBACK(on_save_clicked), xe );
	xe->render_selection_btn = asgtk_add_button_to_box( NULL, GTK_STOCK_SAVE_AS, "Render Selection", G_CALLBACK(on_refresh_clicked), xe ) ; 
	xe->validate_btn = asgtk_add_button_to_box( NULL, GTK_STOCK_REFRESH, "Validate XML", G_CALLBACK(on_validate_clicked), xe ) ; 

	gtk_widget_set_sensitive( xe->refresh_btn, FALSE );
	gtk_widget_set_sensitive( xe->save_btn, FALSE );
	gtk_widget_set_sensitive( xe->save_as_btn, FALSE );

	asgtk_image_view_add_tool( xe->image_view, xe->validate_btn, 0 );
	asgtk_image_view_add_tool( xe->image_view, xe->render_selection_btn, 3 );
	asgtk_image_view_add_tool( xe->image_view, xe->refresh_btn, 3 );
	asgtk_image_view_add_tool( xe->image_view, xe->save_btn, 3 );
	asgtk_image_view_add_tool( xe->image_view, xe->save_as_btn, 0 );

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
	gtk_widget_set_sensitive( xe->save_as_btn, FALSE );

}



