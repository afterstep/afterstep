/****************************************************************************
 *
 * Copyright (c) 2005 Sasha Vasko <sasha at aftercode.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.   See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 ****************************************************************************/

#define LOCAL_DEBUG
#include "../configure.h"

#include "../libAfterStep/asapp.h"


#include "asgtk.h"

#include <gdk/gdkx.h>

#include "../libAfterStep/module.h"
#include "../libAfterConf/afterconf.h"

static void print_default_gtkrc_files ()
{
	char **rcfiles = gtk_rc_get_default_files ();

	if (rcfiles && rcfiles[0]) {
		int i = 0;

		while (rcfiles[i]) {
			show_progress ("GTK rcfile[%d] = \"%s\";", i, rcfiles[i]);
			++i;
		}
	} else
		show_progress ("no GTK rcfile defined;");
}


void
init_asgtkapp (int argc, char *argv[], const char *module_name,
							 void (*custom_usage_func) (void), ASFlagType opt_mask)
{
	GdkDisplay *gdk_display;
	int i;
	static char *deleted_arg = "_deleted_arg_";

	InitMyApp (module_name, argc, argv, NULL, custom_usage_func, opt_mask);
	for (i = 1; i < argc; ++i)
		if (argv[i] == NULL)
			argv[i] = strdup (deleted_arg);
	LinkAfterStepConfig ();
	InitSession ();

#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif

	gtk_set_locale ();
	gtk_init (&argc, &argv);

#if defined(LOCAL_DEBUG) && !defined(NO_DEBUG_OUTPUT)
	print_default_gtkrc_files ();
#endif

	gdk_display = gdk_display_get_default ();

	LoadColorScheme ();

	ConnectXDisplay (gdk_x11_display_get_xdisplay (gdk_display), NULL,
									 False);
	ReloadASEnvironment (NULL, NULL, NULL, False, True);
	/*MyArgs.display_name */
}


GdkColor *color_name2GdkColor (const char *name, GdkColor * color)
{
	ARGB32 argb;

	parse_argb_color (name, &argb);
	color->pixel = argb;
	color->red = ARGB32_RED16 (argb);
	color->green = ARGB32_GREEN16 (argb);
	color->blue = ARGB32_BLUE16 (argb);
	return color;
}

char *GdkColor2string (GdkColor * color, int alpha)
{
	char buffer[10];

	if (alpha >= 0)
		sprintf (&buffer[0], "#%2.2X%2.2X%2.2X%2.2X", alpha >> 8,
						 color->red >> 8, color->green >> 8, color->blue >> 8);
	else
		sprintf (&buffer[0], "#%2.2X%2.2X%2.2X", color->red >> 8,
						 color->green >> 8, color->blue >> 8);
	return mystrdup (&buffer[0]);
}

static GtkStyle *ASGtkStyleNormal = NULL;
static GtkStyle *ASGtkStyleButton = NULL;

GtkStyle *get_colorschemed_style_normal ()
{
	if (ASGtkStyleNormal == NULL) {
		ASGtkStyleNormal = gtk_style_new ();	/* gtk_style_copy(GTK_WIDGET(WallpaperState.main_window)->style);        */
		color_name2GdkColor ("InactiveText1",
												 &(ASGtkStyleNormal->fg[GTK_STATE_NORMAL]));
		color_name2GdkColor ("InactiveText2",
												 &(ASGtkStyleNormal->fg[GTK_STATE_ACTIVE]));
		color_name2GdkColor ("HighInactiveText",
												 &(ASGtkStyleNormal->fg[GTK_STATE_PRELIGHT]));
		color_name2GdkColor ("ActiveText",
												 &(ASGtkStyleNormal->fg[GTK_STATE_SELECTED]));
		color_name2GdkColor ("DisabledText",
												 &(ASGtkStyleNormal->fg[GTK_STATE_INSENSITIVE]));
		color_name2GdkColor ("Inactive1",
												 &(ASGtkStyleNormal->bg[GTK_STATE_NORMAL]));
		color_name2GdkColor ("Inactive2",
												 &(ASGtkStyleNormal->bg[GTK_STATE_ACTIVE]));
		color_name2GdkColor ("HighInactive",
												 &(ASGtkStyleNormal->bg[GTK_STATE_PRELIGHT]));
		color_name2GdkColor ("Active",
												 &(ASGtkStyleNormal->bg[GTK_STATE_SELECTED]));
		color_name2GdkColor ("Inactive1",
												 &(ASGtkStyleNormal->bg[GTK_STATE_INSENSITIVE]));
		color_name2GdkColor ("Inactive1Light",
												 &(ASGtkStyleNormal->light[GTK_STATE_NORMAL]));
		color_name2GdkColor ("Inactive2Light",
												 &(ASGtkStyleNormal->light[GTK_STATE_ACTIVE]));
		color_name2GdkColor ("HighInactiveLight",
												 &(ASGtkStyleNormal->light[GTK_STATE_PRELIGHT]));
		color_name2GdkColor ("ActiveLight",
												 &(ASGtkStyleNormal->light[GTK_STATE_SELECTED]));
		color_name2GdkColor ("Inactive1Light",
												 &(ASGtkStyleNormal->
													 light[GTK_STATE_INSENSITIVE]));
		color_name2GdkColor ("Inactive1Dark",
												 &(ASGtkStyleNormal->dark[GTK_STATE_NORMAL]));
		color_name2GdkColor ("Inactive2Dark",
												 &(ASGtkStyleNormal->dark[GTK_STATE_ACTIVE]));
		color_name2GdkColor ("HighInactiveDark",
												 &(ASGtkStyleNormal->dark[GTK_STATE_PRELIGHT]));
		color_name2GdkColor ("ActiveDark",
												 &(ASGtkStyleNormal->dark[GTK_STATE_SELECTED]));
		color_name2GdkColor ("Inactive1Dark",
												 &(ASGtkStyleNormal->dark[GTK_STATE_INSENSITIVE]));
		color_name2GdkColor ("Inactive1",
												 &(ASGtkStyleNormal->mid[GTK_STATE_NORMAL]));
		color_name2GdkColor ("Inactive2",
												 &(ASGtkStyleNormal->mid[GTK_STATE_ACTIVE]));
		color_name2GdkColor ("HighInactive",
												 &(ASGtkStyleNormal->mid[GTK_STATE_PRELIGHT]));
		color_name2GdkColor ("Active",
												 &(ASGtkStyleNormal->mid[GTK_STATE_SELECTED]));
		color_name2GdkColor ("Inactive1",
												 &(ASGtkStyleNormal->mid[GTK_STATE_INSENSITIVE]));
		color_name2GdkColor ("InactiveText1",
												 &(ASGtkStyleNormal->text[GTK_STATE_NORMAL]));
		color_name2GdkColor ("InactiveText2",
												 &(ASGtkStyleNormal->text[GTK_STATE_ACTIVE]));
		color_name2GdkColor ("HighInactiveText",
												 &(ASGtkStyleNormal->text[GTK_STATE_PRELIGHT]));
		color_name2GdkColor ("ActiveText",
												 &(ASGtkStyleNormal->text[GTK_STATE_SELECTED]));
		color_name2GdkColor ("InactiveText1",
												 &(ASGtkStyleNormal->text[GTK_STATE_INSENSITIVE]));

		color_name2GdkColor ("Inactive1",
												 &(ASGtkStyleNormal->base[GTK_STATE_NORMAL]));
		color_name2GdkColor ("Inactive2",
												 &(ASGtkStyleNormal->base[GTK_STATE_ACTIVE]));
		color_name2GdkColor ("HighInactive",
												 &(ASGtkStyleNormal->base[GTK_STATE_PRELIGHT]));
		color_name2GdkColor ("Active",
												 &(ASGtkStyleNormal->base[GTK_STATE_SELECTED]));
		color_name2GdkColor ("Inactive1",
												 &(ASGtkStyleNormal->base[GTK_STATE_INSENSITIVE]));
#if 0
		color_name2GdkColor ("",
												 &(ASGtkStyleNormal->text_aa[GTK_STATE_NORMAL]));
		color_name2GdkColor ("",
												 &(ASGtkStyleNormal->text_aa[GTK_STATE_ACTIVE]));
		color_name2GdkColor ("",
												 &(ASGtkStyleNormal->text_aa[GTK_STATE_PRELIGHT]));
		color_name2GdkColor ("",
												 &(ASGtkStyleNormal->text_aa[GTK_STATE_SELECTED]));
		color_name2GdkColor ("",
												 &(ASGtkStyleNormal->
													 text_aa[GTK_STATE_INSENSITIVE]));
#endif
	}
	return ASGtkStyleNormal;
}

GtkStyle *get_colorschemed_style_button ()
{
	if (ASGtkStyleButton == NULL) {
		ASGtkStyleButton = gtk_style_copy (get_colorschemed_style_normal ());
		color_name2GdkColor ("HighInactiveText",
												 &(ASGtkStyleButton->fg[GTK_STATE_NORMAL]));
		color_name2GdkColor ("HighInactiveText",
												 &(ASGtkStyleButton->fg[GTK_STATE_ACTIVE]));
		color_name2GdkColor ("HighInactiveText",
												 &(ASGtkStyleButton->fg[GTK_STATE_PRELIGHT]));
		color_name2GdkColor ("ActiveText",
												 &(ASGtkStyleButton->fg[GTK_STATE_SELECTED]));
		color_name2GdkColor ("HighInactiveText",
												 &(ASGtkStyleButton->fg[GTK_STATE_INSENSITIVE]));
		color_name2GdkColor ("HighInactiveBack",
												 &(ASGtkStyleButton->bg[GTK_STATE_NORMAL]));
		color_name2GdkColor ("HighInactiveLight",
												 &(ASGtkStyleButton->bg[GTK_STATE_ACTIVE]));
		color_name2GdkColor ("HighInactive",
												 &(ASGtkStyleButton->bg[GTK_STATE_PRELIGHT]));
		color_name2GdkColor ("Active",
												 &(ASGtkStyleButton->bg[GTK_STATE_SELECTED]));
		color_name2GdkColor ("HighInactive",
												 &(ASGtkStyleButton->bg[GTK_STATE_INSENSITIVE]));
		color_name2GdkColor ("Inactive1Light",
												 &(ASGtkStyleButton->light[GTK_STATE_NORMAL]));
		color_name2GdkColor ("Inactive2Light",
												 &(ASGtkStyleButton->light[GTK_STATE_ACTIVE]));
		color_name2GdkColor ("HighInactiveLight",
												 &(ASGtkStyleButton->light[GTK_STATE_PRELIGHT]));
		color_name2GdkColor ("ActiveLight",
												 &(ASGtkStyleButton->light[GTK_STATE_SELECTED]));
		color_name2GdkColor ("Inactive1Light",
												 &(ASGtkStyleButton->
													 light[GTK_STATE_INSENSITIVE]));
		color_name2GdkColor ("Inactive1Dark",
												 &(ASGtkStyleButton->dark[GTK_STATE_NORMAL]));
		color_name2GdkColor ("Inactive2Dark",
												 &(ASGtkStyleButton->dark[GTK_STATE_ACTIVE]));
		color_name2GdkColor ("HighInactiveDark",
												 &(ASGtkStyleButton->dark[GTK_STATE_PRELIGHT]));
		color_name2GdkColor ("ActiveDark",
												 &(ASGtkStyleButton->dark[GTK_STATE_SELECTED]));
		color_name2GdkColor ("Inactive1Dark",
												 &(ASGtkStyleButton->dark[GTK_STATE_INSENSITIVE]));
		color_name2GdkColor ("Inactive1",
												 &(ASGtkStyleButton->mid[GTK_STATE_NORMAL]));
		color_name2GdkColor ("Inactive2",
												 &(ASGtkStyleButton->mid[GTK_STATE_ACTIVE]));
		color_name2GdkColor ("HighInactive",
												 &(ASGtkStyleButton->mid[GTK_STATE_PRELIGHT]));
		color_name2GdkColor ("Active",
												 &(ASGtkStyleButton->mid[GTK_STATE_SELECTED]));
		color_name2GdkColor ("Inactive1",
												 &(ASGtkStyleButton->mid[GTK_STATE_INSENSITIVE]));
		color_name2GdkColor ("HighInactiveText",
												 &(ASGtkStyleButton->text[GTK_STATE_NORMAL]));
		color_name2GdkColor ("HighInactiveText",
												 &(ASGtkStyleButton->text[GTK_STATE_ACTIVE]));
		color_name2GdkColor ("HighInactiveText",
												 &(ASGtkStyleButton->text[GTK_STATE_PRELIGHT]));
		color_name2GdkColor ("ActiveText",
												 &(ASGtkStyleButton->text[GTK_STATE_SELECTED]));
		color_name2GdkColor ("DisabledText",
												 &(ASGtkStyleButton->text[GTK_STATE_INSENSITIVE]));

		color_name2GdkColor ("HighInactiveDark",
												 &(ASGtkStyleButton->base[GTK_STATE_NORMAL]));
		color_name2GdkColor ("HighInactive",
												 &(ASGtkStyleButton->base[GTK_STATE_ACTIVE]));
		color_name2GdkColor ("HighInactiveLight",
												 &(ASGtkStyleButton->base[GTK_STATE_PRELIGHT]));
		color_name2GdkColor ("Active",
												 &(ASGtkStyleButton->base[GTK_STATE_SELECTED]));
		color_name2GdkColor ("HighInactive",
												 &(ASGtkStyleButton->base[GTK_STATE_INSENSITIVE]));
#if 0
		color_name2GdkColor ("",
												 &(ASGtkStyleButton->text_aa[GTK_STATE_NORMAL]));
		color_name2GdkColor ("",
												 &(ASGtkStyleButton->text_aa[GTK_STATE_ACTIVE]));
		color_name2GdkColor ("",
												 &(ASGtkStyleButton->text_aa[GTK_STATE_PRELIGHT]));
		color_name2GdkColor ("",
												 &(ASGtkStyleButton->text_aa[GTK_STATE_SELECTED]));
		color_name2GdkColor ("",
												 &(ASGtkStyleButton->
													 text_aa[GTK_STATE_INSENSITIVE]));
#endif
	}
	return ASGtkStyleButton;
}

void colorize_gtk_window (GtkWidget * window)
{
	GdkColor bg;
	ARGB32 argb;
	CARD32 val;

	parse_argb_color ("Base", &argb);
	val =
			ASCS_BLACK_O_WHITE_CRITERIA16_VAL (ARGB32_RED16 (argb),
																				 ARGB32_GREEN16 (argb),
																				 ARGB32_BLUE16 (argb));
	if (val >= 0x09FFF) {
		LOCAL_DEBUG_OUT ("Base is light, val = 0x%lX", val);
		color_name2GdkColor ("BaseDark", &bg);
	} else {
		LOCAL_DEBUG_OUT ("Base is dark, val = 0x%lX", val);
		color_name2GdkColor ("BaseLight", &bg);
	}
	gtk_widget_modify_bg (window, GTK_STATE_NORMAL, &bg);
}

void
colorize_gtk_tree_view (GtkCellRenderer * cell, GtkTreeViewColumn * column)
{
#if 0
	GdkColor bg;
	GdkColor fg;

	color_name2GdkColor ("Inactive1", &bg);
	color_name2GdkColor ("InactiveText1", &fg);

	g_object_set (G_OBJECT (cell), "background-gdk", &bg, "foreground-gdk",
								&fg, NULL);
	g_object_set (G_OBJECT (column), "background-gdk", &bg, "foreground-gdk",
								&fg, NULL);
#else
	gtk_widget_set_style (GTK_WIDGET (cell),
												get_colorschemed_style_normal ());
#endif

}

void colorize_gtk_widget (GtkWidget * widget, gpointer data)
{

	GtkStyle *style = data ? GTK_STYLE (data) : NULL;
	int i;

	if (style == NULL)
		style = get_colorschemed_style_normal ();

	gtk_widget_set_style (widget, style);
	for (i = 0; i < 5; ++i)
		gtk_widget_modify_fg (widget, i, &(style->fg[i]));
	LOCAL_DEBUG_OUT ("widget %p", widget);
	if (GTK_IS_CONTAINER (widget))
		gtk_container_forall (GTK_CONTAINER (widget), colorize_gtk_widget,
													data);

}


void colorize_gtk_tree_view_window (GtkWidget * widget)
{
	colorize_gtk_widget (widget, get_colorschemed_style_button ());
	gtk_widget_set_style (widget, get_colorschemed_style_normal ());
}

GtkWidget *asgtk_new_button (const char *stock, const char *label)
{
	GtkWidget *btn;

	if (stock != NULL) {
		btn = gtk_button_new_from_stock (stock);
		if (label) {
			gtk_button_set_label (GTK_BUTTON (btn), label);
#if (GTK_MAJOR_VERSION>=2) && (GTK_MINOR_VERSION>=6)
			gtk_button_set_image (GTK_BUTTON (btn),
														gtk_image_new_from_stock (stock,
																											GTK_ICON_SIZE_BUTTON));
#endif
		}
	} else if (label)
		btn = gtk_button_new_with_label (label);
	else
		btn = gtk_button_new ();
	colorize_gtk_widget (GTK_WIDGET (btn), get_colorschemed_style_button ());

	return btn;
}

GtkWidget *asgtk_add_button_to_box (GtkBox * hbox, const char *stock,
																		const char *label, GCallback func,
																		gpointer user_data)
{
	GtkWidget *btn = asgtk_new_button (stock, label);

	gtk_widget_show (btn);
	if (hbox)
		gtk_box_pack_start (hbox, btn, FALSE, FALSE, 0);
	g_signal_connect ((gpointer) btn, "clicked", G_CALLBACK (func),
										user_data);
	return btn;
}

const char *asgtk_combo_box_get_active_text (GtkComboBox * combobox)
{
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_combo_box_get_model (combobox);
	char *text;

	if (!gtk_combo_box_get_active_iter (combobox, &iter))
		return NULL;

	gtk_tree_model_get (model, &iter, 0, &text, -1);
	return text;
}

void asgtk_combo_box_add_to_history (GtkComboBox * cb, const char *str)
{
	GtkTreeModel *model = gtk_combo_box_get_model (cb);
	GtkTreeIter iter;

	if (gtk_tree_model_get_iter_first (model, &iter)) {
		do {
			gchar *val = NULL;

			gtk_tree_model_get (model, &iter, 0, &val, -1);
			if (val && strcmp (str, val) == 0) {
				gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
				break;
			}
		}
		while (gtk_tree_model_iter_next (model, &iter));
	}
	gtk_combo_box_prepend_text (cb, str);
	gtk_combo_box_set_active (cb, 0);
}



Bool
asgtk_yes_no_question1 (GtkWidget * main_window, const char *format,
												const char *detail1)
{
	Bool result = False;
	GtkWidget *dialog = gtk_message_dialog_new (GTK_WINDOW (main_window),
																							GTK_DIALOG_DESTROY_WITH_PARENT,
																							GTK_MESSAGE_QUESTION,
																							GTK_BUTTONS_YES_NO,
																							format, detail1);

	result = (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_YES);
	gtk_widget_destroy (dialog);
	return result;
}

void
asgtk_warning2 (GtkWidget * main_window, const char *format,
								const char *detail1, const char *detail2)
{
	GtkWidget *dialog = gtk_message_dialog_new (GTK_WINDOW (main_window),
																							GTK_DIALOG_DESTROY_WITH_PARENT,
																							GTK_MESSAGE_WARNING,
																							GTK_BUTTONS_OK,
																							format, detail1, detail2);

	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
}

void
asgtk_info2 (GtkWidget * main_window, const char *format,
						 const char *detail1, const char *detail2)
{
	GtkWidget *dialog = gtk_message_dialog_new (GTK_WINDOW (main_window),
																							GTK_DIALOG_DESTROY_WITH_PARENT,
																							GTK_MESSAGE_INFO,
																							GTK_BUTTONS_OK,
																							format, detail1, detail2);

	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
}


void find_combobox_entry (GtkWidget * widget, gpointer data)
{
	GtkWidget **pentry = data;

	if (GTK_IS_ENTRY (widget))
		*pentry = widget;
}
