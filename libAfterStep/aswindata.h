#ifndef WINLIST_WINDATA_H_HEADER_INCLUDED
#define WINLIST_WINDATA_H_HEADER_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

struct ASCanvas;
struct ASTBarData;
struct ASDesktopEntry;

#include "module.h"

typedef enum
{
	ASN_Name = 0,
	ASN_IconName,
	ASN_ResClass,
	ASN_ResName,
	ASN_NameMatched,
	ASN_NameTypes
}ASNameTypes ;

typedef struct ASWindowData
{
#define MAGIC_ASWindowData	0xA3817D08
	unsigned long magic ;
	Window 	client;
	Window 	frame ;

	send_data_type 	ref_ptr ; /* address of the related ASWindow structure in
								 afterstep's own memory space - we use it as a
								 reference, to know what object change is related to */
	ASRectangle 	frame_rect ;
	ASRectangle     icon_rect ;

	long            desk ;
	send_data_type  state_flags, flags, client_icon_flags ;

	XSizeHints      hints ;       /* not sure why we need it here */

	Window 			icon_title ;
	Window          icon ;

	char            *window_name ;
	char            *window_name_matched ;
	char 			*icon_name ;
	char 			*res_class ;
	char 			*res_name ;
	INT32		 	 window_name_encoding, window_name_matched_encoding ;
	INT32		 	 icon_name_encoding, res_class_encoding, res_name_encoding ;

	int 			 pid ;

	Bool             focused;

	char 			*cmd_line ;

	/**************************/
	/* the following gets determined based on matching names 
	 * agains the list of desktop entries : 
	 */
	struct ASDesktopEntry	*desktop_entry ;

	/**************************/
	/* some additional data that may or maynot be used by modules : */
	struct ASCanvas        *canvas;
	struct ASTBarData      *bar ;
	ASFlagType				module_flags ;
	void *data ;

}ASWindowData;
/**********************************************************************/
/* w, frame, t and the rest of the full window config */
#define WINDOW_CONFIG_MASK (M_ADD_WINDOW|M_CONFIGURE_WINDOW|M_STATUS_CHANGE|M_MAP)
/* w, frame, t, and then text :*/
#define WINDOW_NAME_MASK   (M_WINDOW_NAME|M_WINDOW_NAME_MATCHED|M_ICON_NAME|M_RES_CLASS|M_RES_NAME)
/* w, frame and t */
#define WINDOW_STATE_MASK  (M_FOCUS_CHANGE|M_DESTROY_WINDOW)

#define WINDOW_PACKET_MASK (WINDOW_CONFIG_MASK| \
							WINDOW_NAME_MASK|WINDOW_STATE_MASK)

typedef enum {
	WP_Error = -1,
	WP_Handled,
	WP_DataCreated,
	WP_DataChanged,
	WP_DataDeleted
}WindowPacketResult ;

void destroy_window_data(ASWindowData *wd);
ASWindowData *fetch_window_by_id( Window w );
ASWindowData *add_window_data( ASWindowData *wd );
WindowPacketResult handle_window_packet(send_data_type type, send_data_type *data, ASWindowData **pdata);

void iterate_window_data( iter_list_data_handler iter_func, void *aux_data);

void window_data_cleanup();

char *get_window_name( ASWindowData *wd, ASNameTypes type, INT32 *encoding );


#ifdef __cplusplus
}
#endif


#endif /* #ifndef WINLIST_WINDATA_H_HEADER_INCLUDED */
