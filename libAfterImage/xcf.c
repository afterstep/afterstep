/* This file contains code for unified image loading from XCF file  */
/********************************************************************/
/* Copyright (c) 2001 Sasha Vasko <sashav@sprintmail.com>           */
/********************************************************************/
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "../configure.h"

#define LOCAL_DEBUG
#define DO_CLOCKING

#include <time.h>

#include "../include/aftersteplib.h"
#include "../include/afterstep.h"
#include "../include/screen.h"
#include "../include/asimage.h"
#include "../include/ashash.h"
#include "../include/asimage.h"
#include "../include/parse.h"
#include "../include/xcf.h"

static XcfProperty *read_xcf_props( FILE *fp );
static XcfListElem *read_xcf_list_offsets( FILE *fp, size_t elem_size );
static void 		read_xcf_layers( XcfImage *xcf_im, FILE *fp, XcfLayer *head );
static void			read_xcf_channels( XcfImage *xcf_im, FILE *fp, XcfChannel *head );
static XcfLayerMask*read_xcf_mask( XcfImage *xcf_im, FILE *fp );
static XcfHierarchy*read_xcf_hierarchy( XcfImage *xcf_im, FILE *fp );
static void 		read_xcf_levels( XcfImage *xcf_im, FILE *fp, XcfLevel *head );
static void 		read_xcf_tiles( XcfImage *xcf_im, FILE *fp, XcfTile *head );
static void 		read_xcf_tiles_rle( XcfImage *xcf_im, FILE *fp, XcfTile *head );

static size_t
xcf_read8 (FILE *fp, CARD8 *data, int count)
{
  	size_t total = count;

  	while (count > 0)
    {
	  	int bytes = fread ((char*) data, sizeof (char), count, fp);
      	if( bytes <= 0 )
			break;
      	count -= bytes;
      	data += bytes;
    }
	return total;
}

static size_t
xcf_read32 (FILE *fp, CARD32 *data, int count)
{
  	size_t total = count;
	if( count > 0 )
	{
		CARD8 *raw = (CARD8*)data ;
		total = xcf_read8( fp, raw, count<<2 )>>2;
		count = 0 ;
#ifndef WORDS_BIGENDIAN
		while( count < total )
		{
			data[count] = (raw[0]<<24)|(raw[1]<<16)|(raw[2]<<8)|raw[3];
			++count ;
			raw += 4 ;
		}
#endif
	}
	return total;
}

static void
xcf_skip_string (FILE *fp)
{
	CARD32 size = 0;
	if( xcf_read32 (fp, &size, 1)< 1 )
		return;
	if( size > 0 )
	{
		fseek(fp, size, SEEK_CUR );
	}
}

XcfImage *
read_xcf_image( FILE *fp )
{
	XcfImage *xcf_im = NULL ;
	XcfProperty *prop ;

	if( fp )
	{
		char sig[XCF_SIGNATURE_FULL_LEN+1] ;
		if( xcf_read8( fp, &(sig[0]), XCF_SIGNATURE_FULL_LEN ) >= XCF_SIGNATURE_FULL_LEN )
		{
			if( mystrncasecmp( sig, XCF_SIGNATURE, XCF_SIGNATURE_LEN) == 0 )
			{
				xcf_im = safecalloc( 1, sizeof(XcfImage));
				if( mystrncasecmp( &(sig[XCF_SIGNATURE_LEN+1]), "file", 4 ) == 0 )
					xcf_im->version = 0 ;
				else
					xcf_im->version = atoi(&(sig[XCF_SIGNATURE_LEN+1]));
				if( xcf_read32( fp, &(xcf_im->width), 3 ) < 3 )
				{
					free( xcf_im );
					xcf_im = NULL ;
				}
			}
		}
		if( xcf_im == NULL )
		{
			show_error( "invalid .xcf file format - not enough data to read" );
			return NULL ;
		}

		xcf_im->properties = read_xcf_props( fp );
		for( prop = xcf_im->properties ; prop != NULL ; prop = prop->next )
		{
			if( prop->id == XCF_PROP_COLORMAP )
			{
				register int i ;
				CARD32 n = *((CARD32*)(prop->data)) ;
				n = as_ntohl(n);
				xcf_im->num_cols = n ;
				n *= 3 ;
				xcf_im->colormap = safemalloc( n );
				if( xcf_im->version == 0 )
				{
					for( i = 0 ; i < n ; i++ )
						xcf_im->colormap[i] = i ;
				}else
					memcpy( xcf_im->colormap, prop->data+4, MIN(prop->len-4,n));
			}else if( prop->id == XCF_PROP_COMPRESSION )
				xcf_im->compression = *(prop->data);
		}
		xcf_im->layers = 	(XcfLayer*)  read_xcf_list_offsets( fp, sizeof(XcfLayer)  );
		xcf_im->channels = 	(XcfChannel*)read_xcf_list_offsets( fp, sizeof(XcfChannel));
		if( xcf_im->layers )
			read_xcf_layers( xcf_im, fp, xcf_im->layers );
		if( xcf_im->channels )
			read_xcf_channels( xcf_im, fp, xcf_im->channels );
	}
	return xcf_im;
}

/*******************************************************************************/
/* printing functions :     												   */
void
print_xcf_properties( char* prompt, XcfProperty *prop )
{
	register int i = 0 ;
	while( prop )
	{
		fprintf( stderr, "%s.properties[%d].id = %ld\n", prompt, i, prop->id );
		fprintf( stderr, "%s.properties[%d].size = %ld\n", prompt, i, prop->len );
		if( prop->len > 0 )
		{
			register int k ;
			fprintf( stderr, "%s.properties[%d].data = ", prompt, i );
			for( k = 0 ; k < prop->len ; k++ )
				fprintf( stderr, "%2.2X ", prop->data[k] );
			fprintf( stderr, "\n" );
		}
		prop = prop->next ;
		++i ;
	}
}

void
print_xcf_hierarchy( char* prompt, XcfHierarchy *h )
{
	if( h )
	{
		XcfLevel *level = h->levels ;
		int i = 0 ;

		fprintf( stderr, "%s.hierarchy.width = %ld\n", prompt, h->width );
		fprintf( stderr, "%s.hierarchy.height = %ld\n", prompt, h->height );
		fprintf( stderr, "%s.hierarchy.bpp = %ld\n", prompt, h->bpp );
		while( level )
		{
			XcfTile *tile = level->tiles ;
			int k = 0 ;
			fprintf( stderr, "%s.hierarchy.level[%d].offset = %ld\n", prompt, i, level->offset );
			fprintf( stderr, "%s.hierarchy.level[%d].width = %ld\n", prompt, i, level->width );
			fprintf( stderr, "%s.hierarchy.level[%d].height = %ld\n", prompt, i, level->height );
			while ( tile )
			{
				fprintf( stderr, "%s.hierarchy.level[%d].tile[%d].offset = %ld\n", prompt, i, k, tile->offset );
				fprintf( stderr, "%s.hierarchy.level[%d].tile[%d].estimated_size = %ld\n", prompt, i, k, tile->estimated_size );
				tile = tile->next ;
				++k ;
			}
			level = level->next ;
			++i ;
		}
	}
}

void
print_xcf_mask( char* prompt, XcfLayerMask *mask )
{
	char p[256] ;
	if( mask )
	{
		fprintf( stderr, "%s.mask.width = %ld\n", prompt, mask->width );
		fprintf( stderr, "%s.mask.height = %ld\n", prompt, mask->height );
		sprintf( p, "%s.mask", prompt );
		print_xcf_properties( p, mask->properties );
		fprintf( stderr, "%s.mask.opacity = %ld\n", prompt, mask->opacity );
		fprintf( stderr, "%s.mask.visible = %d\n", prompt, mask->visible );
		fprintf( stderr, "%s.mask.color = #%lX\n", prompt, mask->color );
		fprintf( stderr, "%s.mask.hierarchy_offset = %ld\n", prompt, mask->hierarchy_offset );
		print_xcf_hierarchy( p, mask->hierarchy );
	}
}


void
print_xcf_layers( char* prompt, XcfLayer *head )
{
	register int i = 0 ;
	char p[256] ;
	while( head )
	{
		fprintf( stderr, "%s.layer[%d].offset = %ld\n", prompt, i, head->offset );
		fprintf( stderr, "%s.layer[%d].width = %ld\n", prompt, i, head->width );
		fprintf( stderr, "%s.layer[%d].height = %ld\n", prompt, i, head->height );
		fprintf( stderr, "%s.layer[%d].type = %ld\n", prompt, i, head->type );
		sprintf( p, "%s.layer[%d]", prompt, i );
		print_xcf_properties( p, head->properties );
		fprintf( stderr, "%s.layer[%d].opacity = %ld\n", prompt, i, head->opacity );
		fprintf( stderr, "%s.layer[%d].visible = %d\n", prompt, i, head->visible );
		fprintf( stderr, "%s.layer[%d].preserve_transparency = %d\n", prompt, i,  head->preserve_transparency );
		fprintf( stderr, "%s.layer[%d].mode = %ld\n"    , prompt, i, head->mode 				   );
		fprintf( stderr, "%s.layer[%d].offset_x = %ld\n", prompt, i, head->offset_x 			   );
		fprintf( stderr, "%s.layer[%d].offset_y = %ld\n", prompt, i, head->offset_y 			   );

		fprintf( stderr, "%s.layer[%d].hierarchy_offset = %ld\n", prompt, i, head->hierarchy_offset );
		print_xcf_hierarchy( p, head->hierarchy );
		fprintf( stderr, "%s.layer[%d].mask_offset = %ld\n", prompt, i, head->mask_offset );
		print_xcf_mask( p, head->mask );

		head = head->next ;
		++i ;
	}
}

void
print_xcf_channels( char* prompt, XcfChannel *head )
{
	register int i = 0 ;
	char p[256] ;
	while( head )
	{
		fprintf( stderr, "%s.channel[%d].offset = %ld\n", prompt, i, head->offset );
		fprintf( stderr, "%s.channel[%d].width = %ld\n", prompt, i, head->width );
		fprintf( stderr, "%s.channel[%d].height = %ld\n", prompt, i, head->height );
		sprintf( p, "%s.channel[%d]", prompt, i );
		print_xcf_properties( p, head->properties );
		fprintf( stderr, "%s.channel[%d].opacity = %ld\n", prompt, i, head->opacity );
		fprintf( stderr, "%s.channel[%d].visible = %d\n", prompt, i, head->visible );
		fprintf( stderr, "%s.channel[%d].color = #%lX\n", prompt, i, head->color );
		fprintf( stderr, "%s.channel[%d].hierarchy_offset = %ld\n", prompt, i, head->hierarchy_offset );
		print_xcf_hierarchy( p, head->hierarchy );

		head = head->next ;
		++i ;
	}
}

void
print_xcf_image( XcfImage *xcf_im )
{
	if( xcf_im )
	{
		fprintf( stderr, "XcfImage.version = %d\n", xcf_im->version );
		fprintf( stderr, "XcfImage.width = %ld\nXcfImage.height = %ld\nXcfImage.type = %ld\n",
		   				  xcf_im->width, xcf_im->height, xcf_im->type );
		fprintf( stderr, "XcfImage.num_cols = %ld\n", xcf_im->num_cols );
		fprintf( stderr, "XcfImage.compression = %d\n", xcf_im->compression );
		print_xcf_properties( "XcfImage", xcf_im->properties );
		print_xcf_layers( "XcfImage", xcf_im->layers );
		print_xcf_channels( "XcfImage", xcf_im->channels );
	}
}

/*******************************************************************************/
/* deallocation functions : 												   */

void
free_xcf_properties( XcfProperty *head )
{
	while( head )
	{
		XcfProperty *next = head->next ;
		if( head->len > 0  && head->data && head->data != (CARD8*)&(head->buffer[0]))
			free( head->data );
		free( head );
		head = next ;
	}
}

void
free_xcf_hierarchy( XcfHierarchy *hierarchy )
{
	if( hierarchy )
	{
		register XcfLevel *level = hierarchy->levels;
		while( level )
		{
			XcfLevel *next = level->next ;
			while( level->tiles )
			{
				XcfTile *next = level->tiles->next ;
				if( level->tiles->data )
					free( level->tiles->data );
				free( level->tiles );
				level->tiles = next ;
			}
			free( level );
			level = next ;
		}
		free( hierarchy );
	}
}

void
free_xcf_channels( XcfChannel *head )
{
	while( head )
	{
		XcfChannel *next = head->next ;
		if( head->properties )
			free_xcf_properties( head->properties );
		if( head->hierarchy )
			free_xcf_hierarchy( head->hierarchy );
		free( head );
		head = next ;
	}
}

void
free_xcf_layers( XcfLayer *head )
{
	while( head )
	{
		XcfLayer *next = head->next ;
		if( head->properties )
			free_xcf_properties( head->properties );
		if( head->hierarchy )
			free_xcf_hierarchy( head->hierarchy );
		if( head->mask )
		{
			if( head->mask->properties )
				free_xcf_properties( head->mask->properties );
			if( head->mask->hierarchy )
				free_xcf_hierarchy( head->mask->hierarchy );
			free( head->mask );
		}
		free( head );
		head = next ;
	}
}

void
free_xcf_image( XcfImage *xcf_im )
{
	if( xcf_im )
	{
		if( xcf_im->properties )
			free_xcf_properties( xcf_im->properties );
		if( xcf_im->colormap )
			free( xcf_im->colormap );
		if( xcf_im->layers )
			free_xcf_layers( xcf_im->layers );
		if( xcf_im->channels )
			free_xcf_channels( xcf_im->channels );
	}
}


/*******************************************************************************/
/* detail loading functions : 												   */

static XcfProperty *
read_xcf_props( FILE *fp )
{
	XcfProperty *head = NULL;
	XcfProperty **tail = &head;
	CARD32 prop_vals[2] ;

	do
	{
		if( xcf_read32( fp, &(prop_vals[0]), 2 ) < 2 )
			break;
		if( prop_vals[0] != 0 )
		{
			*tail = safecalloc( 1, sizeof(XcfProperty));
			(*tail)->id  = prop_vals[0] ;
			(*tail)->len = prop_vals[1] ;
			if( (*tail)->len > 0 )
			{
				if( (*tail)->len <= 8 )
					(*tail)->data = (CARD8*)&((*tail)->buffer[0]) ;
				else
					(*tail)->data = safemalloc( (*tail)->len );
				xcf_read8( fp, (*tail)->data, (*tail)->len );
			}
			tail = &((*tail)->next);
		}
	}while( prop_vals[0] != 0 );
	return head;
}

static XcfListElem *
read_xcf_list_offsets( FILE *fp, size_t elem_size )
{
	XcfListElem *head = NULL ;
	XcfListElem **tail = &head ;
	CARD32 offset ;

	do
	{
		if( xcf_read32( fp, &offset, 1 ) < 1 )
			break;
		if( offset != 0 )
		{
			*tail = safecalloc( 1, elem_size);
			(*tail)->any.offset  = offset ;
			tail = (XcfListElem**)&((*tail)->any.next);
		}
	}while( offset != 0 );
	return head;
}

static void
read_xcf_layers( XcfImage *xcf_im, FILE *fp, XcfLayer *head )
{
	XcfProperty *prop ;
	while( head )
	{
		fseek( fp, head->offset, SEEK_SET );
		if( xcf_read32( fp, &(head->width), 3 ) < 3 )
		{
			head->width = 0 ;
			head->height = 0 ;
			head->type = 0 ;
			continue;                          /* not enough data */
		}
		xcf_skip_string(fp);
		head->properties = read_xcf_props( fp );
		for( prop = head->properties ; prop != NULL ; prop = prop->next )
		{
			CARD32 *pd = (CARD32*)(prop->data) ;
			if( prop->id ==  XCF_PROP_OPACITY )
			{
				head->opacity = as_ntohl(*pd);
			}else if( prop->id ==  XCF_PROP_VISIBLE )
			{
				head->visible = ( *pd !=0);
			}else if( prop->id ==  XCF_PROP_PRESERVE_TRANSPARENCY )
			{
				head->preserve_transparency = (*pd!=0);
			}else if( prop->id == XCF_PROP_MODE )
			{
				head->mode = as_ntohl(*pd);
			}else if( prop->id == XCF_PROP_OFFSETS )
			{
				head->offset_x = as_ntohl(pd[0]);
				head->offset_y = as_ntohl(pd[1]);
			}
		}

		if( xcf_read32( fp, &(head->hierarchy_offset), 2 ) < 2 )
		{
			head->hierarchy_offset = 0 ;
			head->mask_offset = 0 ;
		}

		if( head->hierarchy_offset > 0 )
		{
			fseek( fp, head->hierarchy_offset, SEEK_SET );
			head->hierarchy = read_xcf_hierarchy( xcf_im, fp );
		}
		if( head->mask_offset > 0 )
		{
			fseek( fp, head->mask_offset, SEEK_SET );
			head->mask = read_xcf_mask( xcf_im, fp );
		}
		head = head->next ;
	}
}

static void
read_xcf_channels( XcfImage *xcf_im, FILE *fp, XcfChannel *head )
{
	XcfProperty *prop ;
	while( head )
	{
		fseek( fp, head->offset, SEEK_SET );
		if( xcf_read32( fp, &(head->width), 2 ) < 2 )
		{
			head->width = 0 ;
			head->height = 0 ;
			continue;                          /* not enough data */
		}
		xcf_skip_string(fp); 
		head->properties = read_xcf_props( fp );
		for( prop = head->properties ; prop != NULL ; prop = prop->next )
		{
			CARD32 *pd = (CARD32*)(prop->data) ;
			if( prop->id ==  XCF_PROP_OPACITY )
			{
				head->opacity = as_ntohl(*pd);
			}else if( prop->id ==  XCF_PROP_VISIBLE )
			{
				head->visible = ( *pd !=0);
			}else if( prop->id ==  XCF_PROP_COLOR )
			{
				head->color = MAKE_ARGB32(0xFF,prop->data[0],prop->data[1],prop->data[2]);
			}
		}

		if( xcf_read32( fp, &(head->hierarchy_offset), 1 ) < 1 )
			head->hierarchy_offset = 0 ;

		if( head->hierarchy_offset > 0 )
		{
			fseek( fp, head->hierarchy_offset, SEEK_SET );
			head->hierarchy = read_xcf_hierarchy( xcf_im, fp );
		}
		head = head->next ;
	}
}

static XcfLayerMask*
read_xcf_mask( XcfImage *xcf_im, FILE *fp )
{
	XcfLayerMask *mask = NULL ;
	CARD32        size[2] ;
	XcfProperty *prop ;

	if( xcf_read32( fp, &(size[0]), 2 ) < 2 )
		return NULL;

	mask = safecalloc( 1, sizeof( XcfLayerMask ) );
	mask->width = size[0] ;
	mask->height = size[0] ;
	xcf_skip_string(fp);

	mask->properties = read_xcf_props( fp );

	for( prop = mask->properties ; prop != NULL ; prop = prop->next )
	{
		CARD32 *pd = (CARD32*)(prop->data) ;
		if( prop->id ==  XCF_PROP_OPACITY )
		{
			mask->opacity = as_ntohl(*pd);
		}else if( prop->id ==  XCF_PROP_VISIBLE )
		{
			mask->visible = ( *pd !=0);
		}else if( prop->id ==  XCF_PROP_COLOR )
		{
			mask->color = MAKE_ARGB32(0xFF,prop->data[0],prop->data[1],prop->data[2]);
		}
	}

	if( xcf_read32( fp, &(mask->hierarchy_offset), 1 ) < 1 )
		mask->hierarchy_offset = 0 ;

	if( mask->hierarchy_offset > 0 )
	{
		fseek( fp, mask->hierarchy_offset, SEEK_SET );
		mask->hierarchy = read_xcf_hierarchy( xcf_im, fp );
	}
	return mask;
}

static XcfHierarchy*
read_xcf_hierarchy( XcfImage *xcf_im, FILE *fp )
{
	XcfHierarchy *h = NULL ;
	CARD32 h_props[3] ;

	if( xcf_read32( fp, &(h_props[0]), 3 ) < 3 )
		return NULL;
	h = safecalloc(1, sizeof(XcfHierarchy));
fprintf( stderr, "reading hierarchy %dx%d, bpp = %d\n", h_props[0], h_props[1], h_props[2] );		

	h->width  = h_props[0] ;
	h->height = h_props[1] ;
	h->bpp	  = h_props[2] ;

	h->levels = (XcfLevel*)read_xcf_list_offsets( fp, sizeof(XcfLevel));
	if( h->levels )
		read_xcf_levels( xcf_im, fp, h->levels );
	return h;
}

static void
read_xcf_levels( XcfImage *xcf_im, FILE *fp, XcfLevel *head )
{
	while( head )
	{
		fseek( fp, head->offset, SEEK_SET );
		if( xcf_read32( fp, &(head->width), 2 ) < 2 )
		{
			head->width = 0 ;
			head->height = 0 ;
			continue;                          /* not enough data */
		}

		head->tiles = (XcfTile*)read_xcf_list_offsets( fp, sizeof(XcfTile));
		if( head->tiles )
		{
			if( xcf_im->compression == XCF_COMPRESS_NONE )
				read_xcf_tiles( xcf_im, fp, head->tiles );
			else if( xcf_im->compression == XCF_COMPRESS_RLE )
				read_xcf_tiles_rle( xcf_im, fp, head->tiles );
		}
		head = head->next ;
	}
}

static void
read_xcf_tiles( XcfImage *xcf_im, FILE *fp, XcfTile *head )
{
	while( head )
	{
		head->estimated_size = XCF_TILE_WIDTH*XCF_TILE_HEIGHT*4 ;
		head = head->next ;
	}
}


static void
read_xcf_tiles_rle( XcfImage *xcf_im, FILE *fp, XcfTile *head )
{
	while( head )
	{
		if( head->next )
			head->estimated_size = head->next->offset - head->offset ;
		else
			head->estimated_size = (XCF_TILE_WIDTH*XCF_TILE_HEIGHT)*6 ;
		head = head->next ;
	}
}

