#ifndef WMPROPS_H_HEADER_INCLUDED
#define WMPROPS_H_HEADER_INCLUDED

struct ScreenInfo;

#define INVALID_DESKTOP_PROP    0xFFFFFFFF

/************************************************************************/
/*		globals (atom IDs)					*/
/************************************************************************/
/* standard : .... well, more or less */
extern Atom  _XA_MIT_PRIORITY_COLORS     ;
extern Atom  _XA_WM_CHANGE_STATE         ;

/* from Enlightenment for compatibility with Eterm */
extern Atom  _XROOTPMAP_ID               ;

/* Old Gnome compatibility specs : */
extern Atom  _XA_WIN_SUPPORTING_WM_CHECK ;
extern Atom  _XA_WIN_PROTOCOLS           ;
extern Atom  _XA_WIN_DESKTOP_BUTTON_PROXY;
extern Atom _XA_WIN_WORKSPACE           ;      /* declared in clientprops.c */
extern Atom  _XA_WIN_WORKSPACE_COUNT     ;
extern Atom  _XA_WIN_WORKSPACE_NAMES     ;
extern Atom  _XA_WIN_CLIENT_LIST         ;

/* Extended Window Manager Compatibility specs : */
extern Atom  _XA_NET_SUPPORTED           ;
extern Atom  _XA_NET_CLIENT_LIST         ;
extern Atom  _XA_NET_CLIENT_LIST_STACKING;
extern Atom  _XA_NET_NUMBER_OF_DESKTOPS  ;
extern Atom  _XA_NET_DESKTOP_GEOMETRY    ;
extern Atom  _XA_NET_DESKTOP_VIEWPORT    ;
extern Atom  _XA_NET_CURRENT_DESKTOP     ;
extern Atom  _XA_NET_DESKTOP_NAMES       ;
extern Atom  _XA_NET_ACTIVE_WINDOW       ;
extern Atom  _XA_NET_WORKAREA            ;
extern Atom  _XA_NET_SUPPORTING_WM_CHECK ;
extern Atom  _XA_NET_VIRTUAL_ROOTS       ;

/* AfterStep specific ; */
extern Atom  _AS_STYLE                   ;
extern Atom  _AS_BACKGROUND              ;
extern Atom  _AS_VISUAL_ID               ;
extern Atom  _AS_MODULE_SOCKET           ;
extern Atom  _AS_VIRTUAL_ROOT            ;
extern Atom  _AS_DESK_NUMBERS            ;
extern Atom  _AS_CURRENT_DESK            ;
extern Atom  _AS_CURRENT_VIEWPORT        ;

extern AtomXref WMPropAtoms[];    /*all top level atoms for purpose of easy interning */


/***********************************************************************/
/* AfterStep structure to hold all the Window Management properties:   */
/***********************************************************************/
typedef enum
{
	WMC_NotHandled		= 0,
    WMC_PreservedColors = (0x01<<0),
    WMC_RootPixmap      = (0x01<<1),
    WMC_Desktops        = (0x01<<2),
    WMC_DesktopCurrent  = (0x01<<3),
    WMC_DesktopViewport = (0x01<<4),
    WMC_DesktopNames    = (0x01<<5),
    WMC_ClientList      = (0x01<<6),
    WMC_ActiveWindow    = (0x01<<7),
    WMC_WorkArea        = (0x01<<8),
    WMC_ASStyles        = (0x01<<9),
    WMC_ASBackgrounds   = (0x01<<10),
    WMC_ASVisual        = (0x01<<11),
    WMC_ASModule        = (0x01<<12),
    WMC_ASVirtualRoot   = (0x01<<13),
    WMC_ASDesks         = (0x01<<14),
    WMC_ASViewport      = (0x01<<15)
}WMPropClass;


typedef struct ASWMProps
{
    Bool        manager;
	struct ScreenInfo *scr;       /* who we belong to - each screen has its own stuff */

    Atom        _XA_WM_S ;
    Window      selection_window ;
    Time        selection_time ;

    /* list of supported properties atoms: */
    /* (just for consistency - not really used for now since we are always under AfterStep ) */
    unsigned long  supported_num ;
    Atom          *supported ;
    /* Supporting WM Check window : */
    Window         wm_check_window ;
    /* Event Proxy window : */
    Window         wm_event_proxy ;

    /* Properties : */
    /* PreservedColors: */
    unsigned long  preserved_colors_num;
    unsigned long *preserved_colors;
    /* RootPixmap: */
    Pixmap         root_pixmap;
    /* Desktops : */
    unsigned long  desktop_num;
    unsigned long  desktop_width, desktop_height;
    unsigned long  desktop_viewports_num ;
    unsigned long  *desktop_viewport;   /* one per desk - stupid! stupid!  */
    unsigned long  desktop_current;
    Window        *virtual_roots;
    /* DesktopNames : */
    char         **desktop_names;
    unsigned long  desktop_names_num;
    /* ClientList : */
    unsigned long  clients_num ;
    Window        *client_list ;
    Window        *stacking_order ;
    /* ActiveWindow : */
    Window         active_window ;
    /* WorkArea : */
    unsigned long  work_x, work_y, work_width, work_height ;

    /* AfterStep specific : */
    /* ASStyles : */
    size_t         as_styles_size ;            /* in bytes */
    unsigned long  as_styles_version ;
    unsigned long *as_styles_data ;

    /* ASBackgrounds : */
    size_t        as_backgrounds_size ;        /* in bytes */
    unsigned long as_backgrounds_version ;
    unsigned long *as_backgrounds_data ;

	/* ASVisual : */
    size_t        as_visual_size ;        /* in bytes */
    unsigned long as_visual_version ;
    unsigned long *as_visual_data ;

    /* ASModule : */
    char          *as_socket_filename;
    /* ASVirtualRoot : */
    Window         as_virtual_root ;
    /* ASDesks : */
    unsigned long  as_desk_num ;
    long           as_current_desk ;
    long          *as_desk_numbers ;

    unsigned long  as_current_vx, as_current_vy ;

    ASFlagType     my_props ;
    ASFlagType     set_props ;
}ASWMProps;

/*************************************************************************/
/*                           Interface                                   */
/*************************************************************************/
/* low level */
Bool read_xrootpmap_id (ASWMProps * wmprops, Bool deleted);

/* High level */
void intern_wmprop_atoms ();
ASWMProps *setup_wmprops (struct ScreenInfo* scr, Bool manager, ASFlagType what, ASWMProps *reusable_memory);
void destroy_wmprops( ASWMProps *wmprops, Bool reusable );
void wmprops_cleanup();

/* printing functions :
 * if func and stream are not specified - fprintf(stderr) is used ! */
void print_wm_props         ( stream_func func, void* stream, XWMHints *hints );


/* Setting properties here : */
void set_as_module_socket (ASWMProps *wmprops, char *new_socket );
void set_as_style (ASWMProps *wmprops, unsigned long size, unsigned long version, unsigned long *data );
void set_as_backgrounds (ASWMProps *wmprops, unsigned long size, unsigned long version, unsigned long *data );
void set_xrootpmap_id (ASWMProps *wmprops, Pixmap new_pmap );


unsigned long as_desk2ext_desk( ASWMProps *wmprops, long as_desk );
void set_desktop_num_prop( ASWMProps *wmprops, long new_desk, Window vroot, Bool add );
Bool set_current_desk_prop (ASWMProps *wmprops, long new_desk );
Bool set_current_viewport_prop (ASWMProps * wmprops, long vx, long vy, Bool normal);

void flush_wmprop_data(ASWMProps *wmprops, ASFlagType what );

/* automated event handling : */
WMPropClass handle_wmprop_event( ASWMProps *wmprops, XEvent *event );

/*************************************************************************/
/********************************THE END**********************************/
/*************************************************************************/

#endif  /* CLIENTPROPS_H_HEADER_INCLUDED */
