#include "asdatabase.h"
#ifndef AS_STYLE_H
#define AS_STYLE_H

/* values for name_list flags */
#define STICKY_FLAG		(1 << 0)
#define NOBORDER_FLAG		(1 << 1)
#define NOTITLE_FLAG		(1 << 2)
#define ICON_FLAG		(1 << 3)
#define CIRCULATESKIP_FLAG	(1 << 4)
#define LISTSKIP_FLAG		(1 << 5)
#define STAYSONDESK_FLAG	(1 << 6)
#define SUPPRESSICON_FLAG	(1 << 7)
#define BW_FLAG			(1 << 8)
#define NOBW_FLAG		(1 << 9)
#define START_ICONIC_FLAG	(1 << 10)
#define NOICON_TITLE_FLAG	(1 << 11)
#define NOFOCUS_FLAG		(1 << 12)
#define VIEWPORTX_FLAG		(1 << 13)
#define VIEWPORTY_FLAG		(1 << 14)
#define AVOID_COVER_FLAG	(1 << 15)
#define VERTICAL_TITLE_FLAG	(1 << 16)	/* if this window has a vertical titlebar */
#define STYLE_FOCUS_FLAG	(1 << 17)	/* focus style is defined */
#define STYLE_UNFOCUS_FLAG	(1 << 18)	/* unfocus style is defined */
#define STYLE_STICKY_FLAG	(1 << 19)	/* sticky style is defined */
#define LAYER_FLAG		(1 << 20)	/* layer is defined */
#define NOFRAME_FLAG		(1 << 21)
#define NOHANDLES_FLAG		(1 << 22)
#define PREPOS_FLAG		(1 << 23)

typedef struct name_list
  {
    struct name_list *next;	/* pointer to the next name */
    char *name;			/* the name of the window */
    char *icon_file;		/* icon name */
    int Desk;			/* Desktop number */
    int layer;			/* layer number */
    int ViewportX, ViewportY;
    int PreposX, PreposY;
    unsigned int PreposWidth, PreposHeight;
	int PreposFlags ;
    unsigned long on_flags;
    unsigned long off_flags;
    int border_width;
    int resize_width;

    char *style_focus;
    char *style_unfocus;
    char *style_sticky;

    unsigned long on_buttons;
    unsigned long off_buttons;
  }
name_list;

name_list *style_new (void);
void style_delete (name_list * style);
void style_init (name_list * nl);
void style_parse (char *text, FILE * fd, char **list, int *junk);
void style_fill_by_name (name_list * nl, char **names );
Bool match_window_names_old( char *regexp, char **names );

#endif /* AS_STYLE_H */
