#ifndef XCF_H_INCLUDED
#define XCF_H_INCLUDED

/* GIMP's XCF file properties/structures : */

#define XCF_MAX_CHANNELS     4

#define XCF_GRAY_PIX         0
#define XCF_ALPHA_G_PIX      1
#define XCF_RED_PIX          0
#define XCF_GREEN_PIX        1
#define XCF_BLUE_PIX         2
#define XCF_ALPHA_PIX        3
#define XCF_INDEXED_PIX      0
#define XCF_ALPHA_I_PIX      1

#define XCF_COLORMAP_SIZE    768

typedef enum
{
  XCF_PROP_END = 0,
  XCF_PROP_COLORMAP = 1,
  XCF_PROP_ACTIVE_LAYER = 2,
  XCF_PROP_ACTIVE_CHANNEL = 3,
  XCF_PROP_SELECTION = 4,
  XCF_PROP_FLOATING_SELECTION = 5,
  XCF_PROP_OPACITY = 6,
  XCF_PROP_MODE = 7,
  XCF_PROP_VISIBLE = 8,
  XCF_PROP_LINKED = 9,
  XCF_PROP_PRESERVE_TRANSPARENCY = 10,
  XCF_PROP_APPLY_MASK = 11,
  XCF_PROP_EDIT_MASK = 12,
  XCF_PROP_SHOW_MASK = 13,
  XCF_PROP_SHOW_MASKED = 14,
  XCF_PROP_OFFSETS = 15,
  XCF_PROP_COLOR = 16,
  XCF_PROP_COMPRESSION = 17,
  XCF_PROP_GUIDES = 18,
  XCF_PROP_RESOLUTION = 19,
  XCF_PROP_TATTOO = 20,
  XCF_PROP_PARASITES = 21,
  XCF_PROP_UNIT = 22,
  XCF_PROP_PATHS = 23,
  XCF_PROP_USER_UNIT = 24,
  XCF_PROP_Total = 25
} XcfPropType;

typedef enum
{
  XCF_COMPRESS_NONE = 0,
  XCF_COMPRESS_RLE = 1,
  XCF_COMPRESS_ZLIB = 2,
  XCF_COMPRESS_FRACTAL = 3  /* Unused. */
} XcfCompressionType;

typedef enum
{
  XCF_RED_CHANNEL,
  XCF_GREEN_CHANNEL,
  XCF_BLUE_CHANNEL,
  XCF_GRAY_CHANNEL,
  XCF_INDEXED_CHANNEL,
  XCF_ALPHA_CHANNEL,
  XCF_AUXILLARY_CHANNEL
} XcfChannelType;

typedef enum
{
  XCF_EXPAND_AS_NECESSARY,
  XCF_CLIP_TO_IMAGE,
  XCF_CLIP_TO_BOTTOM_LAYER,
  XCF_FLATTEN_IMAGE
} XcfMergeType;

#define XCF_SIGNATURE      		"gimp xcf"
#define XCF_SIGNATURE_LEN  		8              /* use in strncmp() */
#define XCF_SIGNATURE_FULL 		"gimp xcf file"
#define XCF_SIGNATURE_FULL_LEN 	13             /* use in seek() */

struct XcfProperty;


typedef struct XcfImage
{
	CARD32 		width;
	CARD32 		height;
	CARD32 		type ;

	struct XcfProperty   *properties ;
	struct XcfLayer		 *layers;
	struct XcfChannel	 *channels;
}XcfImage;

typedef struct XcfProperty
{
	CARD32 		id ;
	CARD32		len;
	CARD8	   *data;

	struct XcfProperty *next;
}XcfProperty;

typedef struct XcfLayer
{
	CARD32 		offset ;
	/* layer data goes here */
	CARD32	    width ;
	CARD32	    height ;
	CARD32	    type ;
	/* we don't give a damn about layer's name - skip it */
	struct XcfProperty   *properties ;
	struct XcfHierarchy	 *hierarchy ;
	struct XcfLayerMask	 *masks ;

	struct XcfLayer *next;
}XcfLayer;

typedef struct XcfChannel
{
	CARD32 		offset ;
	/* layer data goes here */
	CARD32	    width ;
	CARD32	    height ;
	CARD32	    type ;
	/* we don't give a damn about layer's name - skip it */
	struct XcfProperty   *properties ;
	struct XcfHierarchy	 *hierarchy ;

	struct XcfChannel *next;
}XcfChannel;

typedef struct XcfLayerMask
{
	CARD32 		offset ;
	/* layer data goes here */
	CARD32	    width ;
	CARD32	    height ;
	/* we don't give a damn about layer's name - skip it */
	struct XcfProperty   *properties ;
	struct XcfHierarchy	 *hierarchy ;

	struct XcfLayerMask *next;
}XcfLayerMask;

typedef struct XcfHierarchy
{
	CARD32 		offset ;
	/* layer data goes here */
	CARD32	    width ;
	CARD32	    height ;
	CARD32		bpp ;

	/* we don't give a damn about layer's name - skip it */
	struct XcfLevel	 	 *levels ;

	struct XcfLayerMask *next;
}XcfHierarchy;

typedef struct XcfLevel
{
	CARD32 		offset ;
	CARD32	    width ;
	CARD32	    height ;

	struct XcfLevel *next ;
}XcfLevel;

#endif /* #ifndef XCF_H_INCLUDED */
