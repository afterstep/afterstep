/*
 * Copyright (c) 2000,2001 Sasha Vasko <sasha at aftercode.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#undef LOCAL_DEBUG
#undef DO_CLOCKING
#ifndef NO_DEBUG_OUTPUT
#undef DEBUG_RECTS
#undef DEBUG_RECTS2
#endif

#ifdef _WIN32
#include "win32/config.h"
#else
#include "config.h"
#endif


#define USE_64BIT_FPU

#include <string.h>
#ifdef DO_CLOCKING
#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif

#ifdef _WIN32
# include "win32/afterbase.h"
#else
# include "afterbase.h"
#endif
#include "asvisual.h"
#include "blender.h"
#include "asimage.h"

static ASVisual __as_dummy_asvisual = {0};
static ASVisual *__as_default_asvisual = &__as_dummy_asvisual ;

/* these are internal library things - user should not mess with it ! */
ASVisual *_set_default_asvisual( ASVisual *new_v )
{
	ASVisual *old_v = __as_default_asvisual ;
	__as_default_asvisual = new_v?new_v:&__as_dummy_asvisual ;
	return old_v;
}

ASVisual *get_default_asvisual()
{
	return __as_default_asvisual?__as_default_asvisual:&__as_dummy_asvisual;
}

/* internal buffer used for compression/decompression */

static CARD8 *__as_compression_buffer = NULL ;
static size_t __as_compression_buffer_len = 0;   /* allocated size */

static inline CARD8* get_compression_buffer( size_t size )
{
	if( size > __as_compression_buffer_len )
 		__as_compression_buffer_len = (size+1023)&(~0x03FF) ;
	return (__as_compression_buffer = realloc( __as_compression_buffer, __as_compression_buffer_len ));
}

static inline void release_compression_buffer( CARD8 *ptr )
{
	/* do nothing so far */
}



#ifdef TRACK_ASIMAGES
static ASHashTable *__as_image_registry = NULL ;
#endif

#ifdef HAVE_MMX
Bool asimage_use_mmx = True;
#else
Bool asimage_use_mmx = False;
#endif
/* ********************************************************************/
/* initialization routines 											  */
/* *********************************************************************/
/* ************************** MMX **************************************/
/*inline extern*/
int mmx_init(void)
{
int mmx_available = 0;
#ifdef HAVE_MMX
	asm volatile (
                      /* Get CPU version information */
                      "pushl %%eax\n\t"
                      "pushl %%ebx\n\t"
                      "pushl %%ecx\n\t"
                      "pushl %%edx\n\t"
                      "movl $1, %%eax\n\t"
                      "cpuid\n\t"
                      "andl $0x800000, %%edx \n\t"
					  "movl %%edx, %0\n\t"
                      "popl %%edx\n\t"
                      "popl %%ecx\n\t"
                      "popl %%ebx\n\t"
                      "popl %%eax\n\t"
                      : "=m" (mmx_available)
                      : /* no input */
					  : "ebx", "ecx", "edx"
			  );
#endif
	return mmx_available;
}

int mmx_off(void)
{
#ifdef HAVE_MMX
	__asm__ __volatile__ (
                      /* exit mmx state : */
                      "emms \n\t"
                      : /* no output */
                      : /* no input  */
			  );
#endif
	return 0;
}

/* *********************   ASImage  ************************************/
void
asimage_init (ASImage * im, Bool free_resources)
{
	if (im != NULL)
	{
		if (free_resources)
		{
			register int i ;
			if( get_flags( im->flags, ASIM_STATIC ) ) 
				free( im->memory.pmem );
			else
				for( i = im->height*4-1 ; i>= 0 ; --i )
					if( im->red[i] )
						free( im->red[i] );
			if( im->red )
				free(im->red);
#ifndef X_DISPLAY_MISSING
			if( im->alt.ximage )
				XDestroyImage( im->alt.ximage );
			if( im->alt.mask_ximage )
				XDestroyImage( im->alt.mask_ximage );
#endif
			if( im->alt.argb32 )
				free( im->alt.argb32 );
			if( im->alt.vector )
				free( im->alt.vector );
		}
		memset (im, 0x00, sizeof (ASImage));
		im->magic = MAGIC_ASIMAGE ;
		im->back_color = ARGB32_DEFAULT_BACK_COLOR ;
	}
}

void
flush_asimage_cache( ASImage *im )
{
#ifndef X_DISPLAY_MISSING
	if( im->alt.ximage )
    {
		XDestroyImage( im->alt.ximage );
        im->alt.ximage = NULL ;
    }
	if( im->alt.mask_ximage )
    {
		XDestroyImage( im->alt.mask_ximage );
        im->alt.mask_ximage = NULL ;
    }
#endif
}

static void
alloc_asimage_channels ( ASImage *im )
{
	/* we want result to be 32bit aligned and padded */
	im->red = safecalloc (1, sizeof (CARD8*) * im->height * 4);
	if( im->red == NULL )
	{
		show_error( "Insufficient memory to create image %dx%d!", im->width, im->height );
		if( im->red )
			free( im->red );
		return ;
	}

	im->green = im->red+im->height;
	im->blue = 	im->red+(im->height*2);
	im->alpha = im->red+(im->height*3);
	im->channels[IC_RED] = im->red ;
	im->channels[IC_GREEN] = im->green ;
	im->channels[IC_BLUE] = im->blue ;
	im->channels[IC_ALPHA] = im->alpha ;
}

void
asimage_start (ASImage * im, unsigned int width, unsigned int height, unsigned int compression)
{
	if (im)
	{
		asimage_init (im, True);
		im->height = height;
		im->width = width;
		
		alloc_asimage_channels( im );

		im->max_compressed_width = width*compression/100;
		if( im->max_compressed_width > im->width )
			im->max_compressed_width = im->width ;
	}
}

void
asimage_start_static(ASImage * im, unsigned int width, unsigned int height, unsigned int compression)
{
	if (im)
	{
		register unsigned int i;
		register CARD8 *ptr ;
		int line_size = ((width+3)/4)*4 ;

		asimage_init (im, True);
		set_flags( im->flags, ASIM_STATIC ); 

		im->height = height;
		im->width = width;
		im->max_compressed_width = width*compression/100;
		if( im->max_compressed_width > im->width )
			im->max_compressed_width = im->width ;

		alloc_asimage_channels( im );

		im->memory.pmem = ptr = safemalloc( line_size*height * 4 );
		for( i = 0 ; i < height ; ++i ) 
		{
			im->red[i] = ptr ;
			ptr += line_size ; 
			im->green[i] = ptr ;
			ptr += line_size ; 
			im->blue[i] = ptr ;
			ptr += line_size ; 
			im->alpha[i] = ptr ;
			ptr += line_size ; 
	
		}		
	}
}


static ASImage* check_created_asimage( ASImage *im, unsigned int width, unsigned int height )
{
	if( im->width == 0 || im->height == 0 )
	{
		free( im );
		im = NULL ;
#ifdef TRACK_ASIMAGES
        show_error( "failed to create ASImage of size %dx%d", width, height );
#endif
    }else
    {
#ifdef TRACK_ASIMAGES
        show_progress( "created %s ASImage %p of size %dx%d with compression %d", 
						get_flags( im->flags, ASIM_STATIC)?"static":"dynamic", im,  
						width, height, (im->max_compressed_width*100)/im->width );
        if( __as_image_registry == NULL )
            __as_image_registry = create_ashash( 0, pointer_hash_value, NULL, NULL );
        add_hash_item( __as_image_registry, AS_HASHABLE(im), im );
#endif
    }
	return im ;
}

ASImage *
create_asimage( unsigned int width, unsigned int height, unsigned int compression)
{
	ASImage *im = safecalloc( 1, sizeof(ASImage) );
	asimage_start( im, width, height, compression );
	return check_created_asimage( im, width, height );
}

ASImage *
create_static_asimage( unsigned int width, unsigned int height, unsigned int compression)
{
	ASImage *im = safecalloc( 1, sizeof(ASImage) );
	asimage_start_static( im, width, height, compression );
	return check_created_asimage( im, width, height );
}


void
destroy_asimage( ASImage **im )
{
	if( im )
		if( *im && !AS_ASSERT_NOTVAL((*im)->imageman,NULL))
		{
#ifdef TRACK_ASIMAGES
            show_progress( "destroying ASImage %p of size %dx%d", *im, (*im)->width, (*im)->height );
            remove_hash_item( __as_image_registry, AS_HASHABLE(*im), NULL, False );
#endif
			asimage_init( *im, True );
			(*im)->magic = 0;
			free( *im );
			*im = NULL ;
		}
}

void print_asimage_func (ASHashableValue value)
{
    ASImage *im = (ASImage*)value ;
    if( im && im->magic == MAGIC_ASIMAGE )
    {
        unsigned int k;
        unsigned int red_mem = 0, green_mem = 0, blue_mem = 0, alpha_mem = 0;
        unsigned int red_count = 0, green_count = 0, blue_count = 0, alpha_count = 0;
        fprintf( stderr,"\n\tASImage[%p].size = %dx%d;\n",  im, im->width, im->height );
        fprintf( stderr,"\tASImage[%p].back_color = 0x%lX;\n", im, (long)im->back_color );
        fprintf( stderr,"\tASImage[%p].max_compressed_width = %d;\n", im, im->max_compressed_width);
        fprintf( stderr,"\t\tASImage[%p].alt.ximage = %p;\n", im, im->alt.ximage );
        if( im->alt.ximage )
        {
            fprintf( stderr,"\t\t\tASImage[%p].alt.ximage.bytes_per_line = %d;\n", im, im->alt.ximage->bytes_per_line);
            fprintf( stderr,"\t\t\tASImage[%p].alt.ximage.size = %dx%d;\n", im, im->alt.ximage->width, im->alt.ximage->height);
        }
        fprintf( stderr,"\t\tASImage[%p].alt.mask_ximage = %p;\n", im, im->alt.mask_ximage);
        if( im->alt.mask_ximage )
        {
            fprintf( stderr,"\t\t\tASImage[%p].alt.mask_ximage.bytes_per_line = %d;\n", im, im->alt.mask_ximage->bytes_per_line);
            fprintf( stderr,"\t\t\tASImage[%p].alt.mask_ximage.size = %dx%d;\n", im, im->alt.mask_ximage->width, im->alt.mask_ximage->height);
        }
        fprintf( stderr,"\t\tASImage[%p].alt.argb32 = %p;\n", im, im->alt.argb32 );
        fprintf( stderr,"\t\tASImage[%p].alt.vector = %p;\n", im, im->alt.vector );
        fprintf( stderr,"\tASImage[%p].imageman = %p;\n", im, im->imageman );
        fprintf( stderr,"\tASImage[%p].ref_count = %d;\n", im, im->ref_count );
        fprintf( stderr,"\tASImage[%p].name = \"%s\";\n", im, im->name );
        fprintf( stderr,"\tASImage[%p].flags = 0x%lX;\n", im, im->flags );

        for( k = 0 ; k < im->height ; k++ )
    	{
            unsigned int tmp ;

            tmp = asimage_print_line( im, IC_RED  , k, 0 );
            if( tmp > 0 )
            {
                ++red_count;
                red_mem += tmp ;
            }
            tmp = asimage_print_line( im, IC_GREEN, k, 0 );
            if( tmp > 0 )
            {
                ++green_count;
                green_mem += tmp ;
            }
            tmp = asimage_print_line( im, IC_BLUE , k, 0 );
            if( tmp > 0 )
            {
                ++blue_count;
                blue_mem += tmp ;
            }
            tmp = asimage_print_line( im, IC_ALPHA, k, 0 );
            if( tmp > 0 )
            {
                ++alpha_count;
                alpha_mem += tmp ;
            }
        }

        fprintf( stderr,"\tASImage[%p].uncompressed_size = %d;\n", im, im->width*red_count +
                                                                    im->width*green_count +
                                                                    im->width*blue_count +
                                                                    im->width*alpha_count );
        fprintf( stderr,"\tASImage[%p].compressed_size = %d;\n",   im, red_mem + green_mem +blue_mem + alpha_mem );
        fprintf( stderr,"\t\tASImage[%p].channel[red].lines_count = %d;\n", im, red_count );
        fprintf( stderr,"\t\tASImage[%p].channel[red].memory_used = %d;\n", im, red_mem );
        fprintf( stderr,"\t\tASImage[%p].channel[green].lines_count = %d;\n", im, green_count );
        fprintf( stderr,"\t\tASImage[%p].channel[green].memory_used = %d;\n", im, green_mem );
        fprintf( stderr,"\t\tASImage[%p].channel[blue].lines_count = %d;\n", im, blue_count );
        fprintf( stderr,"\t\tASImage[%p].channel[blue].memory_used = %d;\n", im, blue_mem );
        fprintf( stderr,"\t\tASImage[%p].channel[alpha].lines_count = %d;\n", im, alpha_count );
        fprintf( stderr,"\t\tASImage[%p].channel[alpha].memory_used = %d;\n", im, alpha_mem );
    }
}

void
print_asimage_registry()
{
#ifdef TRACK_ASIMAGES
    print_ashash( __as_image_registry, print_asimage_func );
#endif
}

void
purge_asimage_registry()
{
#ifdef TRACK_ASIMAGES
    if( __as_image_registry )
        destroy_ashash( &__as_image_registry );
#endif
}

/* ******************** ASImageManager ****************************/
void
asimage_destroy (ASHashableValue value, void *data)
{
	if( data )
	{
		ASImage *im = (ASImage*)data ;
		if( im != NULL )
		{
			if( AS_ASSERT_NOTVAL(im->magic, MAGIC_ASIMAGE) )
				im = NULL ;
			else
				im->imageman = NULL ;
		}
		free( (char*)value );
		destroy_asimage( &im );
	}
}

ASImageManager *create_image_manager( struct ASImageManager *reusable_memory, double gamma, ... )
{
	ASImageManager *imman = reusable_memory ;
	int i ;
	va_list ap;

	if( imman == NULL )
		imman = safecalloc( 1, sizeof(ASImageManager));
	else
		memset( imman, 0x00, sizeof(ASImageManager));

	va_start (ap, gamma);
	for( i = 0 ; i < MAX_SEARCH_PATHS ; i++ )
	{
		char *path = va_arg(ap,char*);
		if( path == NULL )
			break;
		imman->search_path[i] = mystrdup( path );
	}
	va_end (ap);

	imman->search_path[MAX_SEARCH_PATHS] = NULL ;
	imman->gamma = gamma ;

	imman->image_hash = create_ashash( 7, string_hash_value, string_compare, asimage_destroy );

	return imman;
}

void
destroy_image_manager( struct ASImageManager *imman, Bool reusable )
{
	if( imman )
	{
		int i = MAX_SEARCH_PATHS;
		destroy_ashash( &(imman->image_hash) );
		while( --i >= 0 )
			if(imman->search_path[i])
				free( imman->search_path[i] );

		if( !reusable )
			free( imman );
		else
			memset( imman, 0x00, sizeof(ASImageManager));
	}
}

Bool
store_asimage( ASImageManager* imageman, ASImage *im, const char *name )
{
	Bool res = False ;
	if( !AS_ASSERT(im) )
		if( AS_ASSERT_NOTVAL(im->magic, MAGIC_ASIMAGE) )
			im = NULL ;
	if( !AS_ASSERT(imageman) && !AS_ASSERT(im) && !AS_ASSERT((char*)name) )
		if( im->imageman == NULL )
		{
			int hash_res ;
			im->name = mystrdup( name );
			hash_res = add_hash_item( imageman->image_hash, (ASHashableValue)(char*)(im->name), im);
			res = ( hash_res == ASH_Success);
			if( !res )
			{
				free( im->name );
				im->name = NULL ;
			}else
			{
				im->imageman = imageman ;
				im->ref_count = 1 ;
			}
		}
	return res ;
}

inline ASImage *
query_asimage( ASImageManager* imageman, const char *name )
{
	ASImage *im = NULL ;
	if( !AS_ASSERT(imageman) && !AS_ASSERT(name) )
	{
		ASHashData hdata = {0} ;
		if( get_hash_item( imageman->image_hash, AS_HASHABLE((char*)name), &hdata.vptr) == ASH_Success )
		{
			im = hdata.vptr ;
			if( im->magic != MAGIC_ASIMAGE )
				im = NULL ;
        }
	}
	return im;
}

ASImage *
fetch_asimage( ASImageManager* imageman, const char *name )
{
    ASImage *im = query_asimage( imageman, name );
    if( im )
	{
        im->ref_count++ ;
	}
	return im;
}


ASImage *
dup_asimage( ASImage* im )
{
	if( !AS_ASSERT(im) )
		if( AS_ASSERT_NOTVAL(im->magic,MAGIC_ASIMAGE) )
			im = NULL ;

	if( !AS_ASSERT(im) && !AS_ASSERT(im->imageman) )
	{
/*		fprintf( stderr, __FUNCTION__" on image %p ref_count = %d\n", im, im->ref_count ); */
		im->ref_count++ ;
		return im;
	}
	return NULL ;
}

inline int
release_asimage( ASImage *im )
{
	int res = -1 ;
	if( !AS_ASSERT(im) )
	{
		if( im->magic == MAGIC_ASIMAGE )
		{
			if( --(im->ref_count) <= 0 )
			{
				ASImageManager *imman = im->imageman ;
				if( !AS_ASSERT(imman) )
                    if( remove_hash_item(imman->image_hash, (ASHashableValue)(char*)im->name, NULL, True) != ASH_Success )
                        destroy_asimage( &im );
			}else
				res = im->ref_count ;
		}
	}
	return res ;
}

void
forget_asimage( ASImage *im )
{
	if( !AS_ASSERT(im) )
	{
		if( im->magic == MAGIC_ASIMAGE )
		{
			ASImageManager *imman = im->imageman ;
			if( !AS_ASSERT(imman) )
				remove_hash_item(imman->image_hash, (ASHashableValue)(char*)im->name, NULL, False);
/*            im->ref_count = 0;     release_asimage should still correctly work !!!!
 *            im->imageman = NULL;
 */
		}
	}
}

void
forget_asimage_name( ASImageManager *imman, const char *name )
{
    if( !AS_ASSERT(imman) && name != NULL )
	{
        remove_hash_item(imman->image_hash, AS_HASHABLE((char*)name), NULL, False);
    }
}

inline int
safe_asimage_destroy( ASImage *im )
{
	int res = -1 ;
	if( !AS_ASSERT(im) )
	{
		if( im->magic == MAGIC_ASIMAGE )
		{
			ASImageManager *imman = im->imageman ;
			if( imman != NULL )
			{
                res = --(im->ref_count) ;
                if( im->ref_count <= 0 )
					remove_hash_item(imman->image_hash, (ASHashableValue)(char*)im->name, NULL, True);
            }else
			{
				destroy_asimage( &im );
				res = -1 ;
			}
		}
	}
	return res ;
}

int
release_asimage_by_name( ASImageManager *imageman, char *name )
{
	int res = -1 ;
	ASImage *im = NULL ;
	if( !AS_ASSERT(imageman) && !AS_ASSERT(name) )
	{
		ASHashData hdata ;
		if( get_hash_item( imageman->image_hash, AS_HASHABLE((char*)name), &hdata.vptr) == ASH_Success )
		{
			im = hdata.vptr ;
			res = release_asimage( im );
		}
	}
	return res ;
}

void
print_asimage_manager(ASImageManager *imageman)
{
    print_ashash( imageman->image_hash, string_print );
}

/* ******************** ASGradient ****************************/

void
destroy_asgradient( ASGradient **pgrad )
{
	if( pgrad && *pgrad )
	{
		if( (*pgrad)->color )
		{
			free( (*pgrad)->color );
			(*pgrad)->color = NULL ;
		}
		if( (*pgrad)->offset )
		{
			free( (*pgrad)->offset );
			(*pgrad)->offset = NULL ;
		}
		(*pgrad)->npoints = 0 ;
		free( *pgrad );
		*pgrad = NULL ;
	}

}

ASGradient *
flip_gradient( ASGradient *orig, int flip )
{
	ASGradient *grad ;
	int npoints ;
	int type ;
	Bool inverse_points = False ;

	flip &= FLIP_MASK ;
	if( orig == NULL || flip == 0 )
		return orig;

	grad = safecalloc( 1, sizeof(ASGradient));

	grad->npoints = npoints = orig->npoints ;
	type = orig->type ;
    grad->color = safemalloc( npoints*sizeof(ARGB32) );
    grad->offset = safemalloc( npoints*sizeof(double) );

	if( get_flags(flip, FLIP_VERTICAL) )
	{
		Bool upsidedown = get_flags(flip, FLIP_UPSIDEDOWN) ;
		switch(type)
		{
			case GRADIENT_Left2Right  :
				type = GRADIENT_Top2Bottom ; inverse_points = !upsidedown ;
				break;
			case GRADIENT_TopLeft2BottomRight :
				type = GRADIENT_BottomLeft2TopRight ; inverse_points = upsidedown ;
				break;
			case GRADIENT_Top2Bottom  :
				type = GRADIENT_Left2Right ; inverse_points = upsidedown ;
				break;
			case GRADIENT_BottomLeft2TopRight :
				type = GRADIENT_TopLeft2BottomRight ; inverse_points = !upsidedown ;
				break;
		}
	}else if( flip == FLIP_UPSIDEDOWN )
	{
		inverse_points = True ;
	}

	grad->type = type ;
	if( inverse_points )
    {
        register int i = 0, k = npoints;
        while( --k >= 0 )
        {
            grad->color[i] = orig->color[k] ;
            grad->offset[i] = 1.0 - orig->offset[k] ;
			++i ;
        }
    }else
	{
        register int i = npoints ;
        while( --i >= 0 )
        {
            grad->color[i] = orig->color[i] ;
            grad->offset[i] = orig->offset[i] ;
        }
    }
	return grad;
}

/* ******************** ASImageLayer ****************************/

inline void
init_image_layers( register ASImageLayer *l, int count )
{
	memset( l, 0x00, sizeof(ASImageLayer)*count );
	while( --count >= 0 )
	{
		l[count].merge_scanlines = alphablend_scanlines ;
/*		l[count].solid_color = ARGB32_DEFAULT_BACK_COLOR ; */
	}
}

ASImageLayer *
create_image_layers( int count )
{
	ASImageLayer *l = NULL;

	if( count > 0 )
	{
		l = safecalloc( count, sizeof(ASImageLayer) );
		init_image_layers( l, count );
	}
	return l;
}

void
destroy_image_layers( register ASImageLayer *l, int count, Bool reusable )
{
	if( l )
	{
		register int i = count;
		while( --i >= 0 )
		{
			if( l[i].im )
			{
				if( l[i].im->imageman )
					release_asimage( l[i].im );
				else
					destroy_asimage( &(l[i].im) );
			}
			if( l[i].bevel )
				free( l[i].bevel );
		}
		if( !reusable )
			free( l );
		else
			memset( l, 0x00, sizeof(ASImageLayer)*count );
	}
}



/* **********************************************************************/
/*  Compression/decompression 										   */
/* **********************************************************************/
static void
asimage_apply_buffer (ASImage * im, ColorPart color, unsigned int y, CARD8 *buffer, size_t buf_used)
{
	register int len = (buf_used>>2)+1 ;
	CARD8  **part = im->channels[color];
	register CARD32  *dwsrc = (CARD32*)buffer;
	register CARD32  *dwdst;
	
	if( get_flags( im->flags, ASIM_STATIC ) ) 
		dwdst = (CARD32*)part[y] ;			
	else
	{
		dwdst = safemalloc( sizeof(CARD32)*len);	
		if (part[y] != NULL)
			free (part[y]);
		part[y] = (CARD8*)dwdst;
	}
	
	while( --len >= 0 )
		dwdst[len] = dwsrc[len];

}

size_t
asimage_add_line_mono (ASImage * im, ColorPart color, register CARD8 value, unsigned int y)
{
	register CARD8 *dst;
	int rep_count;
	int buf_used = 0;

	if (AS_ASSERT(im) || color <0 || color >= IC_NUM_CHANNELS )
		return 0;
	if( y >= im->height)
		return 0;

	if( get_flags( im->flags, ASIM_STATIC ) ) 
	{
		CARD8       **part = im->channels[color];
		memset(part[y], value, im->width);
		return im->width ;
	}

	if( (dst = get_compression_buffer(max(RLE_THRESHOLD+2,4))) == NULL )
		return 0;

	if( im->width <= RLE_THRESHOLD )
	{
		int i = im->width+1 ;
		dst[0] = (CARD8) RLE_DIRECT_TAIL;
		dst[i] = 0 ;
		while( --i > 0 )
			dst[i] = (CARD8) value;
		buf_used = 1+im->width+1;
	}else
	{
		rep_count = im->width - RLE_THRESHOLD;
		if (rep_count <= RLE_MAX_SIMPLE_LEN)
		{
			dst[0] = (CARD8) rep_count;
			dst[1] = (CARD8) value;
			dst[2] = 0 ;
			buf_used = 3;
		} else
		{
			dst[0] = (CARD8) ((rep_count >> 8) & RLE_LONG_D)|RLE_LONG_B;
			dst[1] = (CARD8) ((rep_count) & 0x00FF);
			dst[2] = (CARD8) value;
			dst[3] = 0 ;
			buf_used = 4;
		}
	}
	asimage_apply_buffer (im, color, y, dst, buf_used);
	release_compression_buffer( dst );
	return buf_used;
}

size_t
asimage_add_line (ASImage * im, ColorPart color, register CARD32 * data, unsigned int y)
{
	int    i = 0, bstart = 0, ccolor = 0;
	unsigned int    width;
	register CARD8 *dst;
	register int 	tail = 0;
	int best_size, best_bstart = 0, best_tail = 0;
	int buf_used = 0 ;

	if (AS_ASSERT(im) || AS_ASSERT(data) || color <0 || color >= IC_NUM_CHANNELS )
		return 0;
	if (y >= im->height)
		return 0;

	clear_flags( im->flags, ASIM_DATA_NOT_USEFUL );

	if( get_flags( im->flags, ASIM_STATIC ) ) 
	{
		CARD8 *part = im->channels[color][y];
		int i ;
		for( i = 0 ; i < im->width ; ++i ) 
			part[i] = data[i] ;
		return im->width ;
	}

	best_size = 0 ;
	if( im->width == 1 )
	{
		if( (dst = get_compression_buffer( 2 )) == NULL )
			return 0;
		dst[0] = RLE_DIRECT_TAIL ;
		dst[1] = data[0] ;
		buf_used = 2;
	}else
	{
		CARD8 *dst_start ;
		if( (dst_start = get_compression_buffer( im->width*2 )) == NULL )
			return 0;
		dst = dst_start ;
/*		width = im->width; */
		width = im->max_compressed_width ;
		while( i < width )
		{
			while( i < width && data[i] == data[ccolor])
			{
/*				fprintf( stderr, "%d<%2.2X ", i, data[i] ); */
				++i ;
			}
			if( i > ccolor + RLE_THRESHOLD )
			{ /* we have to write repetition count and length */
				register unsigned int rep_count = i - ccolor - RLE_THRESHOLD;

				if (rep_count <= RLE_MAX_SIMPLE_LEN)
				{
					dst[tail] = (CARD8) rep_count;
					dst[++tail] = (CARD8) data[ccolor];
/*					fprintf( stderr, "\n%d:%d: >%d: %2.2X %d: %2.2X ", y, color, tail-1, dst[tail-1], tail, dst[tail] ); */
				} else
				{
					dst[tail] = (CARD8) ((rep_count >> 8) & RLE_LONG_D)|RLE_LONG_B;
					dst[++tail] = (CARD8) ((rep_count) & 0x00FF);
					dst[++tail] = (CARD8) data[ccolor];
/*					fprintf( stderr, "\n%d:%d: >%d: %2.2X %d: %2.2X %d: %2.2X ", y, color, tail-2, dst[tail-2], tail-1, dst[tail-1], tail, dst[tail] );*/
				}
				++tail ;
				bstart = ccolor = i;
			}
/*			fprintf( stderr, "\n"); */
			while( i < width )
			{
/*				fprintf( stderr, "%d<%2.2X ", i, data[i] ); */
				if( data[i] != data[ccolor] )
					ccolor = i ;
				else if( i-ccolor > RLE_THRESHOLD )
					break;
				++i ;
			}
			if( i == width )
				ccolor = i ;
			while( ccolor > bstart )
			{/* we have to write direct block */
				unsigned int dist = ccolor-bstart ;

				if( tail-(int)bstart < best_size )
				{
					best_tail = tail ;
					best_bstart = bstart ;
					best_size = tail-bstart ;
				}
				if( dist > RLE_MAX_DIRECT_LEN )
					dist = RLE_MAX_DIRECT_LEN ;
				dst[tail] = RLE_DIRECT_B | ((CARD8)(dist-1));
/*				fprintf( stderr, "\n%d:%d: >%d: %2.2X ", y, color, tail, dst[tail] ); */
				dist += bstart ;
				++tail ;
				while ( bstart < dist )
				{
					dst[tail] = (CARD8) data[bstart];
/*					fprintf( stderr, "%d: %2.2X ", tail, dst[tail]); */
					++tail ;
					++bstart;
				}
			}
/*			fprintf( stderr, "\n"); */
		}
		if( best_size+(int)im->width < tail )
		{
			width = im->width;
/*			LOCAL_DEBUG_OUT( " %d:%d >resetting bytes starting with offset %d(%d) (0x%2.2X) to DIRECT_TAIL( %d bytes total )", y, color, best_tail, best_bstart, dst[best_tail], width-best_bstart ); */
			dst[best_tail] = RLE_DIRECT_TAIL;
			dst += best_tail+1;
			data += best_bstart;
			for( i = width-best_bstart-1 ; i >=0 ; --i )
				dst[i] = data[i] ;
			buf_used = best_tail+1+width-best_bstart ;
		}else
		{
			if( i < im->width )
			{
				dst[tail] = RLE_DIRECT_TAIL;
				dst += tail+1 ;
				data += i;
				buf_used = tail+1+im->width-i ;
				for( i = im->width-(i+1) ; i >=0 ; --i )
					dst[i] = data[i] ;
			}else
			{
				dst[tail] = RLE_EOL;
				buf_used = tail+1;
			}
		}
		dst = dst_start ;
	}
	asimage_apply_buffer (im, color, y, dst, buf_used);
	release_compression_buffer( dst );
	return buf_used;
}



unsigned int
asimage_print_line (ASImage * im, ColorPart color, unsigned int y, unsigned long verbosity)
{
	CARD8       **color_ptr;
	register CARD8 *ptr;
	int           to_skip = 0;
	int 		  uncopressed_size = 0 ;

	if (AS_ASSERT(im) || color < 0 || color >= IC_NUM_CHANNELS )
		return 0;
	if (y >= im->height)
		return 0;

	color_ptr = im->channels[color];
	if( AS_ASSERT(color_ptr) )
		return 0;
	ptr = color_ptr[y];
	
	if( get_flags( im->flags, ASIM_STATIC ) ) 
	{
		int i ;
		for( i = 0 ; i < im->width ; ++i ) 
			fprintf (stderr, " %2.2X", ptr[i]);
		return im->width ;
	}
	
	if( ptr == NULL )
	{
		if(  verbosity != 0 )
			show_error( "no data available for line %d", y );
		return 0;
	}
	while (*ptr != RLE_EOL)
	{
		while (to_skip-- > 0)
		{
			if (get_flags (verbosity, VRB_LINE_CONTENT))
				fprintf (stderr, " %2.2X", *ptr);
			ptr++;
		}

		if (get_flags (verbosity, VRB_CTRL_EXPLAIN))
			fprintf (stderr, "\nControl byte encountered : %2.2X", *ptr);

		if (((*ptr) & RLE_DIRECT_B) != 0)
		{
			if( *ptr == RLE_DIRECT_TAIL )
			{
				if (get_flags (verbosity, VRB_CTRL_EXPLAIN))
					fprintf (stderr, " is RLE_DIRECT_TAIL ( %d bytes ) !", im->width-uncopressed_size);
				if (get_flags (verbosity, VRB_LINE_CONTENT))
				{
					to_skip = im->width-uncopressed_size ;
					while (to_skip-- > 0)
					{
						fprintf (stderr, " %2.2X", *ptr);
						ptr++;
					}
				}else
					ptr += im->width-uncopressed_size ;
				break;
			}
			to_skip = 1 + ((*ptr) & (RLE_DIRECT_D));
			uncopressed_size += to_skip;
			if (get_flags (verbosity, VRB_CTRL_EXPLAIN))
				fprintf (stderr, " is RLE_DIRECT !");
		} else if (((*ptr) & RLE_SIMPLE_B_INV) == 0)
		{
			if( *ptr == RLE_EOL )
			{
				if (get_flags (verbosity, VRB_CTRL_EXPLAIN))
					fprintf (stderr, " is RLE_EOL !");
				break;
			}
			if (get_flags (verbosity, VRB_CTRL_EXPLAIN))
				fprintf (stderr, " is RLE_SIMPLE !");
			uncopressed_size += ((int)ptr[0])+ RLE_THRESHOLD;
			to_skip = 1;
		} else if (((*ptr) & RLE_LONG_B) != 0)
		{
			if (get_flags (verbosity, VRB_CTRL_EXPLAIN))
				fprintf (stderr, " is RLE_LONG !");
			uncopressed_size += ((int)ptr[1])+((((int)ptr[0])&RLE_LONG_D ) << 8)+RLE_THRESHOLD;
			to_skip = 1 + 1;
		}
		to_skip++;
		if (get_flags (verbosity, VRB_CTRL_EXPLAIN))
			fprintf (stderr, " to_skip = %d, uncompressed size = %d\n", to_skip, uncopressed_size);
	}
	if (get_flags (verbosity, VRB_LINE_CONTENT))
		fprintf (stderr, " %2.2X\n", *ptr);

	ptr++;

	if (get_flags (verbosity, VRB_LINE_SUMMARY))
		fprintf (stderr, "Row %d, Component %d, Memory Used %ld\n", y, color, (long)(ptr - color_ptr[y]));
	return ptr - color_ptr[y];
}

void print_asimage( ASImage *im, int flags, char * func, int line )
{
	if( im )
	{
		register unsigned int k ;
		int total_mem = 0 ;
		fprintf( stderr, "%s:%d> printing ASImage %p.\n", func, line, im);
		for( k = 0 ; k < im->height ; k++ )
    	{
 			fprintf( stderr, "%s:%d> ******* %d *******\n", func, line, k );
			total_mem+=asimage_print_line( im, IC_RED  , k, flags );
			total_mem+=asimage_print_line( im, IC_GREEN, k, flags );
			total_mem+=asimage_print_line( im, IC_BLUE , k, flags );
            total_mem+=asimage_print_line( im, IC_ALPHA , k, flags );
        }
    	fprintf( stderr, "%s:%d> Total memory : %u - image size %dx%d ratio %d%%\n", func, line, total_mem, im->width, im->height, (total_mem*100)/(im->width*im->height*3) );
	}else
		fprintf( stderr, "%s:%d> Attempted to print NULL ASImage.\n", func, line);
}



inline static CARD32*
asimage_decode_block32 (register CARD8 *src, CARD32 *to_buf, unsigned int width )
{
	register CARD32 *dst = to_buf;
	while ( *src != RLE_EOL)
	{
		if( src[0] == RLE_DIRECT_TAIL )
		{
			register int i = width - (dst-to_buf) ;
			dst += i ;
			src += i+1 ;
			i = -i ;
			while( i < 0 )
			{
				dst[i] = src[i] ;
				++i ;
			}
			break;
		}else if( ((*src)&RLE_DIRECT_B) != 0 )
		{
			register int i = (((int)src[0])&RLE_DIRECT_D) + 1;
			dst += i ;
			src += i+1 ;
			i = -i ;
			while( i < 0 )
			{
				dst[i] = src[i] ;
				++i ;
			}
		}else if( ((*src)&RLE_LONG_B) != 0 )
		{
			register int i = ((((int)src[0])&RLE_LONG_D)<<8|src[1]) + RLE_THRESHOLD;
			dst += i ;
			i = -i ;
			while( i < 0 )
			{
				dst[i] = src[2] ;
				++i ;
			}
			src += 3;
		}else
		{
			register int i = (int)src[0] + RLE_THRESHOLD ;
			dst += i ;
			i = -i;
			while( i < 0 )
			{
				dst[i] = src[1] ;
				++i ;
			}
			src += 2;
		}
	}
	return dst;
}

inline static CARD8*
asimage_decode_block8 (register CARD8 *src, CARD8 *to_buf, unsigned int width )
{
	register CARD8 *dst = to_buf;
	while ( *src != RLE_EOL)
	{
		if( src[0] == RLE_DIRECT_TAIL )
		{
			register int i = width - (dst-to_buf) ;
			dst += i ;
			src += i+1 ;
			i = -i ;
			while( i < 0 )
			{
				dst[i] = src[i] ;
				++i ;
			}
			break;
		}else if( ((*src)&RLE_DIRECT_B) != 0 )
		{
			register int i = (((int)src[0])&RLE_DIRECT_D) + 1;
			dst += i ;
			src += i+1 ;
			i = -i ;
			while( i < 0 )
			{
				dst[i] = src[i] ;
				++i ;
			}
		}else if( ((*src)&RLE_LONG_B) != 0 )
		{
			register int i = ((((int)src[0])&RLE_LONG_D)<<8|src[1]) + RLE_THRESHOLD;
			dst += i ;
			i = -i ;
			while( i < 0 )
			{
				dst[i] = src[2] ;
				++i ;
			}
			src += 3;
		}else
		{
			register int i = (int)src[0] + RLE_THRESHOLD ;
			dst += i ;
			i = -i;
			while( i < 0 )
			{
				dst[i] = src[1] ;
				++i ;
			}
			src += 2;
		}
	}
	return dst;
}

void print_component( register CARD32 *data, int nonsense, int len );

inline int
asimage_decode_line (ASImage * im, ColorPart color, CARD32 * to_buf, unsigned int y, unsigned int skip, unsigned int out_width)
{
	int max_i ;
	register CARD8  *src = im->channels[color][y];
	register int i = 0;
	/* that thing below is supposedly highly optimized : */
LOCAL_DEBUG_CALLER_OUT( "im->width = %d, color = %d, y = %d, skip = %d, out_width = %d, src = %p", im->width, color, y, skip, out_width, src );

	skip = skip%im->width ;
	max_i = MIN(out_width,im->width-skip);

	if( get_flags( im->flags, ASIM_STATIC ) ) 
	{
		int k = skip ; 
		i = skip ;
		while( k < out_width ) 
		{
			while( i < max_i ) 
				to_buf[k++] = src[i++] ;
			i = 0 ;
		}					
		return im->width ;
	}

	if( src )
	{
#if 1
  		if( skip > 0 || out_width+skip < im->width)
		{
			CARD8 *buffer = get_compression_buffer( im->width );

			asimage_decode_block8( src, buffer, im->width );
			src = buffer+skip ;
			while( i < (int)out_width )
			{
				while( i < max_i )
				{
					to_buf[i] = src[i] ;
					++i ;
				}
				src = buffer-i ;
				max_i = MIN(out_width,im->width+i) ;
			}
		}else
		{
#endif
			i = asimage_decode_block32( src, to_buf, im->width ) - to_buf;
            LOCAL_DEBUG_OUT( "decoded %d pixels", i );
#if defined(LOCAL_DEBUG) && !defined(NO_DEBUG_OUTPUT)
            {
                int z = -1 ;
                while( ++z < i )
                    fprintf( stderr, "%X ", src[z] );
                fprintf( stderr, "\n");

            }
#endif
#if 1
	  		while( i < (int)out_width )
			{   /* tiling code : */
				register CARD32 *src = to_buf-i ;
				int max_i = MIN(out_width,im->width+i);
				while( i < max_i )
				{
					to_buf[i] = src[i] ;
					++i ;
				}
			}
#endif
		}
		return i;
	}
	return 0;
}


inline CARD8*
asimage_copy_line (register CARD8 *src, int width)
{
	register int i = 0;

	/* merely copying the data */
	if ( src == NULL )
		return NULL;
	while (src[i] != RLE_EOL && width )
	{
		if ((src[i] & RLE_DIRECT_B) != 0)
		{
			if( src[i] == RLE_DIRECT_TAIL )
			{
				i += width+1 ;
				break;
			}else
			{
				register int to_skip = (src[i] & (RLE_DIRECT_D))+1;
				width -= to_skip ;
				i += to_skip ;
			}
		} else if ((src[i]&RLE_SIMPLE_B_INV) == 0)
		{
			width -= ((int)src[i])+ RLE_THRESHOLD;
			++i ;
		} else if ((src[i] & RLE_LONG_B) != 0)
		{
			register int to_skip = ((((int)src[i])&RLE_LONG_D ) << 8) ;
			width -= ((int)src[++i])+to_skip+RLE_THRESHOLD;
			++i;
		}
		++i;
	}
	if( i > 0 )
	{
		CARD8 *res = safemalloc( i+1 );
		memcpy( res, src, i+1 );
		return res;
	}else
		return NULL ;
}

unsigned int
asimage_threshold_line( CARD8 *src, unsigned int width, unsigned int *runs, unsigned int threshold )
{
	register int i = 0;
	int runs_count = 0 ;
	int start = -1, end = -1, curr_x = 0 ;

	/* merely copying the data */
	if ( src == NULL )
		return 0;
		
	runs[0] = runs[1] = 0 ;
	while (src[i] != RLE_EOL && curr_x < (int)width )
	{
		if ((src[i] & RLE_DIRECT_B) != 0)
		{
			int stop ;
			if( src[i] == RLE_DIRECT_TAIL )
				stop = i + 1 + (width-curr_x) ;
			else
				stop = i + 1 + (src[i] & (RLE_DIRECT_D))+1;
#ifdef DEBUG_RECTS2			
			fprintf( stderr, "%d: i = %d, stop = %d\n", __LINE__, i, stop ); 
#endif				
			while( ++i < stop  )
			{
				if( src[i] >= threshold )
				{
					if( start < 0 )
						start = curr_x ;
					if( end < 0 )
						end = curr_x ;
					else
						++end ;
				}else
				{
					if( start >= 0 && end >= start )
					{
						runs[runs_count] = start ;
						++runs_count;
						runs[runs_count] = end ;
						++runs_count ;
					}
					start = end = -1 ;
				}
				++curr_x ;
#ifdef DEBUG_RECTS2			     				
				fprintf( stderr, "%d: src[%d] = %d, curr_x = %d, start = %d, end = %d, runs_count = %d\n",
  					     __LINE__, i, (int)src[i], curr_x, start, end, runs_count );
#endif
  
			}
		} else
		{
			int len = 0 ;

			if ((src[i]&RLE_SIMPLE_B_INV) == 0)
			{
#ifdef DEBUG_RECTS2			   
			fprintf( stderr, "%d: SimpleRLE: i = %d, src[i] = %d(%2.2X)\n", __LINE__, i, src[i], src[i] ); 
#endif
				len = ((int)src[i])+ RLE_THRESHOLD;
				++i ;
			} else if ((src[i] & RLE_LONG_B) != 0)
			{
#ifdef DEBUG_RECTS2			   
			fprintf( stderr, "%d: LongRLE: i = %d, src[i] = %d(%2.2X), src[i] = %d(%2.2X)\n", __LINE__, i, src[i], src[i], src[i+1], src[i+1] ); 
#endif
				len = ((((int)src[i])&RLE_LONG_D ) << 8) ;
				++i;
				len += ((int)src[i])+RLE_THRESHOLD;
				++i;
			}
#ifdef DEBUG_RECTS2			   
			fprintf( stderr, "%d: i = %d, len = %d, src[i] = %d\n", __LINE__, i, len, src[i] ); 
#endif

			if( src[i] >= threshold )
			{
				if( start < 0 )
					start = curr_x ;
				if( end < 0 )
					end = curr_x-1 ;
				end += len ;
			}else
			{
				if( start >= 0 && end >= start )
				{
					runs[runs_count] = start ;
					++runs_count;
					runs[runs_count] = end ;
					++runs_count ;
				}
				start = end = -1 ;
			}
			curr_x += len ;
#ifdef DEBUG_RECTS2			   
			fprintf( stderr, "%d: src[%d] = %d, curr_x = %d, start = %d, end = %d, runs_count = %d\n",
 					    __LINE__, i, (int)src[i], curr_x, start, end, runs_count );
#endif
			++i;
		}
	}
	if( start >= 0 && end >= start )
	{
		runs[runs_count] = start ;
		++runs_count;
		runs[runs_count] = end ;
		++runs_count ;
	}

#ifdef DEBUG_RECTS2			   
	for( i = 0 ; i < runs_count ; ++i, ++i )
		fprintf( stderr, "\trun[%d] = %d ... %d\n", i, runs[i], runs[i+1] );
#endif

	return runs_count;
}

void
move_asimage_channel( ASImage *dst, int channel_dst, ASImage *src, int channel_src )
{
	if( get_flags( dst->flags, ASIM_STATIC ) || get_flags( src->flags, ASIM_STATIC ) ) 
		return;

	if( !AS_ASSERT(dst) && !AS_ASSERT(src) && channel_src >= 0 && channel_src < IC_NUM_CHANNELS &&
		channel_dst >= 0 && channel_dst < IC_NUM_CHANNELS )
	{
		if( dst->width == src->width )
		{
			register int i = MIN(dst->height, src->height);
			register CARD8 **dst_rows = dst->channels[channel_dst] ;
			register CARD8 **src_rows = src->channels[channel_src] ;
			while( --i >= 0 )
			{
				if( dst_rows[i] )
					free( dst_rows[i] );
				dst_rows[i] = src_rows[i] ;
				src_rows[i] = NULL ;
			}
		}else
			show_debug( __FILE__,"move_asimage_channel",__LINE__, "images size differ : %d and %d", src->width, dst->width );
	}
}


void
copy_asimage_channel( ASImage *dst, int channel_dst, ASImage *src, int channel_src )
{
	if( get_flags( dst->flags, ASIM_STATIC ) || get_flags( src->flags, ASIM_STATIC ) ) 
		return;

	if( !AS_ASSERT(dst) && !AS_ASSERT(src) && channel_src >= 0 && channel_src < IC_NUM_CHANNELS &&
		channel_dst >= 0 && channel_dst < IC_NUM_CHANNELS )
	{
		register int i = MIN(dst->height, src->height);
		LOCAL_DEBUG_OUT( "src = %p, dst = %p, dst->width = %d, src->width = %d", src, dst, dst->width, src->width );
		if( dst->width == src->width )
		{
			register CARD8 **dst_rows = dst->channels[channel_dst] ;
			register CARD8 **src_rows = src->channels[channel_src] ;
			while( --i >= 0 )
			{
				if( dst_rows[i] )
					free( dst_rows[i] );
				dst_rows[i] = asimage_copy_line( src_rows[i], dst->width );
			}
		}else
		{
			CARD32 *buffer = safemalloc( dst->width * sizeof(CARD32));			
			while( --i >= 0 )
			{
				int decoded = asimage_decode_line (src, channel_src, buffer, i, 0, dst->width );
				LOCAL_DEBUG_OUT( "decoded = %d", decoded );
				if( decoded == dst->width ) 
				asimage_add_line (dst, channel_dst, buffer, i) ;
			}		
		}	 
	}
}

void
copy_asimage_lines( ASImage *dst, unsigned int offset_dst,
                    ASImage *src, unsigned int offset_src,
					unsigned int nlines, ASFlagType filter )
{
	if( get_flags( dst->flags, ASIM_STATIC ) || get_flags( src->flags, ASIM_STATIC ) ) 
		return;

	if( !AS_ASSERT(dst) && !AS_ASSERT(src) &&
		offset_src < src->height && offset_dst < dst->height &&
		dst->width == src->width )
	{
		int chan;

		if( offset_src+nlines > src->height )
			nlines = src->height - offset_src ;
		if( offset_dst+nlines > dst->height )
			nlines = dst->height - offset_dst ;

		for( chan = 0 ; chan < IC_NUM_CHANNELS ; ++chan )
			if( get_flags( filter, 0x01<<chan ) )
			{
				register int i = -1;
				register CARD8 **dst_rows = &(dst->channels[chan][offset_dst]) ;
				register CARD8 **src_rows = &(src->channels[chan][offset_src]) ;
LOCAL_DEBUG_OUT( "copying %d lines of channel %d...", nlines, chan );
				while( ++i < (int)nlines )
				{
					if( dst_rows[i] )
						free( dst_rows[i] );
					dst_rows[i] = asimage_copy_line( src_rows[i], dst->width );
				}
			}
#if 0
		for( i = 0 ; i < nlines ; ++i )
		{
			asimage_print_line( src, IC_ALPHA, i, (i==4)?VRB_EVERYTHING:VRB_LINE_SUMMARY );
			asimage_print_line( dst, IC_ALPHA, i, (i==4)?VRB_EVERYTHING:VRB_LINE_SUMMARY );
		}
#endif
	}
}

Bool
asimage_compare_line (ASImage *im, ColorPart color, CARD32 *to_buf, CARD32 *tmp, unsigned int y, Bool verbose)
{
	register unsigned int i;
	if( get_flags( im->flags, ASIM_STATIC ) ) 
	{
		CARD8 *src = im->channels[color][y];
		for( i = 0 ; i < im->width ; i++ )
			if( src[i] != to_buf[i] )
			{
				if( verbose )
					show_error( "line %d, component %d differ at offset %d ( 0x%lX(compresed) != 0x%lX(orig) )\n", y, color, i, 
					src[i], to_buf[i] );
				return False ;
			}
	}else
	{
		asimage_decode_line( im, color, tmp, y, 0, im->width );
		for( i = 0 ; i < im->width ; i++ )
			if( tmp[i] != to_buf[i] )
			{
				if( verbose )
					show_error( "line %d, component %d differ at offset %d ( 0x%lX(compresed) != 0x%lX(orig) )\n", y, color, i, tmp[i], to_buf[i] );
				return False ;
			}
	}
	return True;
}

ASFlagType
get_asimage_chanmask( ASImage *im)
{
    ASFlagType mask = 0 ;
	int color ;

	if( !AS_ASSERT(im) )
		for( color = 0; color < IC_NUM_CHANNELS ; color++ )
		{
			register CARD8 **chan = im->channels[color];
			register int y, height = im->height ;
			for( y = 0 ; y < height ; y++ )
				if( chan[y] )
				{
					set_flags( mask, 0x01<<color );
					break;
				}
		}
    return mask ;
}

int
check_asimage_alpha (ASVisual *asv, ASImage *im )
{
	int recomended_depth = 0 ;
	unsigned int            i;
	ASScanline     buf;

	if( asv == NULL )
		asv = get_default_asvisual();

	if (im == NULL)
		return 0;

	prepare_scanline( im->width, 0, &buf, asv->BGR_mode );
	buf.flags = SCL_DO_ALPHA ;
	for (i = 0; i < im->height; i++)
	{
		int count = asimage_decode_line (im, IC_ALPHA, buf.alpha, i, 0, buf.width);
		if( count < (int)buf.width )
		{
			if( ARGB32_ALPHA8(im->back_color) == 0 )
			{
				if( recomended_depth == 0 )
					recomended_depth = 1 ;
			}else if( ARGB32_ALPHA8(im->back_color) != 0xFF )
			{
				recomended_depth = 8 ;
				break ;
			}
		}
		while( --count >= 0 )
			if( buf.alpha[count] == 0  )
			{
				if( recomended_depth == 0 )
					recomended_depth = 1 ;
			}else if( (buf.alpha[count]&0xFF) != 0xFF  )
			{
				recomended_depth = 8 ;
				break ;
			}
		if( recomended_depth == 8 )
			break;
	}
	free_scanline(&buf, True);

	return recomended_depth;
}



/* ********************************************************************************/
/* Vector -> ASImage functions :                                                  */
/* ********************************************************************************/
Bool
set_asimage_vector( ASImage *im, register double *vector )
{
	if( vector == NULL || im == NULL )
		return False;

	if( im->alt.vector == NULL )
		im->alt.vector = safemalloc( im->width*im->height*sizeof(double));

	{
		register double *dst = im->alt.vector ;
		register int i = im->width*im->height;
		while( --i >= 0 )
			dst[i] = vector[i] ;
	}

	return True;
}

/* ********************************************************************************/
/* Convinience function - very fast image cloning :                               */
/* ********************************************************************************/
ASImage*
clone_asimage( ASImage *src, ASFlagType filter )
{
	ASImage *dst = NULL ;
	START_TIME(started);

	if( !AS_ASSERT(src) )
	{
		int chan ;
		dst = create_asimage(src->width, src->height, (src->max_compressed_width*100)/src->width);
		if( get_flags( src->flags, ASIM_DATA_NOT_USEFUL ) )
			set_flags( dst->flags, ASIM_DATA_NOT_USEFUL );
		dst->back_color = src->back_color ;
		for( chan = 0 ; chan < IC_NUM_CHANNELS;  chan++ )
			if( get_flags( filter, 0x01<<chan) )
			{
				register int i = dst->height;
				register CARD8 **dst_rows = dst->channels[chan] ;
				register CARD8 **src_rows = src->channels[chan] ;
				while( --i >= 0 )
					dst_rows[i] = asimage_copy_line( src_rows[i], dst->width );
			}
	}
	SHOW_TIME("", started);
	return dst;
}

/* ********************************************************************************/
/* Convinience function
 * 		- generate rectangles list for channel values exceeding threshold:        */
/* ********************************************************************************/
XRectangle*
get_asimage_channel_rects( ASImage *src, int channel, unsigned int threshold, unsigned int *rects_count_ret )
{
	XRectangle *rects = NULL ;
	int rects_count = 0, rects_allocated = 0 ;

	START_TIME(started);

	if( !AS_ASSERT(src) && channel < IC_NUM_CHANNELS )
	{
		int i = src->height;
		CARD8 **src_rows = src->channels[channel] ;
		unsigned int *height = safemalloc( (src->width+1)*2 * sizeof(unsigned int) );
		unsigned int *prev_runs = NULL ;
		int prev_runs_count = 0 ;
		unsigned int *runs = safemalloc( (src->width+1)*2 * sizeof(unsigned int) );
		unsigned int *tmp_runs = safemalloc( (src->width+1)*2 * sizeof(unsigned int) );
		unsigned int *tmp_height = safemalloc( (src->width+1)*2 * sizeof(unsigned int) );
		Bool count_empty = (ARGB32_CHAN8(src->back_color,channel)>= threshold);

#ifdef DEBUG_RECTS
		fprintf( stderr, "%d:back_color = #%8.8lX,  count_empty = %d, thershold = %d\n", __LINE__, src->back_color, count_empty, threshold );
#endif
		while( --i >= -1 )
		{
			int runs_count = 0 ;
#ifdef DEBUG_RECTS
			fprintf( stderr, "%d: LINE %d **********************\n", __LINE__, i );
#ifdef DEBUG_RECTS2
			asimage_print_line (src, channel, i, 0xFFFFFFFF);
#else
			asimage_print_line (src, channel, i, VRB_LINE_CONTENT);
#endif
#endif
			if( i >= 0 )
			{
				if( src_rows[i] )
				{
					if( get_flags( src->flags, ASIM_STATIC ) ) 
					{
						/* TODO : ! */
						runs_count = 2 ;
						runs[0] = 0 ;
						runs[1] = src->width ;
					}else
						runs_count = asimage_threshold_line( src_rows[i], src->width, runs, threshold );
				}else if( count_empty )
				{
					runs_count = 2 ;
					runs[0] = 0 ;
					runs[1] = src->width ;
				}
			}

			if( runs_count > 0 && (runs_count &0x0001) != 0 )
			{                                  /* allways wants to have even number of runs */
				runs[runs_count] = 0 ;
				++runs_count ;
			}

			if( prev_runs_count > 0 )
			{ /* here we need to merge runs and add all the detached rectangles to the rects list */
				int k = 0, l = 0, last_k = 0 ;
				int tmp_count = 0 ;
				unsigned int *tmp ;
				if( runs_count == 0 )
				{
					runs[0] = src->width ;
					runs[1] = src->width ;
					runs_count = 2 ;
				}
				tmp_runs[0] = 0 ;
				tmp_runs[1] = src->width ;
				/* two passes : in first pass we go through old runs and try and see if they are continued
				 * in this line. If not - we add them to the list of rectangles. At
				 * the same time we subtract them from new line's runs : */
				for( l = 0 ; l < prev_runs_count ; ++l, ++l )
				{
					int start = prev_runs[l], end = prev_runs[l+1] ;
					int matching_runs = 0 ;
#ifdef DEBUG_RECTS
					fprintf( stderr, "%d: prev run %d : start = %d, end = %d, last_k = %d, height = %d\n", __LINE__, l, start, end, last_k, height[l] );
#endif
					for( k = last_k ; k < runs_count ; ++k, ++k )
					{
#ifdef DEBUG_RECTS
						fprintf( stderr, "*%d: new run %d : start = %d, end = %d\n", __LINE__, k, runs[k], runs[k+1] );
#endif
						if( (int)runs[k] > end )
						{	/* add entire run to rectangles list */
							if( rects_count >= rects_allocated )
							{
								rects_allocated = rects_count + 8 + (rects_count>>3);
								rects = realloc( rects, rects_allocated*sizeof(XRectangle));
							}
							rects[rects_count].x = start ;
							rects[rects_count].y = i+1 ;
							rects[rects_count].width = (end-start)+1 ;
							rects[rects_count].height = height[l] ;
#ifdef DEBUG_RECTS
							fprintf( stderr, "*%d: added rectangle at y = %d\n", __LINE__, rects[rects_count].y );
#endif
							++rects_count ;
							++matching_runs;
							break;
						}else if( (int)runs[k+1] >= start  )
						{
							if( start < (int)runs[k] )
							{	/* add rectangle start, , runs[k]-start, height[l] */
								if( rects_count >= rects_allocated )
								{
									rects_allocated = rects_count + 8 + (rects_count>>3);
									rects = realloc( rects, rects_allocated*sizeof(XRectangle));
								}
								rects[rects_count].x = start ;
								rects[rects_count].y = i+1 ;
								rects[rects_count].width = runs[k]-start ;
								rects[rects_count].height = height[l] ;
#ifdef DEBUG_RECTS
								fprintf( stderr, "*%d: added rectangle at y = %d\n", __LINE__, rects[rects_count].y );
#endif
								++rects_count ;
								start = runs[k] ;
							}else if( start > (int)runs[k] )
							{
								tmp_runs[tmp_count] = runs[k] ;
								tmp_runs[tmp_count+1] = start-1 ;
								tmp_height[tmp_count] = 1 ;
#ifdef DEBUG_RECTS
								fprintf( stderr, "*%d: tmp_run %d added : %d ... %d, height = %d\n", __LINE__, tmp_count, runs[k], start-1, 1 );
#endif
								++tmp_count ; ++tmp_count ;
								runs[k] = start ;
							}
							/* at that point both runs start at the same point */
							if( end < (int)runs[k+1] )
							{
								runs[k] = end+1 ;
							}else 
							{   
								if( end > (int)runs[k+1] )
								{	
									/* add rectangle runs[k+1]+1, , end - runs[k+1], height[l] */
									if( rects_count >= rects_allocated )
									{
										rects_allocated = rects_count + 8 + (rects_count>>3);
										rects = realloc( rects, rects_allocated*sizeof(XRectangle));
									}
									rects[rects_count].x = runs[k+1]+1 ;
									rects[rects_count].y = i+1 ;
									rects[rects_count].width = end - runs[k+1] ;
									rects[rects_count].height = height[l] ;
#ifdef DEBUG_RECTS
									fprintf( stderr, "*%d: added rectangle at y = %d\n", __LINE__, rects[rects_count].y );
#endif
									++rects_count ;
									end = runs[k+1] ;
								
								} 
								/* eliminating new run - it was all used up :) */
								runs[k] = src->width ;
								runs[k+1] = src->width ;
#ifdef DEBUG_RECTS
								fprintf( stderr, "*%d: eliminating new run %d\n", __LINE__, k );
#endif
								++k ; ++k ;
							}
							tmp_runs[tmp_count] = start ;
							tmp_runs[tmp_count+1] = end ;
							tmp_height[tmp_count] = height[l]+1 ;
#ifdef DEBUG_RECTS
							fprintf( stderr, "*%d: tmp_run %d added : %d ... %d, height = %d\n", __LINE__, tmp_count, start, end, height[l]+1 );
#endif
							++tmp_count ; ++tmp_count ;
							last_k = k ;
							++matching_runs;
							break;
						}
					}
					if( matching_runs == 0 ) 
					{  /* no new runs for this prev run - add rectangle */
#ifdef DEBUG_RECTS
						fprintf( stderr, "%d: NO MATCHING NEW RUNS : start = %d, end = %d, height = %d\n", __LINE__, start, end, height[l] );
#endif
						if( rects_count >= rects_allocated )
						{
							rects_allocated = rects_count + 8 + (rects_count>>3);
							rects = realloc( rects, rects_allocated*sizeof(XRectangle));
						}
						rects[rects_count].x = start ;
						rects[rects_count].y = i+1 ;
						rects[rects_count].width = (end-start)+1 ;
						rects[rects_count].height = height[l] ;
#ifdef DEBUG_RECTS
						fprintf( stderr, "*%d: added rectangle at y = %d\n", __LINE__, rects[rects_count].y );
#endif
						++rects_count ;
					}	 
				}
				/* second pass: we need to pick up remaining new runs */
				/* I think these should be inserted in oredrly manner so that we have runs list arranged in ascending order */
				for( k = 0 ; k < runs_count ; ++k, ++k )
					if( runs[k] < src->width )
					{
						int ii = tmp_count ; 
						while( ii > 0 && tmp_runs[ii-1] > runs[k] )
						{
							tmp_runs[ii] = tmp_runs[ii-2] ;
							tmp_runs[ii+1] = tmp_runs[ii-1] ;
							tmp_height[ii] = tmp_height[ii-2] ;
							--ii ; --ii ;
						}
						tmp_runs[ii] = runs[k] ;
						tmp_runs[ii+1] = runs[k+1] ;
						tmp_height[ii] = 1 ;
#ifdef DEBUG_RECTS
						fprintf( stderr, "*%d: tmp_run %d added : %d ... %d, height = %d\n", __LINE__, ii, runs[k], runs[k+1], 1 );
#endif
						++tmp_count, ++tmp_count;
					}
				tmp = prev_runs ;
				prev_runs = tmp_runs ;
				tmp_runs = tmp ;
				tmp = height ;
				height = tmp_height ;
				tmp_height = tmp ;
				prev_runs_count = tmp_count ;
			}else if( runs_count > 0 )
			{
				int k = runs_count;
				prev_runs_count = runs_count ;
				prev_runs = runs ;
				runs = safemalloc( (src->width+1)*2 * sizeof(unsigned int) );
				while( --k >= 0 )
					height[k] = 1 ;
			}
		}
		free( runs );
		if( prev_runs )
			free( prev_runs );
		free( tmp_runs );
		free( tmp_height );
		free( height );
	}
	SHOW_TIME("", started);

	if( rects_count_ret )
		*rects_count_ret = rects_count ;

	return rects;
}

/***********************************************************************************/
void
raw2scanline( register CARD8 *row, ASScanline *buf, CARD8 *gamma_table, unsigned int width, Bool grayscale, Bool do_alpha )
{
	register int x = width;

	if( grayscale )
		row += do_alpha? width<<1 : width ;
	else
		row += width*(do_alpha?4:3) ;

	if( gamma_table )
	{
		if( !grayscale )
		{
			while ( --x >= 0 )
			{
				row -= 3 ;
				if( do_alpha )
				{
					--row;
					buf->alpha[x] = row[3];
				}
				buf->xc1[x]  = gamma_table[row[0]];
				buf->xc2[x]= gamma_table[row[1]];
				buf->xc3[x] = gamma_table[row[2]];
			}
		}else /* greyscale */
			while ( --x >= 0 )
			{
				if( do_alpha )
					buf->alpha[x] = *(--row);
				buf->xc1 [x] = buf->xc2[x] = buf->xc3[x]  = gamma_table[*(--row)];
			}
	}else
	{
		if( !grayscale )
		{
			while ( --x >= 0 )
			{
				row -= 3 ;
				if( do_alpha )
				{
					--row;
					buf->alpha[x] = row[3];
				}
				buf->xc1[x]  = row[0];
				buf->xc2[x]= row[1];
				buf->xc3[x] = row[2];
			}
		}else /* greyscale */
			while ( --x >= 0 )
			{
				if( do_alpha )
					buf->alpha[x] = *(--row);
				buf->xc1 [x] = buf->xc2[x] = buf->xc3[x]  = *(--row);
			}
	}
}

/* ********************************************************************************/
/* The end !!!! 																 */
/* ********************************************************************************/

