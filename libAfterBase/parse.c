/*
 * Copyright (c) 2000 Sasha Vasko <sasha at aftercode.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
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

#include "config.h"

#ifdef LOCAL_DEBUG
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

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "astypes.h"
#include "ashash.h"
#include "mystring.h"
#include "safemalloc.h"
#include "xwrap.h"
#include "parse.h"
#include "audit.h"
#include "output.h"
#include "xml.h"

/****************************************************************************
 * parse_argb_color - should be used for all your color parsing needs
 ***************************************************************************/
ASHashTable *custom_argb_colornames = NULL ;

void
register_custom_color(const char* name, CARD32 value) 
{
	ASHashData hdata = {0} ;

	if( name == NULL )
		return ;
	if (custom_argb_colornames == NULL )
    	custom_argb_colornames = create_ashash(0, casestring_hash_value, casestring_compare, string_destroy_without_data);

    /* Destroy any old data associated with this name. */
	remove_hash_item(custom_argb_colornames, AS_HASHABLE(name), NULL, False);

    show_progress("Defining color [%s] == #%X.", name, value);
	hdata.c32 = value ;
    add_hash_item(custom_argb_colornames, AS_HASHABLE(mystrdup(name)), hdata.vptr);
}

void
unregister_custom_color(const char* name)
{
	if( custom_argb_colornames )
	{
		if( name == NULL )
			flush_ashash (custom_argb_colornames);
		else
	      	remove_hash_item(custom_argb_colornames, AS_HASHABLE(name), NULL, False);
	}
}

void
custom_color_cleanup() 
{
	if (custom_argb_colornames != NULL )
    	destroy_ashash( &custom_argb_colornames );
}

Bool
get_custom_color(const char* name, CARD32 *color) {
	ASHashData hdata = {0} ;
	if( custom_argb_colornames )
	{
		/* show_progress( "Looking up custom color \"%s\"...", name ); */
      	if( get_hash_item(custom_argb_colornames, AS_HASHABLE(name), &hdata.vptr) != ASH_Success )
		{
			show_debug(__FILE__, "asvar_get", __LINE__, "Use of undefined variable [%s].", name);
		}else
		{
			*color = hdata.c32 ;
			show_progress( "Custom color \"%s\" found with argb #%8.8X", name, *color );
			return True;
		}
	}
	return False ;
}

const char *parse_argb_part( const char *color, int part, CARD32 *pargb )
{
	CARD32 new_val = 0x00FF;
	char buf[4] = "100";
	register char *ptr = (char*)&(color[0]);
	register int i = 0;
	CARD32 old_argb = 0xFF000000 ;
	const char *tail = color ;

	for( i = 0 ; i < 3 ; ++i )
	{
		if( !isdigit(ptr[i]) )
			break;
		buf[i] = ptr[i] ;
	}
	if( i > 0 )
		buf[i] = '\0' ;

	new_val = ((atoi( &(buf[0]) ) * 255)/100) &0x000000FF;

	if( ptr[i] == ',' )
	{
		++i ;
		tail = parse_argb_color( &(ptr[i]), &old_argb );
		if( tail != color )
		{
			switch( part )
			{
				case 0 : *pargb = (old_argb&0x00FFFFFF)|((new_val<<24)&0xFF000000); break ;
				case 1 : *pargb = (old_argb&0xFF00FFFF)|((new_val<<16)&0x00FF0000); break ;
				case 2 : *pargb = (old_argb&0xFFFF00FF)|((new_val<<8)&0x0000FF00); break ;
				case 3 : *pargb = (old_argb&0xFFFFFF00)|((new_val)&0x000000FF); break ;
			}
			if( *tail ==')' )
				++tail ;
		}
	}
	return tail;
}

static void parse_hsv2rgb8( unsigned int hue, unsigned int sat, unsigned int val, CARD32 *red8, CARD32 *green8, CARD32 *blue8 )
{
	int red = 0, green = 0, blue = 0;
	while( hue > 360 ) hue -= 360 ;
	if( sat >= 100 )
		sat = 100 ;
	if( val >= 100 )
		val = 100 ;
	if (sat == 0 || hue == 0 )
	{
    	*blue8 = *green8 = *red8 = (val*255)/100;
	}else
	{
		int delta = ((sat*(val/2))*255)/(50*100) ;
		int max_val = (val*255)/100;
		int min_val = max_val - delta;
		if( hue >= 0 && hue < 60 )
		{
			red   = max_val;
			green  = (hue*delta)/60 +min_val;
			blue   = min_val;
		}if( hue >= 60 && hue < 120 )
		{
			green = max_val;
			red   = max_val - ((hue-60)*delta)/60;
			blue  = min_val;
		}if( hue >= 120 && hue < 180 )
		{
			green = max_val;
			blue  = ((hue-120)*delta)/60 + min_val;
			red   = min_val;
		}if( hue >= 180 && hue < 240 )
		{
			blue  = max_val;
			green = max_val - ((hue-180)*delta)/60;
			red   = min_val;
		}if( hue >= 240 && hue < 300 )
		{
			blue  = max_val;
			red   = ((hue-240)*delta)/60 + min_val;
			green = min_val;
		}if( hue >= 300 && hue <= 360 )
		{
			red   = max_val;
			blue  = max_val-((hue-300)*delta)/60;
			green = min_val;
		}
		if( red < 0  ) 			red = 0 ;
		else if( red > 255 ) 	red = 255 ;
		if( green < 0  ) 		green = 0 ;
		else if( green > 255 ) 		green = 255 ;
		if( blue < 0  )  		blue = 0 ;
		else if( blue > 255 ) 	blue = 255 ;
		*red8 = red ;
		*green8 = green ;
		*blue8 = blue ;
	}
}

static void parse_rgb2hsv( CARD8 red8, CARD8 green8, CARD8 blue8, int *hue, int *sat, int *val )
{
	if( red8 == green8 && red8 == blue8 )
	{
		*sat = *hue = 0 ;
		*val = (red8*100)/255 ;
	}else
	{
		int segment = 0 ;
		int direction = 1 ;
		int min_v = red8, max_v = red8, mid_v = red8;
		if( red8 >= green8 )
		{
			min_v = green8 ;
			if( red8 < blue8 )
				segment = 240 ;
			else if( blue8 > green8 )
			{
				mid_v = blue8 ;
				direction = -1 ;
			}else
			{
				min_v = blue8 ;
				mid_v = green8 ;
			}
		}else if( green8 >= blue8 )
		{
			segment = 120 ;
			if( blue8 < red8 )
			{
				min_v = blue8;
				direction = -1 ;
			}else
				mid_v = blue8 ;
		}else
		{
			segment = 240 ;
			direction = -1 ;
			mid_v = green8 ;
		}
		if( segment == 240 )
			max_v = blue8 ;
		else if( segment == 120 )
			max_v = green8 ;

		if( direction > 0 )
		{
			*hue = segment + ((mid_v-min_v)*60)/((max_v>min_v)?max_v-min_v:1) ;
			if( *hue > 360 )
				*hue -= 360 ;
		}else
		{
			*hue = segment - ((mid_v-min_v)*60)/((max_v>min_v)?max_v-min_v:1) ;
			if( *hue < 0 )
				*hue += 360 ;
		}
		*sat = ((max_v - min_v)*100)/((max_v>mid_v)?max_v-mid_v:1) ;
		*val = (max_v*100)/255 ;
	}
}

const char *parse_hsv_part( const char *color, int part, CARD32 *pargb )
{
	CARD32 new_val = 0x00FF;
	char buf[4] = "100";
	register char *ptr = (char*)&(color[0]);
	register int i = 0;
	CARD32 old_argb = 0xFF000000 ;
	const char *tail = color ;

	for( i = 0 ; i < 3 ; ++i )
	{
		if( !isdigit(ptr[i]) )
			break;
		buf[i] = ptr[i] ;
	}
	if( i > 0 )
		buf[i] = '\0' ;


	new_val = atoi( &(buf[0]) );

	if( ptr[i] == ',' )
	{
		++i ;
		tail = parse_argb_color( &(ptr[i]), &old_argb );
		if( tail != color )
		{
			int hue, sat, val ;
			CARD32 red8, green8, blue8 ;
			parse_rgb2hsv( ((old_argb&0x00FF0000)>>16), ((old_argb&0x0000FF00)>>8), (old_argb&0x000000FF), &hue, &sat, &val );
			LOCAL_DEBUG_OUT( "old_argb = #%8.8lX, hue = %d, sat = %d, val = %d", old_argb, hue, sat, val );
			switch( part )
			{
				case 1 : hue = new_val ;  break ;
				case 2 : sat = new_val ;  break ;
				case 3 : val = new_val ;  break ;
			}
			parse_hsv2rgb8( hue, sat, val, &red8, &green8, &blue8 );
			*pargb = (old_argb&0xFF000000)|(red8<<16)|(green8<<8)|blue8;
			LOCAL_DEBUG_OUT( "new_argb = #%8.8lX, hue = %d, sat = %d, val = %d, r = %lx, g = %lx, b = %lx", *pargb, hue, sat, val, red8, green8, blue8 );

			if( *tail ==')' )
				++tail ;
		}
	}
	return tail;
}


const char *parse_argb_dec( const char *color, Bool has_alpha, Bool hsv, CARD32 *pargb )
{
	unsigned int dec_val[4] = {0xFF, 0, 0, 0} ;
	char buf[4] ;
	int k ;
	register char *ptr = (char*)&(color[0]);
	register int i = 0;
	CARD32 alpha8 = 0, red8 = 0, green8 = 0, blue8 = 0 ;

	for( k = has_alpha?0:1 ; k < 4 ; ++k )
	{
		if( !isdigit(ptr[0]) )
		{
			CARD32 tmp_argb = 0xFF000000 ;
			const char *tmp = parse_argb_color( ptr, &tmp_argb );
			if( tmp == &(ptr[i]) )
				break;
			if( k == 0 )
				dec_val[0] = (((tmp_argb>>24)&0x00FF)*100)>>8;
			else
			{   /* cannot translate argb to hsv here :( so handling only argb */
				switch( k )
				{
					case 1 : dec_val[1] = (((tmp_argb>>16)&0x00FF)*100)>>8;   break ;
					case 2 : dec_val[2] = (((tmp_argb>>8)&0x00FF)*100)>>8;   break ;
					case 3 : dec_val[3] = (((tmp_argb)&0x00FF)*100)>>8;   break ;
				}
			}
			i = tmp - ptr ;
		}else
		{
			for( i = 0 ; i < 3 ; ++i )
			{
				if( !isdigit(ptr[i]) )
					break;
				buf[i] = ptr[i] ;
			}
			buf[i] = '\0' ;
			dec_val[k] = atoi( &(buf[0]) );
		}
		if( !isdigit(ptr[i]) )
		{
			if( ptr[i] != ',' )
				break;
			++i ;
		}
		ptr += i ;
		i = 0 ;
	}
	alpha8 = dec_val[0]&0x00FF ;
	if( hsv )
	{
		parse_hsv2rgb8( dec_val[1], dec_val[2], dec_val[3], &red8, &green8, &blue8 );
	}else
	{
		red8   = dec_val[1]&0x00FF ;
		green8 = dec_val[2]&0x00FF ;
		blue8  = dec_val[3]&0x00FF ;
	}

	*pargb = (alpha8<<24)|(red8<<16)|(green8<<8)|blue8;

	if( ptr[i] ==')' )
		++i ;
	return &(ptr[i]);
}

const char *parse_argb_color( const char *color, CARD32 *pargb )
{
#define hextoi(h)   (isdigit(h)?((h)-'0'):(isupper(h)?((h)-'A'+10):((h)-'a'+10)))
	if( color )
	{
		if( *color == '#' )
		{
			CARD32 argb = 0 ;
			int len = 0 ;
			register const char *ptr = color+1 ;
			while( isxdigit((int)ptr[len]) ) len++;
			if( len >= 3)
			{
				if( (len&0x3) == 0 && len != 12 )
				{  /* we do have alpha channel !!! */
					len = len>>2 ;
					argb = (hextoi((int)ptr[0])<<28)&0xF0000000 ;
					if( len > 1 )
						argb |= (hextoi((int)ptr[1])<<24)&0x0F000000 ;
					else
						argb |= 0x0F000000;
					ptr += len ;
				}else
				{
					len = len/3 ;
					argb = 0xFF000000;
				}
				/* processing rest of the channels : */
				if( len == 1 )
				{
					argb |= 0x000F0F0F;
					argb |= (hextoi((int)ptr[0])<<20)&0x00F00000 ;
					argb |= (hextoi((int)ptr[1])<<12)&0x0000F000 ;
					argb |= (hextoi((int)ptr[2])<<4 )&0x000000F0 ;
					ptr += 3 ;
				}else
				{
					argb |= (hextoi((int)ptr[0])<<20)&0x00F00000 ;
					argb |= (hextoi((int)ptr[1])<<16)&0x000F0000 ;
					ptr += len ;
					argb |= (hextoi((int)ptr[0])<<12)&0x0000F000 ;
					argb |= (hextoi((int)ptr[1])<<8) &0x00000F00 ;
					ptr += len ;
					argb |= (hextoi((int)ptr[0])<<4 )&0x000000F0 ;
					argb |= (hextoi((int)ptr[1]))    &0x0000000F ;
					ptr += len ;
				}
				*pargb = argb ;
				return ptr;
			}
			return color;
		}

		if( color[0] == 'h' || color[0] == 'H' )
		{
			if( mystrncasecmp( &color[1], "sv(", 3) == 0 )
				return parse_argb_dec( &color[4], False, True, pargb );
			else if( mystrncasecmp( &color[1], "ue(", 3) == 0 )
				return parse_hsv_part( &color[4], 1, pargb );
		}else if( color[0] == 'r' || color[0] == 'r' )
		{
			if( mystrncasecmp( &color[1], "gb(", 3) == 0 )
				return parse_argb_dec( &color[4], False, False, pargb );
			else if( mystrncasecmp( &color[1], "ed(", 3) == 0 )
				return parse_argb_part( &color[4], 1, pargb );
		}else if( color[0] == 'a' || color[0] == 'A' )
		{
			if( mystrncasecmp( &color[1], "hsv(", 4) == 0)
				return parse_argb_dec( &color[5], True, True, pargb );
			else if( mystrncasecmp( &color[1], "rgb(", 4) == 0 )
				return parse_argb_dec( &color[5], True, False, pargb );
			else if( mystrncasecmp( &color[1], "lpha(", 5) == 0 )
				return parse_argb_part( &color[6], 0, pargb );
		}else if( color[0] == 'g' || color[0] == 'G' )
		{
			if( mystrncasecmp( &color[1], "reen(", 5) == 0 )
				return parse_argb_part( &color[6], 2, pargb );
		}else if( color[0] == 'b' || color[0] == 'B' )
		{
			if( mystrncasecmp( &color[1], "lue(", 4) == 0 )
				return parse_argb_part( &color[5], 3, pargb );
		}else if( color[0] == 's' || color[0] == 'S' )
		{
			if( mystrncasecmp( &color[1], "aturation(", 10) == 0 )
				return parse_hsv_part( &color[11], 2, pargb ); 
			else if( mystrncasecmp( &color[1], "at(", 3) == 0 )
				return parse_hsv_part( &color[4], 2, pargb );
		}else if( color[0] == 'v' || color[0] == 'V' )
		{
			if( mystrncasecmp( &color[1], "alue(", 5) == 0 )
				return parse_hsv_part( &color[6], 3, pargb );
			else if( mystrncasecmp( &color[1], "al(", 3) == 0 )
				return parse_hsv_part( &color[4], 3, pargb );
		}

		/* parsing as named color : */
		if( color[0] )
		{
			register char *ptr = (char*)&(color[0]);
			register int i = 0;
			Bool success = True ;

			while( isalnum((int)ptr[i]) ) ++i;
			if( ptr[i] != '\0' )
				ptr = mystrndup(&(color[0]), i );

			if( !get_custom_color( ptr, pargb) )
			{
#ifndef X_DISPLAY_MISSING
				XColor xcol, xcol_scr ;
				/* does not really matter here what screen to use : */
				if( AS_ASSERT(dpy) )
					success = False ;
				else
				{
					if( XLookupColor( dpy, DefaultColormap(dpy,DefaultScreen(dpy)), ptr, &xcol, &xcol_scr) )
						*pargb = 0xFF000000|((xcol.red<<8)&0x00FF0000)|(xcol.green&0x0000FF00)|((xcol.blue>>8)&0x000000FF);
					else
						success = False ;
				}
#endif
			}
			if( ptr != &(color[0]) )
				free( ptr );
			if( !success )
			{
				int orig_i = i ;
				ptr = (char*)&(color[0]);

				while( isalnum((int)ptr[i]) || ptr[i] == '.' || ptr[i] == '_') ++i;
				if( orig_i < i )
				{
					if( ptr[i] != '\0' )
						ptr = mystrndup(&(color[0]), i );
					success = get_custom_color( ptr, pargb);
					if( ptr != &(color[0]) )
						free( ptr );
				}
			}
			if( !success && strlen(color) < 30 )
				show_warning( "unable to parse color \"%s\"", color );
			return success?&(color[i]):color;
		}
	}
	return color;
}

const char *parse_hue( const char *color, int *hue )
{	  
	CARD32 argb ; 
	int dumm ;
	const char *res ;
	
	if( color == NULL || hue == NULL ) 
		return NULL;
	*hue = 0 ;
	res = parse_argb_color( color, &argb ) ;
	if( res != color ) 
	{
		parse_rgb2hsv( (argb&0x00FF0000)>>16, (argb&0x0000FF00)>>8, (argb&0x000000FF), hue, &dumm, &dumm );

		while( *hue < 0 ) 
			*hue += 360 ; 
		while( *hue >= 360 ) 
			*hue -= 360 ; 
	}

	return res;
}
/****************************************************************************
 * Some usefull parsing functions
 ****************************************************************************/
char*
find_doublequotes ( const char *ptr)
{
	register int i = 0 ;
	if (ptr == NULL)
		return NULL;

	if (ptr[i] == '"')
		++i;

	while(ptr[i] != '"')
	{
		if( ptr[i] == '\\' ) 
			++i ;
		++i ;
		if( ptr[i] == '\0' )
		   	return NULL;
	}
	return (char*)&ptr[i];
}

/****************************************************************************
 * Copies a string into a new, malloc'ed string
 * Strips leading and trailing whitespace
 ****************************************************************************/

char         *
stripcpy (const char *source)
{
	const char   *ptr = NULL;

	if( source == NULL ) 
		return NULL;
	for (; isspace ((int)*source); source++);
	for (ptr = source + strlen (source); ptr > source && isspace ((int)*(ptr - 1)); ptr--);
	return mystrndup (source, ptr - source);
}


/****************************************************************************
 *
 * Copies a string into a new, malloc'ed string
 * Strips all data before the first quote and after the second
 *
 ****************************************************************************/

char         *
stripcpy2 (const char *source, int tab_sensitive)
{
	const char *ptr = source, *tail;

	if (*ptr != '"')
		ptr = find_doublequotes (ptr);
	if (ptr != NULL)
	{
		if ((tail = find_doublequotes (ptr)) != NULL)
			return mystrndup (ptr + 1, tail - ptr - 1);
		return mystrdup (ptr + 1);
	}
	return stripcpy (source);
}


/* here we'll strip comments and skip whitespaces */
char         *
stripcomments2 (char *source, char **comments )
{
	int start = 0, curr = 0 ;

	/* remove comments from the line */
	while (isspace ((int)source[start])) ++start;
	
	for (curr = start; source[curr]; curr++)
	{
		if (source[curr] == '"')
		{
			const char *ptr = find_doublequotes (&source[curr]) ;
			if ( ptr == NULL )
			{
				while(source[++curr]); 
				break;
			}else
				curr = (int)(ptr - source) ;
		}
		if (source[curr] == '#')
		{									   /* checking if it is not a hex color code */
			int           i = curr;

			while(isxdigit ((int)source[++i]));

			if (i-curr < 4 || i-curr > 13 || (source[i] && !isspace ((int)source[i])))
			{
				if( comments ) 
					*comments = mystrdup( &source[curr+1] );
				break;
			}
			
			curr = i - 1;
		}
	}
	do{ --curr ; }while( curr > start && isspace ((int)source[curr]));
	source[curr+1] = '\0';
	return &source[start];
}

char         *
stripcomments (char *source)
{
	return stripcomments2 (source, NULL );
}



char         *
strip_whitespace (char *str)
{
	char         *ptr;

	/* strip trailing whitespace */
	for (ptr = str + strlen (str); ptr > str && isspace ((int)*(ptr - 1)); ptr--)
		*(ptr - 1) = '\0';
	/* skip leading whitespace */
	for (ptr = str; isspace ((int)*ptr); ptr++);
	return ptr;
}


/****************************************************************************
 *
 * Copies a whitespace separated token into a new, malloc'ed string
 * Strips leading and trailing whitespace
 *
 ****************************************************************************/
char *
tokenskip( const char *ptr, unsigned int n_tokens )
{
    int    tok_no = 0 ;  ;
    register int  curr = 0 ;

    if( ptr == NULL ) return NULL ;
    while( ++tok_no <= n_tokens && ptr[curr] != '\0' )
	{
		while (!isspace ((int)ptr[curr]) && ptr[curr])
        {   /* we have to match doublequotes if we encounter any, to allow for spaces in filenames */
			if (ptr[curr] == '"')
			{
				register const char *closing_quote = find_doublequotes (&ptr[curr]);
				if (closing_quote)
					curr = (int)(closing_quote-ptr);
			}
			++curr;
		}

		while (isspace ((int)ptr[curr])) ++curr;
	}
    return (char*)&ptr[curr];
}

/****************************************************************************
 * Copies a whitespace separated token into a new, malloc'ed string
 * Strips leading and trailing whitespace
 * returns pointer to the end of token in source string
 ****************************************************************************/
char         *
parse_token (const char *source, char **trg)
{
	int tok_start = 0, tok_end;
	char *next_tok_ptr ; 
	while( isspace(source[tok_start]) ) ++tok_start;
	next_tok_ptr = tokenskip( &source[tok_start], 1 );
	tok_end = (int)(next_tok_ptr-source) ; 
	if( tok_end > tok_start )
		if( source[tok_end] != '\0' )  /* then it points to beginning of the next token */ 	
			do{	--tok_end;}while( tok_end > tok_start && isspace(source[tok_end-1]));
	
	if( trg ) 
		*trg = mystrndup (&source[tok_start], tok_end - tok_start );
	
	return next_tok_ptr;
}


char         *
tokencpy (const char *source)
{
	char *token = NULL ; 
	parse_token (source, &token);	
	return token;
}

char         *
parse_tab_token (const char *source, char **trg)
{
	char         *ptr;

	for (; isspace ((int)*source); source++);
	for (ptr = (char *)source; *ptr != '\t' && *ptr; ptr++);
	*trg = mystrndup (source, ptr - source);
	return ptr;
}

char         *
parse_token_strip_quotes (const char *source, char **trg)
{
	char *token = NULL ; 
	char *next_token = parse_token (source, trg?&token:NULL);	
	if( trg ) 
	{	
		if( token && token[0] == '\"' )
		{	
			*trg = stripcpy2 (token, 0);
			free( token );
		}else
			*trg = token ;
	}
	return next_token;
}

/****************************************************************************
 *
 * Matches text from config to a table of strings.
 *
 ****************************************************************************/

struct config *
find_config (struct config *table, const char *text)
{
    for (; table->keyword[0] != '\0'; table++)
		if (!mystrncasecmp (text, table->keyword, strlen (table->keyword)))
			return table;

	return NULL;
}

/****************************************************************************
 * copy src to dest, backslash-escaping all non-alphanumeric characters in
 * src; write a maximum of maxlen chars to dest, including terminating zero
 ****************************************************************************/
int
quotestr (char *dest, const char *src, int maxlen)
{
	int           n = maxlen;

	/* require at least enough space for the terminating zero */
	if (maxlen < 1)
		return maxlen - n;
	n--;
	while (n && *src)
	{
		if (!isalnum ((int)*src) && n > 1)
		{
			*dest++ = '\\';
			n--;
		}
		*dest++ = *src++;
		n--;
	}
	*dest = '\0';
	return maxlen - n;
}

/* used in ParseMouse and in communication with modules */
char         *
parse_signed_int (register char *tline, int *val_return, int *sign_return)
{
	int  val = 0, sign = 0;
	register int i = 0 ;

	while (isspace ((int)tline[i])) ++i;

	switch( tline[i] )
	{ /* handling constructs like --10 or -+10 which is equivalent to -0-10or -0+10 */
		case '\0' : sign = 5 ; --i; break ;
		case '-' : 	sign = -1;
					if( tline[i+1] == '-' )
					{ ++i ; sign = -2 ; }
					else if( tline[i+1] == '+' )
					{ ++i ; sign = 3 ; }
					break;
		case '+' :	++sign;
					if( tline[i+1] == '-' )
					{ ++i ; sign = -3 ; }
					else if( tline[i+1] == '+' )
					{ ++i ; sign = 2 ; }
					break;
		case '=' :  break; /* skipping */
		case 'x' :
		case 'X' :  sign = 4; break;
	  default : --i ;
	}
	while (isdigit ((int)tline[++i]))
		val = val * 10 + (int)(tline[i] - '0');

	if( val_return )
		*val_return = (sign < 0)?-val:val ;
	if( sign_return )
		*sign_return = sign;
	return tline+i;
}

char         *
parse_func_args (char *tline, char *unit, int *func_val)
{
	tline = parse_signed_int( tline, func_val, NULL );

	*unit = *tline;
	if (isspace ((int)*tline))
		*unit = '\0' ;
	return tline[0]?tline+1:tline;
}

/* Math expression parsing algorithm. */
double parse_math(const char* str, char** endptr, double size) {
	double total = 0;
	char op = '+';
	char minus = 0;
	char logical_not = 0;
/*	const char* startptr = str; */
	if( str == NULL ) 
		return 0 ;

	while (isspace((int)*str)) str++;
	if( *str == '!' ) 
	{
		logical_not = 1;
		++str ;
	}else if( *str == '-' ) 
	{
		minus = 1 ;
		++str ;
	}

	while (*str) 
	{
		while (isspace((int)*str)) str++;
		if (!op) 
		{
			if (*str == '+' || *str == '-' || *str == '*' || *str == '/') op = *str++;
			else if (*str == '-') { minus = 1; str++; }
			else if (*str == '!') { logical_not = 1; str++; }
			else if (*str == ')') { str++; break; }
			else break;
		} else 
		{
			char* ptr;
			double num;
			
			if (*str == '(') 
				num = parse_math(str + 1, &ptr, size);
            else if (*str == '$') 
			{
            	for (ptr = (char*)str + 1 ; *ptr && !isspace(*ptr) && *ptr != '+' && *ptr != '-' && *ptr != '*' && *ptr != '!' && *ptr != '/' && *ptr != ')' ; ptr++);
               	num = asxml_var_nget((char*)str + 1, ptr - (str + 1));
            }else 
				num = strtod(str, &ptr);
			
			if (str != ptr) 
			{
				if (*ptr == '%') num *= size / 100.0, ptr++;
				if (minus) num = -num;
				if (logical_not) num = !num;
				
				if (op == '+') total += num;
				else if (op == '-') total -= num;
				else if (op == '*') total *= num;
				else if (op == '/' && num) total /= num;
			} else 
				break;
			str = ptr;
			op = '\0';
			minus = logical_not = 0;
		}
	}
	if (endptr) *endptr = (char*)str;
/* 	show_debug(__FILE__,"parse_math",__LINE__,"Parsed math [%s] with reference [%.2f] into number [%.2f].", startptr, size, total); */
	return total;
}



char         *
parse_geometry (register char *tline,
                int *x_return, int *y_return,
                unsigned int *width_return,
  				unsigned int *height_return,
				int* flags_return )
{
	int flags = 0 ;
	int sign, val ;

	tline = parse_signed_int( tline, &val, &sign );
	if( sign == 0 )
	{
		if( width_return )
		{
			*width_return = val ;
			set_flags( flags, WidthValue );
		}
		tline = parse_signed_int( tline, &val, &sign );
	}
	if( sign == 4 )
	{
		if( height_return )
		{
			*height_return = val ;
			set_flags( flags, HeightValue );
		}
		tline = parse_signed_int( tline, &val, &sign );
	}
	if( sign == 0 )
		sign = 1 ;
	if( sign == 1 || sign == -1)
	{
		if( x_return )
		{
			*x_return = val ;
			set_flags( flags, ( sign < 0 )?XNegative|XValue:XValue );
		}
		tline = parse_signed_int( tline, &val, &sign );
	}else if( sign != 5 )
	{
		if( x_return )
		{
			*x_return = 0 ;
			set_flags( flags, ( sign == -2 || sign == 3 )?XNegative|XValue:XValue );
		}
	}

	if( sign != 5 && y_return )
	{
		*y_return = val ;
		set_flags( flags, ( sign < 0 )?YNegative|YValue:YValue );
	}

	if( flags_return )
		*flags_return = flags ;

	return tline;
}

char         *
string_from_int (int param)
{
	char         *mem;
	register int  i;
	int           neg = 0;

	if (param == 0)
		return mystrdup ("0");
	if (param < 0)
	{
		param = -param;
		neg = 1;
	}
	for (i = 1; param >> (i * 3); i++);
	if (neg)
		i++;
	mem = safemalloc (i + 1);
	if (neg)
		mem[0] = '-';
	sprintf (&(mem[neg]), "%u", param);
	return mem;
}

static char _as_hex_to_char_table[] = "0123456789ABCDEF";

char         *
hex_to_buffer_reverse(void *data, size_t bytes, char* buffer)
{

    char *p = buffer;
    unsigned char *d = data;
    register int   i = bytes, k = 0 ;

    if( data == NULL || buffer == NULL )
        return buffer ;

    while( i > 0 )
    {
        i-- ;
        p[k++] = _as_hex_to_char_table[d[i]>>4];
        p[k++] = _as_hex_to_char_table[d[i]&0x0F];
    }
    return p+k;
}

char         *
hex_to_buffer(void *data, size_t bytes, char* buffer)
{
    char *p = buffer;
    unsigned char *d = data;
    register int   i = bytes, k = 0 ;

    if( data == NULL || buffer == NULL )
        return buffer ;

    while( i < bytes )
    {
        p[k++] = _as_hex_to_char_table[d[i]>>4];
        p[k++] = _as_hex_to_char_table[d[i]&0x0F];
        i++ ;
    }
    return p+k;
}

/***********************************************************************
 * Procedure:
 *	scanForHotkeys - Look for hotkey markers in a function "name"
 * Inputs:
 *	txt	- string to scan
 * Output:      - hotkey found
 ***********************************************************************/
char
scan_for_hotkey (char *txt)
{
	char          hotkey = '\0';

	if (txt != NULL)
		for (; *txt != '\0'; txt++)			   /* Scan whole string                 */
			if (*txt == '&')
			{								   /* A hotkey marker?                  */
				char         *tmp;			   /* Copy the string down over it      */

				for (tmp = txt; *tmp != '\0'; tmp++)
					*tmp = *(tmp + 1);
				if (*txt != '&')			   /* Not Just an escaped &            */
					hotkey = *txt;
			}
	return hotkey;
}


char         *
get_string_list_item (char *ptr, char **item_start, char **item_end, char separator)
{

	if (ptr == NULL)
		return NULL;
	while (*ptr && (isspace ((int)*ptr) || *ptr == separator))
		ptr++;
	if (*ptr == '\0')
		return NULL;

	*item_end = *item_start = ptr;
	if (*ptr == '"')
	{
		if ((*item_end = find_doublequotes (ptr)) == NULL)
			return NULL;
		ptr = *item_end;
		while (*ptr && !(isspace ((int)*ptr) || *ptr == separator))
			ptr++;
	} else
	{
		while (*ptr && *ptr != separator)
			ptr++;
		*item_end = ptr;
	}
	return ptr;
}

Bool
match_compound_string (char *string, char separator, char *what)
{
	if( string == NULL && what == NULL )
		return True;

	if( what != NULL )
	{
		register char *ptr = string ;
		if( ptr ) 
		{
			int what_len = strlen(what);
			while(*ptr)
			{
				char *item_start = NULL, *item_end = NULL;
				int item_len ;
				if ((ptr = get_string_list_item (ptr, &item_start, &item_end, separator)) == NULL)
					break;
				item_len = (item_end - item_start);
				if( item_len == what_len ) 
					if( strncmp(item_start, what, item_len ) == 0 ) 
						return True;
			}
		}
	}
	return False ;
}

char        **
compound_string2string_list (char *string, char separator, Bool duplicate, int *nitems_return)
{
	register char *ptr;
	int           items;
	char        **list = NULL, *item_start = NULL, *item_end = NULL;

	if (string == NULL )
		return NULL;

	ptr = string;
	items = 0;
	while (*ptr)
	{
		if ((ptr = get_string_list_item (ptr, &item_start, &item_end, separator)) == NULL)
			break;
		items++;
	}
	
	if( nitems_return ) 
		*nitems_return = items ;

	if (items > 0)
	{
		register int  i;

        list = safecalloc (items + 1, sizeof (char *));

		ptr = string;
		for (i = 0; i < items; i++)
		{
			if ((ptr = get_string_list_item (ptr, &item_start, &item_end, separator)) == NULL)
				break;
			if( duplicate ) 
				list[i] = mystrndup (item_start, (item_end - item_start));
			else
			{	
				list[i] = item_start ; 
				if( ptr == item_end && *ptr != '\0' )
					++ptr ; 
				*item_end = '\0';
			}
		}
	}
	return list;
}

char         *
string_list2compound_string (char **list, char separator)
{
	char         *string = NULL;

	if (list)
	{
		int           len = 0;
		register int  i = 0;

		for (i = 0; list[i]; i++)
			if (*list[i])
				len += strlen (list[i]) + 1;

		if (len > 0)
		{
			register char *src, *dst;

			dst = string = (char *)safemalloc (len);
			for (i = 0; list[i]; i++)
			{
				src = list[i];
				if (*src)
				{
					while (*src)
					{
						*(dst++) = *(src++);
					}
					*(dst++) = separator;
				}
			}
			*(dst - 1) = '\0';
		}
	}
	return string;
}




/* Conversion of the coma separated values into a list of strings */
char         *
get_comma_item (char *ptr, char **item_start, char **item_end)
{
	return get_string_list_item (ptr, item_start, item_end, ',');
}

char        **
comma_string2list (char *string)
{
	return compound_string2string_list (string, ',', True, NULL);
}

char         *
list2comma_string (char **list)
{
	return string_list2compound_string (list, ',');
}

void
destroy_string_list( char **list, int max_items )
{
    if( list )
    {
        register int i = 0;
        while( (i < max_items || max_items == 0) && list[i] )  { free( list[i] ); ++i; }
        free( list );
    }
}

char *
make_tricky_text( char *src )
{
    int len = 0, longest_line = 0;
    int pos = 0;
    char *trg ;
    register int i, k ;

    for( i = 0 ; src[i] ; i++ )
        if( src[i] == '\n' )
        {
            if( len > longest_line )
                longest_line = len;
            len = 0 ;
            pos++ ;
        }else
            len++ ;

    if( len > longest_line )
        longest_line = len;
    pos++ ;
    trg = safemalloc( longest_line*(pos+1)+1 );
    i = 0 ;
    for( pos = 0 ; pos < longest_line ; pos++ )
    {
        len = 0 ;
        for( k = 0 ; src[k] ; k++ )
        {
            if( src[k] == '\n' )
            {
                if( len <= pos )
                    trg[i++] = ' ';
                len = 0 ;
            }else
            {
                if( len == pos )
                    trg[i++] = src[k] ;
                len++ ;
            }
        }
        trg[i++] = '\n' ;
    }
    if( i )
        i-- ;
    trg[i] = '\0' ;
    return trg;
}

char *interpret_ascii_string( const char *str )
{
	int src_i = 0, dst_i = 0; 
	char *dst = safemalloc( (str?strlen(str):0)+1 );
	
	while( str[src_i] != '\0' ) 
	{
		if( str[src_i] == '\\' )
		{
			++src_i ; 
			if( str[src_i] == 't' ) 
				dst[dst_i++] = '\t' ;
			else if( str[src_i] == 'n' ) 
				dst[dst_i++] = '\n' ;
			else if( str[src_i] == 'r' ) 
				dst[dst_i++] = '\r' ;
			else if( str[src_i] == '\\' ) 
				dst[dst_i++] = '\\' ;
			else if( str[src_i] == '\0' ) 
				break;
		}else
			dst[dst_i++] = str[src_i] ;
		
		++src_i ;
	}
	dst[dst_i] = '\0' ;
	return dst;
}
