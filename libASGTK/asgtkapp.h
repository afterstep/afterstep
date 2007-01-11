#ifndef ASGTKAPP_HEADER_FILE_INCLUDED
#define ASGTKAPP_HEADER_FILE_INCLUDED


#ifdef __cplusplus
extern "C" {
#endif

void 
init_asgtkapp( int argc, char *argv[], const char *module_name, void (*custom_usage_func) (void), ASFlagType opt_mask);

GdkColor* color_name2GdkColor( const char *name, GdkColor* color );
char *GdkColor2string( GdkColor *color, int alpha );

GtkStyle* get_colorschemed_style_normal();
GtkStyle* get_colorschemed_style_button();

void colorize_gtk_window( GtkWidget *window );
void colorize_gtk_tree_view( GtkCellRenderer *cell, GtkTreeViewColumn *column );
void colorize_gtk_widget(GtkWidget *widget, gpointer data);
void colorize_gtk_tree_view_window(GtkWidget *widget);
#define colorize_gtk_button(btn) colorize_gtk_widget( GTK_WIDGET(btn), get_colorschemed_style_button())
#define colorize_gtk_edit(btn) 	 colorize_gtk_widget( GTK_WIDGET(btn), get_colorschemed_style_normal())


GtkWidget *asgtk_new_button(const char *stock, const char * label );
GtkWidget *asgtk_add_button_to_box( GtkBox *hbox, const char *stock, const char * label, GCallback func, gpointer user_data );
const char *asgtk_combo_box_get_active_text( GtkComboBox *combobox );
void asgtk_combo_box_add_to_history( GtkComboBox *cb, const char *str );


Bool asgtk_yes_no_question1( GtkWidget *main_window, const char *format, const char *detail1 );
void asgtk_warning2( GtkWidget *main_window, const char *format, const char *detail1, const char *detail2 );
void asgtk_info2( GtkWidget *main_window, const char *format, const char *detail1, const char *detail2 );


void find_combobox_entry(GtkWidget *widget, gpointer data/* == GtkWidget **pentry */);

#ifdef __cplusplus
}
#endif


#endif /* #ifndef ASGTKAPP_HEADER_FILE_INCLUDED */
