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

#define ASGTK_SCROLLED_WINDOW(h_policy,v_policy,shadow) \
	({GtkScrolledWindow *__asgtk_tmpvar_scrolled_window = GTK_SCROLLED_WINDOW(gtk_scrolled_window_new(NULL,NULL)); \
	  	gtk_scrolled_window_set_shadow_type (__asgtk_tmpvar_scrolled_window, (shadow));  \
  		gtk_scrolled_window_set_policy (__asgtk_tmpvar_scrolled_window, (h_policy), (v_policy)); \
	  	gtk_container_set_border_width (GTK_CONTAINER (__asgtk_tmpvar_scrolled_window), 0); \
		GTK_WIDGET(__asgtk_tmpvar_scrolled_window);})

#define ASGTK_CONTAINER_ADD(cont_widget,widget) \
	do{	gtk_container_add (GTK_CONTAINER (cont_widget), GTK_WIDGET(widget)); \
		gtk_widget_show(GTK_WIDGET(widget)); }while(0)

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
