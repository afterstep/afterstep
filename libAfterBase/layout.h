#ifndef AS_LAYOUT_H_HEADER_FILE
#define AS_LAYOUT_H_HEADER_FILE

/****h* afterstep/libAfterBase/layout.h
 * SYNOPSIS
 * Defines main structures and function for handling of complex layout
 * of diverse elements.
 * DESCRIPTION
 * The need for this interface comes from the fact that there is a need
 * to place multiple elements within the window, with different
 * characteristics, like fixed width/height, overlapping elements, etc.
 *
 * We define layout (MyLayout) as a two-dimentional grid of rows and
 * columns, on the crossing points of which layout elements (MyLayoutElem)
 * are located. Some of the crossings may be empty, some elements may
 * span several rows and/or columns. Some elements may have fixed size,
 * like icons for example, or height of the text label. In that case
 * depending on spanning situation entire column/row may be declared as
 * having fixed width/height.
 *
 * Entire layout maybe resized and/or its individual elements may be
 * resized, in which case we have dynamically adjust position and sizes
 * of adjusent elements.
 *
 * SEE ALSO
 * Structures :
 *          MyLayoutElem
 *          MyLayout
 *
 * Functions :
 *
 *
 * Other libAfterBase modules :
 *          safemalloc.h
 * AUTHOR
 * Sasha Vasko <sashav at sprintmail dot com>
 ******
 */

#define LF_FixedWidth           (0x01<<0)
#define LF_FixedHeight          (0x01<<1)
#define LF_FixedSize            (LF_FixedWidth|LF_FixedHeight)


typedef struct ASLayoutElem
{
    unsigned char flags ;
    unsigned char bw ;
    unsigned char h_span, v_span ;/* number of grid elements we occupy
                                   * we need that to facilitate overlapping
                                   */
    short x, y ;
    unsigned short width, height ;/* current width and height */
    unsigned short fixed_width, fixed_height ;

	unsigned char row, column ;
	unsigned char padding[2] ;
    int context;
    struct ASLayoutElem *right, *below ;
}ASLayoutElem;

typedef struct ASLayout
{/* we allocate entire memory at once, and then distribute it among other members */
 /* relative to the parent window */
    int offset_east, offset_north, offset_west, offset_south ;
    int x, y ;
    unsigned int width, height ;
    unsigned short h_border, v_border, h_spacing, v_spacing ;

#define ASLAYOUT_MAX_SIZE       64             /* 64x64 seems like a huge grid, */
                                               /* should be ample for anything  */
    unsigned short dim_x, dim_y ;

    ASLayoutElem **rows, **cols;                /* 2 dimentional array of pointers */

    ASLayoutElem *disabled ;                    /* elements that has been temporarily
                                                 * taken out of the circulation */
}ASLayout;


ASLayout *create_aslayout( unsigned int dim_x, unsigned int dim_y );
void destroy_aslayout( ASLayout **playout );


void insert_layout_elem( ASLayout *layout, ASLayoutElem *elem,
		    		     unsigned int h_slot, unsigned int v_slot,
                 	     unsigned int h_span, unsigned int v_span );
ASLayoutElem *gather_layout_elems( ASLayout *layout );
ASLayoutElem *extract_layout_context( ASLayout *layout, int context );
ASLayoutElem *find_layout_context( ASLayout *layout, int context );

void flush_layout_elems( ASLayout *layout );

void disable_layout_elem( ASLayout *layout, ASLayoutElem **pelem );
void enable_layout_elem( ASLayout *layout, ASLayoutElem **pelem );
int  disable_layout_context( ASLayout *layout, int context, Bool batch );
int  enable_layout_context( ASLayout *layout, int context, Bool batch );


void set_layout_spacing( ASLayout *layout,
	                     unsigned int h_border, unsigned int v_border,
						 unsigned int h_spacing, unsigned int v_spacing );
void set_layout_offsets( ASLayout *layout,
	                     int east, int north, int west, int south );

/* fixed size handling : */
void get_layout_fixed_size( ASLayout *layout,
	                        CARD32 *fixed_width,
	                        CARD32 *fixed_height );
Bool get_layout_context_size( ASLayout *layout, int context,
	                          int *x, int *y,
							  unsigned int *width, unsigned int *height );
ASLayoutElem *find_layout_point( ASLayout *layout, int x, int y, ASLayoutElem *start );


ASFlagType set_layout_context_fixed_size( ASLayout *layout, int context,
			                              unsigned int width,
										  unsigned int height );

Bool moveresize_layout( ASLayout *layout,
	                    unsigned int width, unsigned int height,
						Bool force );


#endif  /* AS_LAYOUT_H_HEADER_FILE */

