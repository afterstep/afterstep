#ifndef BACKGROUND_H_INCLUDED
#define BACKGROUND_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#define 	BGFLAG_FILE	(1<<1)	/* if not FILE then MyStyle or CMD */
#define 	BGFLAG_MYSTYLE	(1<<2)
#define   	BGFLAG_CUT	(1<<3)
#define   	BGFLAG_TINT	(1<<4)
#define   	BGFLAG_SCALE	(1<<5)
#define   	BGFLAG_ALIGN	(1<<6)
#define 	BGFLAG_ALIGN_CENTER  (1<<7)
#define 	BGFLAG_ALIGN_RIGHT   (1<<8)
#define 	BGFLAG_ALIGN_BOTTOM  (1<<9)
#define   	BGFLAG_PAD	(1<<10)
#define 	BGFLAG_PAD_VERT (1<<11)
#define 	BGFLAG_PAD_HOR	(1<<12)
#define   BGFLAG_MIRROR	(1<<13)
#define 	BGFLAG_BAD	(1<<14)
#define 	BGFLAG_COMPLETE	(1<<15)

typedef union
  {
	Atom atom;
	Pixmap pixmap;
  }
StoredBackData;

typedef struct ASDeskBack
  {
	unsigned long desk;
	Atom data_type;		/* XA_PIXMAP if Pixmap 0 if InternAtom */
	StoredBackData data;
	Atom MyStyle;		/* we should watch for MyStyles's updates
				   if this is not NULL */
  }
ASDeskBack;

typedef struct
  {
	ASDeskBack *desks;
	unsigned long desks_num;
  }
ASDeskBackArray;

ASDeskBack *FindDeskBack (ASDeskBackArray * backs, long desk);
int IsPurePixmap (ASDeskBack * back);
void SetRootPixmapPropertyID (Atom id);
void BackgroundSetForDesk (ASDeskBackArray * backs, long desk);
void FreeDeskBackArray (ASDeskBackArray * backs, int free_pixmaps);
ASDeskBackArray *UpdateDeskBackArray (ASDeskBackArray * old_info, ASDeskBackArray * new_info);

#define AS_BACKGROUND_ATOM_DEF   {None,"_AS_BACKGROUND", XA_INTEGER}
#define XROOTPMAP_ID_ATOM_DEF    {None,"_XROOTPMAP_ID", XA_PIXMAP}

void SetBackgroundsProperty (ASDeskBackArray * backs, Atom property);
void GetBackgroundsProperty (ASDeskBackArray * backs, Atom property);

#ifdef DEBUG_BACKGROUNDS
void PrintDeskBackArray (ASDeskBackArray * backs);
#else
#define PrintDeskBackArray(x)
#endif

#ifdef __cplusplus
}
#endif


#endif
