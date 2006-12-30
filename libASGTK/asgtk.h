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


#include "asgtkapp.h"
#include "asgtkimageview.h"
#include "asgtkimagedir.h"
#include "asgtkimagebrowser.h"
#include "asgtkxmleditor.h"
#include "asgtkcolorsel.h"
#include "asgtkgradient.h"
#include "asgtkdirtree.h"

#endif /* #ifndef ASGTK_H_HEADER_INCLUDED */
