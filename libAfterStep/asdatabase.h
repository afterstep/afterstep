#ifndef AS_STYLE_H
#define AS_STYLE_H

#include "afterstep.h"

#ifdef __cplusplus
extern "C" {
#endif

struct wild_reg_exp;

/*  we clean up flags usage in style and use :
 *  set_flags/flags pair instead of  on_flags/off_flags
 */
/* this are flags defining that particular value is set and enable/disabled */
#define STYLE_ICON              (1 << 0)   /* also goes into style->icon_file */
#define STYLE_STARTUP_DESK      (1 << 1)   /* also goes into style->Desk */
#define STYLE_BORDER_WIDTH      (1 << 2)   /* style->border_width */
#define STYLE_HANDLE_WIDTH      (1 << 3)   /* style->resize_width */
#define STYLE_DEFAULT_GEOMETRY  (1 << 4)   /* to override PPosition's geometry */
#define STYLE_VIEWPORTX         (1 << 5)   /* style->ViewportX */
#define STYLE_VIEWPORTY         (1 << 6)   /* style->ViewportY */
#define STYLE_GRAVITY           (1 << 7)   /* if user wants us to override gravity from WM_NORMAL_HINTS */
#define STYLE_LAYER             (1 << 8)   /* style->layer - layer is defined */
#define STYLE_FRAME             (1 << 9)   /* style->frame - frame is defined */
#define STYLE_WINDOWBOX         (1 << 10)  /* style->windowbox - windowbox is defined */


/* this are pure flags */
#define STYLE_KDE_HINTS         (1 << 7)    /* if set - then we should honor KDE Hints */
#define STYLE_CURRENT_VIEWPORT  (1 << 8)   /* Ignore ConfigRequests for the client */   
#define STYLE_IGNORE_CONFIG     (1 << 9)   /* Ignore ConfigRequests for the client */   
#define STYLE_LONG_LIVING       (1 << 10)
#define STYLE_STICKY            (1 << 11)
#define STYLE_TITLE             (1 << 12)
#define STYLE_CIRCULATE         (1 << 13)
#define STYLE_WINLIST           (1 << 14)
#define STYLE_START_ICONIC      (1 << 15)
#define STYLE_ICON_TITLE        (1 << 16)
#define STYLE_FOCUS             (1 << 17)
#define STYLE_AVOID_COVER       (1 << 18)
#define STYLE_VERTICAL_TITLE    (1 << 19)    /* if this window has a vertical titlebar */
#define STYLE_HANDLES           (1 << 20)
#define STYLE_PPOSITION         (1 << 21)    /* if set - then we should honor PPosition */
#define STYLE_GROUP_HINTS       (1 << 22)    /* if set - then we should use Group Hint for initial placement */
#define STYLE_TRANSIENT_HINTS   (1 << 23)    /* if set - then we should use Transient Hint for initial placement */
#define STYLE_MOTIF_HINTS       (1 << 24)    /* if set - then we should honor Motif Hints */
#define STYLE_GNOME_HINTS       (1 << 25)    /* if set - then we should honor Gnome Hints */
#define STYLE_EXTWM_HINTS       (1 << 26)    /* if set - then we should honor Extended WM Hints */
#define STYLE_XRESOURCES_HINTS  (1 << 27)    /* if set - then we should honor data from .XDefaults */
#define STYLE_FOCUS_ON_MAP      (1 << 28)    /* if set - window will be focused as soon as its mapped */


/* the following is needed for MATCH_ enum - see styledb.c for more */
#define STYLE_MYSTYLES          (1 << 29)   /*  */
#define STYLE_BUTTONS           (1 << 30)   /*  */
#define STYLE_FLAGS             (1 << 31)   /*  */

#define STYLE_DEFAULTS		(STYLE_TITLE|STYLE_CIRCULATE|STYLE_WINLIST| \
                             STYLE_FOCUS|STYLE_FOCUS_ON_MAP|STYLE_HANDLES| \
							 STYLE_ICON_TITLE|STYLE_PPOSITION|STYLE_GROUP_HINTS| \
							 STYLE_TRANSIENT_HINTS|STYLE_MOTIF_HINTS|STYLE_KDE_HINTS| \
							 STYLE_GNOME_HINTS|STYLE_EXTWM_HINTS|STYLE_LONG_LIVING)

/************************************************************************************
 * Data structure definitions :
 ************************************************************************************/
/*
 * This one is used to read up data from the config file/ save data into config file
 * ( basically just an interface with libASConfig )
 */
typedef struct name_list
{
  struct name_list *next;	/* pointer to the next list elemaent structure */
  char *name;			/* the name of the window */

  /* new approach :
   * If flag is set in both set_flags and flags
   - then it is present in config and value is On
   * If flag is set in set_flags but not in flags
   - then it is present in config and value is Off
   * if flag is not set in set_flags
   - then it is not present in config and value is Undefined
   */
  unsigned long set_flags;
  unsigned long flags;
  /* flag that defines which of the following data members are set in config: */
  unsigned long set_data_flags;

  char *icon_file;		/* icon name */
  ASGeometry default_geometry ;
  int Desk;			/* Desktop number */
  int layer;			/* layer number */
  int ViewportX, ViewportY;

  int border_width;
  int resize_width;
  int gravity ;

  char *window_styles[BACK_STYLES];
  char *frame_name;
  char *windowbox_name;

  unsigned long on_buttons;
  unsigned long off_buttons;
}
name_list;

/*
 * this is somewhat compiled/preprocessed version, suitable for database storin,
 * and searching through:
 */
typedef struct ASDatabaseRecord
{
  unsigned long magic ;         /* magic number identifying valid record */

  struct wild_reg_exp *regexp ;
  unsigned long set_flags;      /* (set_flags&flags) will get you flags that are ON */
  unsigned long flags;          /* (set_flags&(~flags)) will get you flags that are OFF */
  unsigned long set_buttons;    /* (set_buttons&buttons) will get you buttons that are ON */
  unsigned long buttons;        /* (set_buttons&(~buttons)) will get you buttons that are OFF */
  unsigned long set_data_flags; /* flag that defines which of the following data members are set in config: */
  ASGeometry default_geometry ;
  int desk;                     /* Startup Desktop number */
  int layer;                    /* Startup layer number */
  int viewport_x, viewport_y;   /* Startup viewport */
  int border_width;
  int resize_width;
  int gravity;

  char *icon_file;
  char *frame_name;
  char *windowbox_name;
  char *window_styles[BACK_STYLES];

  Bool  own_strings ;

}ASDatabaseRecord;

typedef struct ASDatabase
{
    size_t allocated_num ;

    ASDatabaseRecord *styles_table ;/* list of all available styles excluding default */
    size_t            styles_num ;  /* number of entries in above table */

    ASDatabaseRecord  style_default;/* this one is not included in above table to speed up search */

    int *match_list ;               /* indexes of matched styles in last search (-1 default style)*/
}ASDatabase;

/************************************************************************************
 * This is the high level interface that should be used by application to
 * sort through and find matches in the Database :
 ************************************************************************************/
ASDatabaseRecord *make_asdb_record ( name_list * nl, struct wild_reg_exp *regexp,
                                     ASDatabaseRecord *reusable_memory );
/* NULL terminated list of names/aliases sorted in order of descending importance */
ASDatabaseRecord *fill_asdb_record (ASDatabase *db, char **names,
                                    ASDatabaseRecord *reusable_memory, Bool dup_strings);
void destroy_asdb_record(ASDatabaseRecord *rec, Bool reusable);

ASDatabase* build_asdb( name_list *nl );
void destroy_asdb( ASDatabase **db );
/*
 * Note that Database cannot be build on stack
 * But Record can. Make sure not to destroy something you should not.
 */

void print_asdb_matched_rec( stream_func func, void *stream, ASDatabase *db, ASDatabaseRecord *db_rec );
void print_asdb( stream_func func, void *stream, ASDatabase *db );
void print_asdb_match_list( stream_func func, void *stream, ASDatabase *db );
void print_asdb_record( stream_func func, void *stream, ASDatabaseRecord *db_rec, const char *prompt );

/************************************************************************************
 * this are low level functions typically used by configuration management facilities
 * like ASCP and such :
 ************************************************************************************/
void style_init (name_list * nl);
name_list *style_new (name_list ** tail);
/* this one just copies all the pointers, without allocating any more memory */
void style_copy (name_list * to, name_list * from);
/* this one actually allocates memory for copies of the strings : */
name_list *style_dup (name_list * from);
void style_delete (name_list * style, name_list ** phead);
void delete_name_list ( name_list **head);

/* This one will have to go : */
struct ASWindow;
void style_fill_by_name (name_list * nl, struct ASWindow *t);

#ifdef __cplusplus
}
#endif



#endif /* AS_STYLE_H */
