
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

typedef struct ASSortedColorStack
{
	unsigned int count ;
	ASMappedColor *head, *tail ;			/* pointers to first and last
											 * mapped colors in the stack */
	int good_offset ;                       /* skip to closest stack that
											 * has mapped colors */
}ASSortedColorStack;

#define MAKE_INDEXED_COLOR24(red,green,blue) \
                   ((((green&0x200)|(blue&0x100)|(red&0x80))<<14)| \
		            (((green&0x100)|(blue&0x80) |(red&0x40))<<12)| \
		            (((green&0x80) |(blue&0x40) |(red&0x20))<<10)| \
					(((green&0x40) |(blue&0x20) |(red&0x10))<<8)| \
					(((green&0x20) |(blue&0x10) |(red&0x08))<<6)| \
					(((green&0x10) |(blue&0x08) |(red&0x04))<<4)| \
					(((green&0x08) |(blue&0x04) |(red&0x02))<<2)| \
					 ((green&0x04) |(blue&0x02) |(red&0x01)))
#define MAKE_INDEXED_COLOR21(red,green,blue) \
                   ((((green&0x200)|(blue&0x100)|(red&0x80))<<12)| \
		            (((green&0x100)|(blue&0x80) |(red&0x40))<<10)| \
		            (((green&0x80) |(blue&0x40) |(red&0x20))<<8)| \
					(((green&0x40) |(blue&0x20) |(red&0x10))<<6)| \
					(((green&0x20) |(blue&0x10) |(red&0x08))<<4)| \
					(((green&0x10) |(blue&0x08) |(red&0x04))<<2)| \
					(((green&0x08) |(blue&0x04) |(red&0x02))>>1))

#define SLOTS_OFFSET24 15
#define SLOTS_MASK24   0x1FF
#define SLOTS_OFFSET21 12
#define SLOTS_MASK21   0x1FF

#define MAKE_INDEXED_COLOR MAKE_INDEXED_COLOR21
#define SLOTS_OFFSET	9
#define SLOTS_MASK		0xFFF
#define COLOR_STACK_SLOTS		  4096


typedef struct ASSortedColorIndex
{
	unsigned int count_unique ;
	ASSortedColorStack stacks[COLOR_STACK_SLOTS] ;
	CARD32  last_found ;
	int     last_idx ;
}ASSortedColorIndex;

typedef struct ASColormapEntry
{
	CARD8 red, green, blue;
}ASColormapEntry;

typedef struct ASColormap
{
	ASColormapEntry *entries ;
	unsigned int count ;
	ASSortedColorIndex *index ;
}ASColormap;


int          add_index_color( ASSortedColorIndex *index, 
                              CARD32 red, CARD32 green, CARD32 blue );
void         destroy_colorindex( ASSortedColorIndex *index, Bool reusable );

unsigned int add_colormap_items( ASSortedColorIndex *index,  
                                 unsigned int start, unsigned int stop, 
								 unsigned int quota, unsigned int base, 
								 ASColormapEntry *entries );
void         fix_colorindex_shortcuts( ASSortedColorIndex *index );
ASColormap  *color_index2colormap( ASSortedColorIndex *index,  
                                   unsigned int max_colors, 
								   ASColormap *reusable_memory );
void         destroy_colormap( ASColormap *cmap, Bool reusable );

int          get_color_index( ASSortedColorIndex *index, CARD32 red, 
                              CARD32 green, CARD32 blue );
void         quantize_scanline( ASScanline *scl, ASSortedColorIndex *index, 
                                ASColormap *cmap, int *dst, 
								int transp_threshold, int transp_idx );
