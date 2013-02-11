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

#include "../libAfterStep/asapp.h"
#include "../libAfterStep/screen.h"
#include "../libAfterStep/colorscheme.h"
#include "../libAfterImage/afterimage.h"


#include "asgtk.h"
#include "asgtkai.h"
#include "asgtkcolorsel.h"
#include "asgtkgradient.h"

#define COLOR_LIST_WIDTH  150
#define COLOR_LIST_HEIGHT 210
#define COLOR_PREVIEW_WIDTH 	100
#define COLOR_PREVIEW_HEIGHT 	16
#define PREVIEW_WIDTH  320
#define PREVIEW_HEIGHT 240
#define DEFAULT_COLOR	0xFF000000
#define DEFAULT_COLOR_STR  "#FF000000"

/*  local function prototypes  */
static void   asgtk_gradient_class_init (ASGtkGradientClass * klass);
static void   asgtk_gradient_init (ASGtkGradient * ib);
static void   asgtk_gradient_dispose (GObject * object);
static void   asgtk_gradient_finalize (GObject * object);
static void   asgtk_gradient_style_set (GtkWidget * widget, GtkStyle * prev_style);

static void   update_color_preview (ASGtkGradient * ge, const char *color);


typedef struct ASGradientPoint
{
	int           seq_no;
	char         *color_str;
	ARGB32        color_argb;
	double        offset;
} ASGradientPoint;


/*  private variables  */
static GtkDialogClass *parent_class = NULL;

static int    ASGtkGradient_LastSeqNo = 0;

GType
asgtk_gradient_get_type (void)
{
	static GType  ib_type = 0;

	if (!ib_type)
	{
		static const GTypeInfo ib_info = {
			sizeof (ASGtkGradientClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) asgtk_gradient_class_init,
			NULL,							   /* class_finalize */
			NULL,							   /* class_data     */
			sizeof (ASGtkGradient),
			0,								   /* n_preallocs    */
			(GInstanceInitFunc) asgtk_gradient_init,
		};

		ib_type = g_type_register_static (GTK_TYPE_DIALOG, "ASGtkGradient", &ib_info, 0);
	}

	return ib_type;
}

static void
asgtk_gradient_class_init (ASGtkGradientClass * klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	object_class->dispose = asgtk_gradient_dispose;
	object_class->finalize = asgtk_gradient_finalize;

	widget_class->style_set = asgtk_gradient_style_set;

}

ASGradientPoint *
create_asgrad_point ()
{
	ASGradientPoint *point = safecalloc (1, sizeof (ASGradientPoint));

	point->seq_no = ASGtkGradient_LastSeqNo;
	++ASGtkGradient_LastSeqNo;
	return point;
}

void
destroy_asgrad_point (void *data)
{
	ASGradientPoint *point = (ASGradientPoint *) data;

	if (point)
	{
		if (point->color_str)
			free (point->color_str);
		memset (point, 0x00, sizeof (ASGradientPoint));
		free (point);
	}
}

int
compare_asgrad_point (void *data1, void *data2)
{
	ASGradientPoint *point1 = (ASGradientPoint *) data1;
	ASGradientPoint *point2 = (ASGradientPoint *) data2;

	if (point1 == NULL)
		return (point2 == NULL) ? 0 : -1;
	if (point2 == NULL)
		return 1;
	if (point1->offset + 0.0001 > point2->offset && point1->offset - 0.0001 < point2->offset)
		return 0;
	return point1->offset > point2->offset ? 1 : -1;
}

static void
asgtk_gradient_init (ASGtkGradient * ge)
{
	ge->points = create_asbidirlist (destroy_asgrad_point);
	ge->color_preview_width = COLOR_PREVIEW_WIDTH;
	ge->color_preview_height = COLOR_PREVIEW_HEIGHT;
}

static void
asgtk_gradient_dispose (GObject * object)
{
	ASGtkGradient *ge = ASGTK_GRADIENT (object);

	if (ge->points)
		destroy_asbidirlist (&(ge->points));
	if (ge->point_list_model)
	{
		g_object_unref (ge->point_list_model);
		ge->point_list_model = NULL;
	}

	G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
asgtk_gradient_finalize (GObject * object)
{
	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
asgtk_gradient_style_set (GtkWidget * widget, GtkStyle * prev_style)
{
	GTK_WIDGET_CLASS (parent_class)->style_set (widget, prev_style);
}

static void
asgtk_gradient_color_select (GtkTreeSelection * selection, gpointer user_data)
{
	ASGtkGradient *ge = ASGTK_GRADIENT (user_data);
	GtkTreeIter   iter;
	GtkTreeModel *model;

	if (gtk_tree_selection_get_selected (selection, &model, &iter))
	{
		gpointer      p = NULL;

		gtk_tree_model_get (model, &iter, 2, &p, -1);
		if ((ge->current_point = (ASGradientPoint *) p) != NULL)
		{
			gtk_entry_set_text (GTK_ENTRY (ge->color_entry), ge->current_point->color_str);
			update_color_preview (ge, ge->current_point->color_str);
			gtk_spin_button_set_value (GTK_SPIN_BUTTON (ge->offset_entry), ge->current_point->offset);
		}
	}
}

static GtkWidget *
asgtk_gradient_create_color_list (ASGtkGradient * ge)
{
	GtkWidget    *scrolled_win;
	GtkTreeViewColumn *column;

	ge->point_list_model = gtk_list_store_new (3, G_TYPE_STRING, G_TYPE_FLOAT, G_TYPE_POINTER);
	ge->point_list = gtk_tree_view_new_with_model (GTK_TREE_MODEL (ge->point_list_model));

	column = gtk_tree_view_column_new_with_attributes ("Point Offset : ", gtk_cell_renderer_text_new (), "text", 1,
													   NULL);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_append_column (GTK_TREE_VIEW (ge->point_list), column);
	column = gtk_tree_view_column_new_with_attributes ("Point Color : ", gtk_cell_renderer_text_new (), "text", 0,
													   NULL);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_append_column (GTK_TREE_VIEW (ge->point_list), column);

	gtk_widget_set_size_request (ge->point_list, COLOR_LIST_WIDTH, COLOR_LIST_HEIGHT);
	g_signal_connect (gtk_tree_view_get_selection (GTK_TREE_VIEW (ge->point_list)), "changed",
					  G_CALLBACK (asgtk_gradient_color_select), ge);

	scrolled_win = ASGTK_SCROLLED_WINDOW (GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS, GTK_SHADOW_IN);
	ASGTK_CONTAINER_ADD (scrolled_win, ge->point_list);

	colorize_gtk_tree_view_window (GTK_WIDGET (scrolled_win));

	return scrolled_win;
}

static        Bool
add_point_to_color_list (void *data, void *aux_data)
{
	ASGradientPoint *point = (ASGradientPoint *) data;
	ASGtkGradient *ge = (ASGtkGradient *) aux_data;
	GtkTreeIter   iter;

	gtk_list_store_append (ge->point_list_model, &iter);
	gtk_list_store_set (ge->point_list_model, &iter, 0, point->color_str, 1, point->offset, 2, point, -1);

	if (point == ge->current_point)
		gtk_tree_selection_select_iter (gtk_tree_view_get_selection (GTK_TREE_VIEW (ge->point_list)), &iter);

	return True;
}


static void
asgtk_gradient_update_color_list (ASGtkGradient * ge)
{
	gtk_list_store_clear (ge->point_list_model);
	iterate_asbidirlist (ge->points, add_point_to_color_list, ge, NULL, False);
}


static void
update_color_preview (ASGtkGradient * ge, const char *color)
{
	ARGB32        argb = DEFAULT_COLOR;
	GdkPixbuf    *pb;

	if (color && color[0] != '\0')
		parse_argb_color (color, &argb);

	pb = solid_color2GdkPixbuf (argb, ge->color_preview_width, ge->color_preview_height);
	if (pb)
	{
		gtk_image_set_from_pixbuf (GTK_IMAGE (ge->color_preview), pb);
		g_object_unref (pb);
	}
}


void
on_color_clicked (GtkWidget * btn, gpointer data)
{
	ASGtkGradient *ge = ASGTK_GRADIENT (data);
	GtkDialog    *cs = GTK_DIALOG (asgtk_color_selection_new ());
	int           response = 0;
	const char   *orig_color = gtk_entry_get_text (GTK_ENTRY (ge->color_entry));

	gtk_dialog_add_buttons (cs, GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);

	if (orig_color && orig_color[0] != '\0')
		asgtk_color_selection_set_color_by_name (ASGTK_COLOR_SELECTION (cs), orig_color);
	else
		asgtk_color_selection_set_color_by_name (ASGTK_COLOR_SELECTION (cs), DEFAULT_COLOR_STR);
	gtk_window_set_title (GTK_WINDOW (cs), "Pick a color ...   ");
	gtk_window_set_modal (GTK_WINDOW (cs), TRUE);
	response = gtk_dialog_run (cs);

	if (response == GTK_RESPONSE_ACCEPT)
	{
		char         *color = asgtk_color_selection_get_color_str (ASGTK_COLOR_SELECTION (cs));

		if (color)
		{
			gtk_entry_set_text (GTK_ENTRY (ge->color_entry), color);
			update_color_preview (ge, color);
			free (color);
		}
	}
	gtk_widget_destroy (GTK_WIDGET (cs));
}

static        Bool
add_point_to_gradient (void *data, void *aux_data)
{
	ASGradientPoint *point = (ASGradientPoint *) data;
	ASGradient   *gradient = (ASGradient *) aux_data;

	gradient->color[gradient->npoints] = point->color_argb;
	gradient->offset[gradient->npoints] = point->offset;
	++gradient->npoints;

	return True;
}


static void
refresh_gradient_preview (ASGtkGradient * ge)
{
	int           width = get_screen_width (NULL);
	int           height = get_screen_height (NULL);
	struct ASGradient gradient;
	struct ASImageListEntry *entry;
	ARGB32       *color;
	double       *offset;

	if (ge->points->count <= 0)
		return;

	if (GTK_WIDGET_STATE (ge->width_entry) != GTK_STATE_INSENSITIVE)
		if ((width = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (ge->width_entry))) == 0)
			width = get_screen_width (NULL);

	if (GTK_WIDGET_STATE (ge->height_entry) != GTK_STATE_INSENSITIVE)
		if ((height = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (ge->height_entry))) == 0)
			height = get_screen_height (NULL);

	entry = create_asimage_list_entry ();
	/* rendering gradient preview : */

	gradient.npoints = 0;
	gradient.type = ge->type;

	gradient.offset = offset = safemalloc ((ge->points->count + 2) * sizeof (double));
	gradient.color = color = safemalloc ((ge->points->count + 2) * sizeof (ARGB32));

	++gradient.offset;
	++gradient.color;
	iterate_asbidirlist (ge->points, add_point_to_gradient, &gradient, NULL, False);

	if (gradient.offset[0] > 0.)
	{
		--gradient.offset;
		--gradient.color;
		gradient.offset[0] = 0.;
		gradient.color[0] = DEFAULT_COLOR;	   /* black */
		++gradient.npoints;
	}
	if (gradient.offset[gradient.npoints - 1] < 1.)
	{
		gradient.offset[gradient.npoints] = 1.;
		gradient.color[gradient.npoints] = DEFAULT_COLOR;	/* black */
		++gradient.npoints;
	}


	entry->preview =
		make_gradient (get_screen_visual (NULL), &gradient, width, height, SCL_DO_ALL, ASA_ASImage, 0,
					   ASIMAGE_QUALITY_DEFAULT);

	free (offset);
	free (color);

	/* applying gradient preview : */
	if (entry->preview)
		asgtk_image_view_set_entry (ge->image_view, entry);
	else
		asgtk_image_view_set_entry (ge->image_view, NULL);

	unref_asimage_list_entry (entry);

}

static void
on_direction_clicked (GtkWidget * widget, gpointer data)
{
	ASGtkGradient *ge = ASGTK_GRADIENT (data);
	int           new_type = 0;

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (ge->tl2br_radio)))
		new_type = GRADIENT_TopLeft2BottomRight;
	else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (ge->t2b_radio)))
		new_type = GRADIENT_Top2Bottom;
	else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (ge->bl2tr_radio)))
		new_type = GRADIENT_BottomLeft2TopRight;

	if (ge->type != new_type)
	{
		ge->type = new_type;
		refresh_gradient_preview (ge);
	}
}

static void
on_size_clicked (GtkWidget * widget, gpointer data)
{
	ASGtkGradient *ge = ASGTK_GRADIENT (data);
	Bool          active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));

	if (widget == ge->screen_width_check)
		gtk_widget_set_sensitive (ge->width_entry, !active);
	else
		gtk_widget_set_sensitive (ge->height_entry, !active);
	refresh_gradient_preview (ge);
}

static void
on_delete_point_clicked (GtkWidget * widget, gpointer data)
{
	ASGtkGradient *ge = ASGTK_GRADIENT (data);

	if (ge->current_point && ge->points->count > 0)
	{
		discard_bidirelem (ge->points, ge->current_point);
		ge->current_point = NULL;
		asgtk_gradient_update_color_list (ge);
		refresh_gradient_preview (ge);
	}
}

static void
query_point_values (ASGtkGradient * ge, ASGradientPoint * point)
{
	const char   *color;

	point->offset = gtk_spin_button_get_value (GTK_SPIN_BUTTON (ge->offset_entry));
	color = gtk_entry_get_text (GTK_ENTRY (ge->color_entry));
	if (point->color_str)
		free (point->color_str);
	if (color == NULL || color[0] == '\0')
	{
		asgtk_warning2 (GTK_WIDGET (ge), "Color value is not specifyed. Default color of black will be used.", NULL,
						NULL);
		point->color_str = mystrdup (DEFAULT_COLOR_STR);
		point->color_argb = DEFAULT_COLOR;
	} else
	{
		if (parse_argb_color (color, &(point->color_argb)) == color)
			asgtk_warning2 (GTK_WIDGET (ge), "Color \"%s\" cannot be translated. It is probably a mistyped color name.",
							color, NULL);
		point->color_str = mystrdup (color);
	}
}

static void
on_add_point_clicked (GtkWidget * widget, gpointer data)
{
	ASGtkGradient *ge = ASGTK_GRADIENT (data);
	ASGradientPoint *point = create_asgrad_point ();

	query_point_values (ge, point);
	append_bidirelem (ge->points, point);
	bubblesort_asbidirlist (ge->points, compare_asgrad_point);
	ge->current_point = point;
	asgtk_gradient_update_color_list (ge);
	refresh_gradient_preview (ge);

	gtk_widget_set_sensitive (ge->delete_btn, TRUE);
	gtk_widget_set_sensitive (ge->apply_btn, TRUE);
}

static void
on_apply_point_clicked (GtkWidget * widget, gpointer data)
{
	ASGtkGradient *ge = ASGTK_GRADIENT (data);

	if (ge->current_point)
	{
		query_point_values (ge, ge->current_point);
		bubblesort_asbidirlist (ge->points, compare_asgrad_point);
		asgtk_gradient_update_color_list (ge);
		refresh_gradient_preview (ge);
	}
}

static void
on_refresh_clicked (GtkWidget * widget, gpointer data)
{
	ASGtkGradient *ge = ASGTK_GRADIENT (data);

	refresh_gradient_preview (ge);
}

static void
color_preview_size_alloc (GtkWidget * widget, GtkAllocation * allocation, gpointer user_data)
{
	ASGtkGradient *ge = ASGTK_GRADIENT (user_data);
	int           w = allocation->width - 4;
	int           h = allocation->height - 4;

#if 1										   /* if size changed - refresh */
	if (ge->color_preview_width != w || ge->color_preview_height != h)
	{
		const char   *color;

		ge->color_preview_width = w;
		ge->color_preview_height = h;
		color = gtk_entry_get_text (GTK_ENTRY (ge->color_entry));
		update_color_preview (ge, color);
	}
#endif
}


/*  public functions  */
GtkWidget    *
asgtk_gradient_new ()
{
	ASGtkGradient *ge;
	GtkWidget    *main_vbox, *main_hbox;
	GtkWidget    *scrolled_window;
	GtkWidget    *list_vbox;
	GtkWidget    *frame, *hbox, *vbox, *btn, *table;
	GtkWidget    *label;

	ge = g_object_new (ASGTK_TYPE_GRADIENT, NULL);
	colorize_gtk_window (GTK_WIDGET (ge));
	gtk_container_set_border_width (GTK_CONTAINER (ge), 5);

	main_vbox = GTK_DIALOG (ge)->vbox;

	main_hbox = gtk_hbox_new (FALSE, 5);
	gtk_widget_show (main_hbox);
	gtk_box_pack_start (GTK_BOX (main_vbox), main_hbox, TRUE, TRUE, 0);

	list_vbox = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (main_hbox), list_vbox, FALSE, FALSE, 0);

	frame = gtk_frame_new ("Gradient direction : ");
	gtk_box_pack_start (GTK_BOX (list_vbox), frame, FALSE, FALSE, 5);

	table = gtk_table_new (2, 2, FALSE);
	gtk_container_add (GTK_CONTAINER (frame), table);
	gtk_container_set_border_width (GTK_CONTAINER (table), 3);

	ge->l2r_radio = gtk_radio_button_new_with_label (NULL, "Left to Right");
	gtk_table_attach_defaults (GTK_TABLE (table), ge->l2r_radio, 0, 1, 0, 1);
	ge->t2b_radio = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (ge->l2r_radio), "Top to Bottom");
	gtk_table_attach_defaults (GTK_TABLE (table), ge->t2b_radio, 0, 1, 1, 2);
	ge->tl2br_radio =
		gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (ge->t2b_radio), "Top-Left to Bottom-Right");
	gtk_table_attach_defaults (GTK_TABLE (table), ge->tl2br_radio, 1, 2, 0, 1);
	ge->bl2tr_radio =
		gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (ge->tl2br_radio), "Bottom-Left to Top-Right");
	gtk_table_attach_defaults (GTK_TABLE (table), ge->bl2tr_radio, 1, 2, 1, 2);
	gtk_widget_show_all (table);
	gtk_widget_show (table);
	colorize_gtk_widget (frame, get_colorschemed_style_normal ());

	g_signal_connect ((gpointer) ge->l2r_radio, "clicked", G_CALLBACK (on_direction_clicked), ge);
	g_signal_connect ((gpointer) ge->tl2br_radio, "clicked", G_CALLBACK (on_direction_clicked), ge);
	g_signal_connect ((gpointer) ge->t2b_radio, "clicked", G_CALLBACK (on_direction_clicked), ge);
	g_signal_connect ((gpointer) ge->bl2tr_radio, "clicked", G_CALLBACK (on_direction_clicked), ge);


	ge->screen_width_check = gtk_check_button_new_with_label ("Use screen width");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ge->screen_width_check), TRUE);
	ge->width_entry = gtk_spin_button_new_with_range (1, 10000, 1);
	gtk_widget_set_sensitive (ge->width_entry, FALSE);
	ge->screen_height_check = gtk_check_button_new_with_label ("Use screen height");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ge->screen_height_check), TRUE);
	ge->height_entry = gtk_spin_button_new_with_range (1, 10000, 1);
	gtk_widget_set_sensitive (ge->height_entry, FALSE);

	g_signal_connect ((gpointer) ge->screen_width_check, "clicked", G_CALLBACK (on_size_clicked), ge);
	g_signal_connect ((gpointer) ge->screen_height_check, "clicked", G_CALLBACK (on_size_clicked), ge);


	ge->size_frame = gtk_frame_new ("Rendered gradient size : ");
	gtk_box_pack_start (GTK_BOX (list_vbox), ge->size_frame, FALSE, FALSE, 5);
	colorize_gtk_tree_view_window (GTK_WIDGET (ge->size_frame));

	table = gtk_table_new (2, 4, FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (table), 3);
	gtk_container_add (GTK_CONTAINER (ge->size_frame), table);

	gtk_table_attach_defaults (GTK_TABLE (table), gtk_label_new ("Width : "), 0, 1, 0, 1);
	gtk_table_attach_defaults (GTK_TABLE (table), ge->width_entry, 1, 2, 0, 1);
	gtk_table_attach_defaults (GTK_TABLE (table), gtk_label_new ("Height : "), 2, 3, 0, 1);
	gtk_table_attach_defaults (GTK_TABLE (table), ge->height_entry, 3, 4, 0, 1);
	gtk_table_attach (GTK_TABLE (table), ge->screen_width_check, 0, 2, 1, 2, GTK_FILL, GTK_FILL, 10, 0);
	gtk_table_attach (GTK_TABLE (table), ge->screen_height_check, 2, 4, 1, 2, GTK_FILL, GTK_FILL, 10, 0);

	gtk_widget_show_all (table);
	gtk_widget_show (table);
	colorize_gtk_widget (ge->size_frame, get_colorschemed_style_normal ());

	scrolled_window = asgtk_gradient_create_color_list (ge);
	gtk_box_pack_start (GTK_BOX (list_vbox), scrolled_window, FALSE, FALSE, 0);

	ge->color_entry = gtk_entry_new_with_max_length (24);
	gtk_entry_set_width_chars (GTK_ENTRY (ge->color_entry), 16);
#if 0
	ge->color_preview = gtk_color_button_new ();
	g_signal_connect ((gpointer) ge->color_preview, "clicked", G_CALLBACK (on_color_preview_clicked), ge);
#else
	ge->color_preview = gtk_image_new ();
	update_color_preview (ge, DEFAULT_COLOR_STR);
#endif

	ge->offset_entry = gtk_spin_button_new_with_range (0., 1., 0.05);


	frame = gtk_frame_new ("Change point attributes : ");
	gtk_box_pack_end (GTK_BOX (list_vbox), frame, FALSE, FALSE, 5);
	colorize_gtk_tree_view_window (GTK_WIDGET (frame));

	vbox = gtk_vbox_new (FALSE, 5);
	gtk_container_add (GTK_CONTAINER (frame), vbox);

	table = gtk_table_new (4, 2, FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (table), 3);


//  hbox = gtk_hbox_new( FALSE, 5 );               
//  gtk_container_set_border_width( GTK_CONTAINER (hbox), 3 );
	gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
	label = gtk_label_new ("Color : ");
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 0, 1);
	label = gtk_label_new ("Offset : ");
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 2, 3, 0, 1);
	gtk_table_attach (GTK_TABLE (table), ge->color_entry, 1, 2, 0, 1, GTK_SHRINK, GTK_SHRINK, 2, 2);
	gtk_table_attach (GTK_TABLE (table), ge->offset_entry, 3, 4, 0, 1, GTK_SHRINK, GTK_SHRINK, 2, 2);

	frame = gtk_frame_new (NULL);
	colorize_gtk_tree_view_window (GTK_WIDGET (frame));
	gtk_widget_set_size_request (frame, COLOR_PREVIEW_WIDTH, COLOR_PREVIEW_HEIGHT);
	gtk_container_set_border_width (GTK_CONTAINER (table), 0);
	gtk_container_add (GTK_CONTAINER (frame), ge->color_preview);
	gtk_widget_show (ge->color_preview);

	btn = gtk_button_new ();
	gtk_container_set_border_width (GTK_CONTAINER (btn), 0);
	//btn = gtk_button_new_with_label(" Color selector ");
	colorize_gtk_widget (GTK_WIDGET (btn), get_colorschemed_style_button ());
	gtk_container_add (GTK_CONTAINER (btn), frame);
	gtk_widget_show (frame);
	g_signal_connect ((gpointer) frame, "size-allocate", G_CALLBACK (color_preview_size_alloc), ge);

	gtk_table_attach (GTK_TABLE (table), btn, 0, 2, 1, 2, GTK_FILL, GTK_SHRINK, 2, 2);
	g_signal_connect ((gpointer) btn, "clicked", G_CALLBACK (on_color_clicked), ge);

	gtk_widget_show_all (table);
	gtk_widget_show (table);

	hbox = gtk_hbox_new (FALSE, 5);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 3);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

	ge->delete_btn = btn = gtk_button_new_from_stock (GTK_STOCK_DELETE);
	gtk_widget_set_sensitive (ge->delete_btn, FALSE);
	colorize_gtk_widget (GTK_WIDGET (btn), get_colorschemed_style_button ());
	gtk_box_pack_end (GTK_BOX (hbox), btn, FALSE, FALSE, 0);
	g_signal_connect ((gpointer) btn, "clicked", G_CALLBACK (on_delete_point_clicked), ge);

	btn = gtk_button_new_from_stock (GTK_STOCK_ADD);
	colorize_gtk_widget (GTK_WIDGET (btn), get_colorschemed_style_button ());
	gtk_box_pack_end (GTK_BOX (hbox), btn, FALSE, FALSE, 0);
	g_signal_connect ((gpointer) btn, "clicked", G_CALLBACK (on_add_point_clicked), ge);

	ge->apply_btn = btn = gtk_button_new_from_stock (GTK_STOCK_APPLY);
	gtk_widget_set_sensitive (ge->apply_btn, FALSE);
	colorize_gtk_widget (GTK_WIDGET (btn), get_colorschemed_style_button ());
	gtk_box_pack_end (GTK_BOX (hbox), btn, FALSE, FALSE, 0);
	g_signal_connect ((gpointer) btn, "clicked", G_CALLBACK (on_apply_point_clicked), ge);

	gtk_widget_show_all (hbox);
	gtk_widget_show (hbox);

	/* The preview : */
	ge->image_view = ASGTK_IMAGE_VIEW (asgtk_image_view_new ());
	gtk_widget_set_size_request (GTK_WIDGET (ge->image_view), PREVIEW_WIDTH, PREVIEW_HEIGHT);
	gtk_box_pack_end (GTK_BOX (main_hbox), GTK_WIDGET (ge->image_view), TRUE, TRUE, 0);
	asgtk_image_view_set_aspect (ge->image_view, get_screen_width (NULL), get_screen_height (NULL));
	asgtk_image_view_set_resize (ge->image_view,
								 ASGTK_IMAGE_VIEW_SCALE_TO_VIEW |
								 ASGTK_IMAGE_VIEW_TILE_TO_ASPECT, ASGTK_IMAGE_VIEW_RESIZE_ALL);



	gtk_widget_show_all (list_vbox);
	gtk_widget_show_all (main_hbox);
	gtk_widget_hide (ge->image_view->details_label);

	btn = gtk_check_button_new_with_label ("Use screen aspect ratio");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (btn), TRUE);
	gtk_widget_show (btn);
	colorize_gtk_widget (btn, get_colorschemed_style_normal ());
	g_signal_connect (G_OBJECT (btn), "toggled",
					  G_CALLBACK (asgtk_image_view_screen_aspect_toggle), (gpointer) ge->image_view);
	gtk_button_set_alignment (GTK_BUTTON (btn), 1.0, 0.5);
	asgtk_image_view_add_detail (ge->image_view, btn, 0);

	btn = gtk_check_button_new_with_label ("Scale to fit this view");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (btn), TRUE);
	gtk_widget_show (btn);
	colorize_gtk_widget (btn, get_colorschemed_style_normal ());
	g_signal_connect (G_OBJECT (btn), "toggled",
					  G_CALLBACK (asgtk_image_view_scale_to_view_toggle), (gpointer) ge->image_view);
	gtk_button_set_alignment (GTK_BUTTON (btn), 1.0, 0.5);
	asgtk_image_view_add_detail (ge->image_view, btn, 0);

	btn = asgtk_add_button_to_box (NULL, GTK_STOCK_REFRESH, NULL, G_CALLBACK (on_refresh_clicked), ge);
	asgtk_image_view_add_tool (ge->image_view, btn, 3);


	LOCAL_DEBUG_OUT ("created image ASGtkGradient object %p", ge);
	return GTK_WIDGET (ge);
}


static        Bool
collect_gradient_offsets_str (void *data, void *aux_data)
{
	ASGradientPoint *point = (ASGradientPoint *) data;
	char        **pstr = (char **)aux_data;

	if (*pstr)
	{
		*pstr = realloc (*pstr, strlen (*pstr) + 6);
		sprintf (*pstr, "%s %2.2f", *pstr, point->offset);
	} else
	{
		*pstr = safemalloc (6);
		sprintf (*pstr, "%2.2f", point->offset);
	}
	return True;
}

static        Bool
collect_gradient_colors_str (void *data, void *aux_data)
{
	ASGradientPoint *point = (ASGradientPoint *) data;
	char        **pstr = (char **)aux_data;
	int           clen = strlen (point->color_str);

	if (*pstr)
	{
		*pstr = realloc (*pstr, strlen (*pstr) + 1 + clen + 1);
		sprintf (*pstr, "%s %s", *pstr, point->color_str);
	} else
	{
		*pstr = mystrdup (point->color_str);
	}
	return True;
}


char         *
asgtk_gradient_get_xml (ASGtkGradient * ge, char **mini)
{
	char         *xml = NULL;

	if (ge->points->count > 0)
	{
		char         *offsets = NULL;
		char         *colors = NULL;
		ASGradientPoint *first, *last;
		Bool          add_first = False, add_last = False;
		int           width = 0, height = 0;
		int           pos = 0, colors_len, offsets_len;

		first = LIST_START (ge->points)->data;
		last = LIST_END (ge->points)->data;

		add_first = (first->offset > 0.);
		add_last = (last->offset < 1.);

		iterate_asbidirlist (ge->points, collect_gradient_offsets_str, &offsets, NULL, False);
		iterate_asbidirlist (ge->points, collect_gradient_colors_str, &colors, NULL, False);

		colors_len = strlen (colors);
		offsets_len = strlen (offsets);

		if (GTK_WIDGET_STATE (ge->width_entry) != GTK_STATE_INSENSITIVE)
			width = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (ge->width_entry));

		if (GTK_WIDGET_STATE (ge->height_entry) != GTK_STATE_INSENSITIVE)
			height = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (ge->height_entry));

		xml = safemalloc (64 + 10 + 10 + strlen (colors) + 4 + 4 + strlen (offsets) + 32 + 32 + 2 + 1);
		strcpy (xml, "<gradient colors=\"");
		pos = 18;
		if (add_first)
		{
			strcpy (&xml[pos], "#FF000000 ");
			pos += 10;
		}
		strcpy (&xml[pos], colors);
		pos += colors_len;
		if (add_last)
		{
			strcpy (&xml[pos], " #FF000000");
			pos += 10;
		}
		strcpy (&xml[pos], "\" offsets=\"");
		pos += 11;

		if (add_first)
		{
			strcpy (&xml[pos], "0.0 ");
			pos += 4;
		}
		strcpy (&xml[pos], offsets);
		pos += offsets_len;
		if (add_last)
		{
			strcpy (&xml[pos], " 1.0");
			pos += 4;
		}

		if (mini)
		{
			*mini = safemalloc (pos + 64);
			sprintf (*mini, "%s\" width=\"$minipixmap.width\" height=\"$minipixmap.height\"/>", xml);
		}

		strcpy (&xml[pos], "\" width=\"");
		pos += 9;

		if (width > 0)
			sprintf (&xml[pos], "%d", width);
		else
			strcpy (&xml[pos], "$xroot.width");
		while (xml[pos] != '\0')
			++pos;

		strcpy (&xml[pos], "\" height=\"");
		pos += 10;

		/* no more then 32 bytes : */
		if (height > 0)
			sprintf (&xml[pos], "%d", height);
		else
			strcpy (&xml[pos], "$xroot.height");
		while (xml[pos] != '\0')
			++pos;

		/* no more then 2 bytes : */
		sprintf (&xml[pos], "\"/>");
	}
	return xml;
}
