#ifndef ASGTK_H_HEADER_INCLUDED
#define ASGTK_H_HEADER_INCLUDED

#include <gdk/gdk.h>	   
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#define ASGTK_PACK_BOX 									__asgtk_tmpvar_box
#define ASGTK_PACK_BEGIN(cont_widget) 				do{ GtkBox *ASGTK_PACK_BOX = GTK_BOX(cont_widget)
#define ASGTK_PACK_TO_START(widget,expand,fill,pad) 	gtk_box_pack_start (ASGTK_PACK_BOX, widget, expand, fill, pad)
#define ASGTK_PACK_TO_END(widget,expand,fill,pad)  		gtk_box_pack_end (ASGTK_PACK_BOX, widget, expand, fill, pad)
#define ASGTK_PACK_END 									gtk_widget_show_all (GTK_WIDGET(ASGTK_PACK_BOX));\
													}while(0)

#define ASGTK_TABLE_TMPVAR							__asgtk_tmpvar_table
#define ASGTK_TABLE_ROW_TMPVAR						__asgtk_tmpvar_table_row
#define ASGTK_TABLE_COL_TMPVAR						__asgtk_tmpvar_table_col
#define ASGTK_TABLE_BEGIN(cont_widget) 				do{ int ASGTK_TABLE_ROW_TMPVAR = 0 ; \
														GtkTable *ASGTK_TABLE_TMPVAR = GTK_TABLE(cont_widget)
#define ASGTK_ROW_BEGIN 							do{ int ASGTK_TABLE_COL_TMPVAR = 0;
#define ASGTK_TABLE_CELL(widget)						gtk_table_attach_defaults ( ASGTK_TABLE_TMPVAR, GTK_WIDGET(widget), \
																					ASGTK_TABLE_COL_TMPVAR, \
																					ASGTK_TABLE_COL_TMPVAR+1, \
																					ASGTK_TABLE_ROW_TMPVAR, \
																					ASGTK_TABLE_ROW_TMPVAR+1); \
														++ASGTK_TABLE_COL_TMPVAR
#define ASGTK_TABLE_CELL_SPAN(widget,span_x)			gtk_table_attach_defaults ( ASGTK_TABLE_TMPVAR, GTK_WIDGET(widget), \
																					ASGTK_TABLE_COL_TMPVAR, \
																					ASGTK_TABLE_COL_TMPVAR+(span_x), \
																					ASGTK_TABLE_ROW_TMPVAR, \
																					ASGTK_TABLE_ROW_TMPVAR+1); \
														ASGTK_TABLE_COL_TMPVAR+=(span_x)
#define ASGTK_TABLE_CELL_SPAN2D(widget,span_x,span_y)   gtk_table_attach_defaults ( ASGTK_TABLE_TMPVAR, GTK_WIDGET(widget), \
																					ASGTK_TABLE_COL_TMPVAR, \
																					ASGTK_TABLE_COL_TMPVAR+(span_x), \
																					ASGTK_TABLE_ROW_TMPVAR, \
																					ASGTK_TABLE_ROW_TMPVAR+(span_y)); \
														ASGTK_TABLE_COL_TMPVAR+=(span_x)

#define ASGTK_ROW_END									++ASGTK_TABLE_ROW_TMPVAR; }while(0)
														
#define ASGTK_TABLE_END 							gtk_widget_show_all (GTK_WIDGET(ASGTK_TABLE_TMPVAR));\
													}while(0)


#define ASGTK_SCROLLED_WINDOW(h_policy,v_policy,shadow) \
	({GtkScrolledWindow *__asgtk_tmpvar_scrolled_window = GTK_SCROLLED_WINDOW(gtk_scrolled_window_new(NULL,NULL)); \
	  	gtk_scrolled_window_set_shadow_type (__asgtk_tmpvar_scrolled_window, (shadow));  \
  		gtk_scrolled_window_set_policy (__asgtk_tmpvar_scrolled_window, (h_policy), (v_policy)); \
	  	gtk_container_set_border_width (GTK_CONTAINER (__asgtk_tmpvar_scrolled_window), 0); \
		GTK_WIDGET(__asgtk_tmpvar_scrolled_window);})

#define ASGTK_ALIGNED_LABEL(text,x_align,y_align) \
	({GtkMisc *__asgtk_tmpvar_label = GTK_MISC(gtk_label_new(text)); \
		gtk_misc_set_alignment( __asgtk_tmpvar_label, (x_align), (y_align)); \
		GTK_WIDGET(__asgtk_tmpvar_label);})

#define ASGTK_CONTAINER_ADD(cont_widget,widget) \
	do{	GtkWidget *__asgtk_tmpvar_widget = GTK_WIDGET(widget); \
		gtk_container_add (GTK_CONTAINER (cont_widget), __asgtk_tmpvar_widget); \
		gtk_widget_show(__asgtk_tmpvar_widget); }while(0)

#define ASGTK_CONTAINER_CONFIG(cont_widget,width,height,bw) \
	do{ gtk_container_set_border_width(GTK_CONTAINER (cont_widget), (bw)); \
	  	gtk_widget_set_size_request(GTK_WIDGET(cont_widget),(width),(height)); }while(0)
		


#include "asgtkapp.h"
#include "asgtkimageview.h"
#include "asgtkimagedir.h"
#include "asgtkimagebrowser.h"
#include "asgtkxmleditor.h"
#include "asgtkcolorsel.h"
#include "asgtkgradient.h"
#include "asgtkdirtree.h"

#endif /* #ifndef ASGTK_H_HEADER_INCLUDED */
