#ifndef ASCMAP_H_HEADER_ICLUDED
#define ASCMAP_H_HEADER_ICLUDED

/***********************************************************************************/
/* reduced colormap building code :                                                */
/***********************************************************************************/
typedef struct ASMappedColor
{
	CARD8  alpha, red, green, blue;
	CARD32 indexed;
	unsigned int count ;
	int cmap_idx ;
	struct ASMappedColor *next ;
}ASMappedColor;

typedef struct ASSortedColorBucket
{
	unsigned int count ;
	ASMappedColor *head, *tail ;			/* pointers to first and last
											 * mapped colors in the stack */
	int good_offset ;                       /* skip to closest stack that
											 * has mapped colors */
}ASSortedColorBucket;

#define MAKE_INDEXED_COLOR3(red,green,blue) \
                   ((((green&0x200)|(blue&0x100)|(red&0x80))<<14))

#define MAKE_INDEXED_COLOR6(red,green,blue) \
				   (MAKE_INDEXED_COLOR3(red,green,blue)| \
		            (((green&0x100)|(blue&0x80) |(red&0x40))<<12))

#define MAKE_INDEXED_COLOR9(red,green,blue) \
                   (MAKE_INDEXED_COLOR6(red,green,blue)| \
		            (((green&0x80) |(blue&0x40) |(red&0x20))<<10))

#define MAKE_INDEXED_COLOR12(red,green,blue) \
                   (MAKE_INDEXED_COLOR9(red,green,blue)| \
					(((green&0x40) |(blue&0x20) |(red&0x10))<<8 ))

#define MAKE_INDEXED_COLOR15(red,green,blue) \
                   (MAKE_INDEXED_COLOR12(red,green,blue)| \
					(((green&0x20) |(blue&0x10) |(red&0x08))<<6 ))

#define MAKE_INDEXED_COLOR18(red,green,blue) \
                   (MAKE_INDEXED_COLOR15(red,green,blue)| \
					(((green&0x10) |(blue&0x08) |(red&0x04))<<4 ))

#define MAKE_INDEXED_COLOR21(red,green,blue) \
                   (MAKE_INDEXED_COLOR18(red,green,blue)| \
					(((green&0x08) |(blue&0x04) |(red&0x02))<<2 ))

#define MAKE_INDEXED_COLOR24(red,green,blue) \
                   (MAKE_INDEXED_COLOR21(red,green,blue)| \
					 ((green&0x04) |(blue&0x02) |(red&0x01)))

#define INDEX_SHIFT_RED(r)    (r)
#define INDEX_SHIFT_GREEN(g) ((g)<<2)
#define INDEX_SHIFT_BLUE(b) ((b)<<1)

#define INDEX_UNSHIFT_RED(r)    (r)
#define INDEX_UNSHIFT_GREEN(g)  ((g)>>2)
#define INDEX_UNSHIFT_BLUE(b)   ((b)>>1)

#define INDEX_UNESHIFT_RED(r,e)   ((r)>>(e))
#define INDEX_UNESHIFT_GREEN(g,e) ((g)>>(2+(e)))
#define INDEX_UNESHIFT_BLUE(b,e)  ((b)>>(1+(e)))


#define SLOTS_OFFSET24 15
#define SLOTS_MASK24   0x1FF
#define SLOTS_OFFSET21 12
#define SLOTS_MASK21   0x1FF

#define MAKE_INDEXED_COLOR MAKE_INDEXED_COLOR21
#define SLOTS_OFFSET	9
#define SLOTS_MASK		0xFFF
#define MAX_COLOR_BUCKETS		  4096


typedef struct ASSortedColorHash
{
	unsigned int count_unique ;
	ASSortedColorBucket *buckets ;
	int buckets_num ;
	CARD32  last_found ;
	int     last_idx ;
}ASSortedColorHash;

typedef struct ASColormapEntry
{
	CARD8 red, green, blue;
}ASColormapEntry;

typedef struct ASColormap
{
	ASColormapEntry *entries ;
	unsigned int count ;
	ASSortedColorHash *hash ;
	Bool has_opaque ;
}ASColormap;

void  add_index_color( ASSortedColorHash *index, CARD32 indexed, unsigned int slot, CARD32 red, CARD32 green, CARD32 blue );
void destroy_colorhash( ASSortedColorHash *index, Bool reusable );
unsigned int add_colormap_items( ASSortedColorHash *index, unsigned int start, unsigned int stop, unsigned int quota, unsigned int base, ASColormapEntry *entries );
void fix_colorindex_shortcuts( ASSortedColorHash *index );
ASColormap *color_hash2colormap( ASColormap *cmap, unsigned int max_colors );
void destroy_colormap( ASColormap *cmap, Bool reusable );
int  get_color_index( ASSortedColorHash *index, CARD32 indexed, unsigned int slot );
int *colormap_asimage( ASImage *im, ASColormap *cmap, unsigned int max_colors, unsigned int dither, int opaque_threshold );


#endif
