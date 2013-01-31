/*
 * Copyright (c) 2000 Andrew Ferguson <andrew@owsla.cjb.net>
 * Copyright (c) 1998 Sasha Vasko <sasha at aftercode.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.   See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

/*#define DO_CLOCKING      */

#define LOCAL_DEBUG
#include "../configure.h"
#include "asapp.h"
#include "session.h"
#include "afterstep.h"
#include "screen.h"
#include "parser.h"
#include "functions.h"
#include "freestor.h"
#include "../libAfterImage/afterimage.h"


/*************************************************************************/
/* parsing code :
 */
TermDef      *
txt2fterm (const char *txt, int quiet)
{
	TermDef      *fterm;

	if (pFuncSyntax->term_hash == NULL)
		PrepareSyntax ((SyntaxDef*)pFuncSyntax);
	if ((fterm = FindStatementTerm ((char *)txt, (SyntaxDef*)pFuncSyntax)) == NULL && !quiet)
		show_error ("unknown function name in function specification [%s].\n", txt);

	return fterm;
}

int
txt2func (const char *text, FunctionData * fdata, int quiet)
{
	TermDef      *fterm;

	for (; isspace (*text); text++);
	fterm = txt2fterm (text, quiet);
	if (fterm != NULL)
	{
		init_func_data (fdata);
		fdata->func = fterm->id;
		for (; !isspace (*text) && *text; text++);
		for (; isspace (*text); text++);
		if (*text)
		{
			const char   *ptr = text + strlen ((char*)text);

			for (; isspace (*(ptr - 1)); ptr--);
			fdata->text = mystrndup (text, ptr - text);
		}
	}
	return (fterm != NULL);
}

int
parse_func (const char *text, FunctionData * data, int quiet)
{
	TermDef      *fterm;
	char         *ptr;
	int           curr_arg = 0;
	int           sign = 0;

	init_func_data (data);
	for (ptr = (char *)text; isspace (*ptr); ptr++);
	if (*ptr == '\0')
	{
		if (!quiet)
			show_error ("empty function specification encountered.%s");
		return -1;
	}

	if ((fterm = txt2fterm (ptr, quiet)) == NULL)
		return -2;

	if (IsInternFunc (fterm->id))
		return 0;

	while (!isspace (*ptr) && *ptr)
		ptr++;
	data->func = fterm->id;
	if (fterm->flags & TF_SYNTAX_TERMINATOR)
		return 0;

	set_func_val (data, -1, default_func_val(data->func));

	/* now let's do actual parsing */
	if (!(fterm->flags & NEED_CMD))
		ptr = stripcomments (ptr);
	else
	{										   /* we still want to strip trailing whitespaces */
		char         *tail = ptr + strlen (ptr) - 1;

		for (; isspace (*tail) && tail > ptr; tail--);
		*(tail + 1) = '\0';
	}
	/* this function is very often called so we try to use as little
	   calls to other function as possible */
	for (; *ptr; ptr++)
	{
		if (!isspace (*ptr))
		{
			int           is_text = 0;

			if (*ptr == '"')
			{
				char         *tail = ptr;
				char         *text;

				while (*(tail + 1) && *(tail + 1) != '"')
					tail++;
				if (*(tail + 1) == '\0')
				{
					show_error ("impaired doublequotes encountered in [%s].", ptr);
					return -3;
				}
				text = mystrndup (ptr + 1, (tail - ptr));
				if (data->name == NULL)
					data->name = text;
				else if (data->text == NULL)
					data->text = text;
				ptr = tail + 1;
			} else if (isdigit (*ptr))
			{
				int           count;
				char          unit = '\0';
				int           val = 0;

				for (count = 1; isdigit (*(ptr + count)); count++);
				if (*(ptr + count) != '\0' && !isspace (*(ptr + count)))
					is_text = (!isspace (*(ptr + count + 1)) && *(ptr + count + 1) != '\0') ? 1 : 0;
				if (is_text == 0)
					ptr = parse_func_args (ptr, &unit, &val) - 1;
				if (curr_arg < MAX_FUNC_ARGS)
				{
					data->func_val[curr_arg] = (sign != 0) ? val * sign : val;
					data->unit[curr_arg] = unit;
					curr_arg++;
				}
			} else if (*ptr == '-')
			{
				if (sign == 0)
				{
					sign--;
					continue;
				} else
					is_text = 1;
			} else if (*ptr == '+')
			{
				if (sign == 0)
				{
					sign++;
					continue;
				} else
					is_text = 1;
			} else
				is_text = 1;

			if (is_text)
			{
				if (sign != 0)
					ptr--;
				if (data->text == NULL)
				{
					if (fterm->flags & NEED_CMD)
					{
						data->text = mystrdup (ptr);
						break;
					}
					ptr = parse_token (ptr, &(data->text)) - 1;
				} else
					while (*(ptr + 1) && !isspace (*(ptr + 1)))
						ptr++;
			}
			sign = 0;
		}
	}

	decode_func_units (data);
	data->hotkey = scan_for_hotkey (data->name);

	/* now let's check for valid number of arguments */
	if ((fterm->flags & NEED_NAME) && data->name == NULL)
	{
		show_error ("function specification requires \"name\" in [%s].", text);
		return FUNC_ERR_NO_NAME;
	}
	if (data->text == NULL)
	{
		if ((fterm->flags & NEED_WINDOW) || ((fterm->flags & NEED_WINIFNAME) && data->name != NULL))
		{
			show_error ("function specification requires window name in [%s].", text);
			return FUNC_ERR_NO_TEXT;
		}
		if (fterm->flags & NEED_CMD)
		{
			show_error ("function specification requires shell command or full file name in [%s].", text);
			return FUNC_ERR_NO_TEXT;
		}
	}
/*
   if( data->func == F_SCROLL )
   {
   fprintf( stderr,"Function parsed: [%s] [%s] [%s] [%d] [%d] [%c]\n",fterm->keyword,data->name,data->text,data->func_val[0], data->func_val[1],data->hotkey );
   fprintf( stderr,"from: [%s]\n", text );
   }
 */
	return 0;
}

#if 0
FunctionData *
String2Func ( const char *string, FunctionData *p_fdata, Bool quiet )
{
	if( p_fdata )
		free_func_data( p_fdata );
	else
		p_fdata = safecalloc( 1, sizeof(FunctionData));

	LOCAL_DEBUG_OUT( "parsing message \"%s\"", string );
	if( parse_func( string, p_fdata, quiet ) < 0 )
	{
		LOCAL_DEBUG_OUT( "parsing failed%s", "" );
		free_func_data( p_fdata );
		free( p_fdata );
		p_fdata = NULL;
	}else
		LOCAL_DEBUG_OUT( "parsing success with func = %d", p_fdata->func );
	return p_fdata;
}

#endif
/****************************************************************************/
/* FunctionData - related code 	                                            */
void
init_func_data (FunctionData * data)
{
	int           i;

	if (data)
	{
		data->func = F_NOP;
		for (i = 0; i < MAX_FUNC_ARGS; i++)
		{
			data->func_val[i] = DEFAULT_OTHERS;
			data->unit_val[i] = 0;
			data->unit[i] = '\0';
		}
		data->hotkey = '\0';
		data->name = data->text = NULL;
		data->name_encoding = 0 ;
		data->popup = NULL ;
	}
}

void
copy_func_data (FunctionData * dst, FunctionData * src)
{
	if (dst && src)
	{
		register int  i;

		dst->func = src->func;
		dst->name = src->name;
		dst->name_encoding = src->name_encoding;
		dst->text = src->text;
		for (i = 0; i < MAX_FUNC_ARGS; i++)
		{
			dst->func_val[i] = src->func_val[i];
			dst->unit[i] = src->unit[i];
			dst->unit_val[i] = src->unit_val[i];
		}
		dst->hotkey = src->hotkey;
		dst->popup = src->popup;
	}
}

void
dup_func_data (FunctionData * dst, FunctionData * src)
{
	if (dst && src)
	{
		register int  i;

		dst->func = src->func;
		dst->name = mystrdup(src->name);
		dst->text = mystrdup(src->text);
		for (i = 0; i < MAX_FUNC_ARGS; i++)
		{
			dst->func_val[i] = src->func_val[i];
			dst->unit[i] = src->unit[i];
			dst->unit_val[i] = src->unit_val[i];
		}
		dst->hotkey = src->hotkey;
		dst->popup = src->popup;
	}
}

inline FunctionData *
create_named_function( int func, char *name)
{
	FunctionData *fdata = safecalloc( 1, sizeof(FunctionData) );
	init_func_data (fdata);
	fdata->func = func;
	if( name )
		fdata->name = mystrdup (name);
	return fdata ;
}



void
set_func_val (FunctionData * data, int arg, int value)
{
	int           i;

	if (arg >= 0 && arg < MAX_FUNC_ARGS)
		data->func_val[arg] = value;
	else
		for (i = 0; i < MAX_FUNC_ARGS; i++)
			data->func_val[i] = value;
}

int
free_func_data (FunctionData * data)
{
	if (data)
	{
#ifdef DEBUG_ALLOCS
		LOCAL_DEBUG_OUT( "freeing func = \"%s\"", data->name?data->name:"(null)" );
#endif
		if (data->name)
		{
			free (data->name);
			data->name = NULL;
		}
		if (data->text)
		{
#ifdef DEBUG_ALLOCS		
			LOCAL_DEBUG_OUT( "func->text = \"%s\"", data->text );
#endif
			free (data->text);
			data->text = NULL;
		}
		return data->func;
	}
	return F_NOP;
}

void
destroy_func_data( FunctionData **pdata )
{
	if( pdata && *pdata )
	{
		free_func_data( *pdata );
		free( *pdata );
		*pdata = NULL ;
	}
}

long
default_func_val( FunctionCode func )
{
	long val = 0 ;
	switch (func)
	{
	 case F_MAXIMIZE:
		 val = DEFAULT_MAXIMIZE;
		 break;
	 case F_MOVE:
	 case F_RESIZE:
		 val = INVALID_POSITION;
		 break;
	 default:
		 break;
	}
	return val;
}

void
decode_func_units (FunctionData * data)
{
	register int  i;
#if 0
	int defaults[MAX_FUNC_ARGS];

	defaults[0] = ASDefaultScrWidth;
	defaults[1] = ASDefaultScrHeight;
#endif
	for (i = 0; i < MAX_FUNC_ARGS; i++)
		switch (data->unit[i])
		{
		 case 'p':
		 case 'P':
			 data->unit_val[i] = 1;
			 break;
		 default:
			 data->unit_val[i] = 0/*defaults[i]*/;
		}
}

/**********************************************************************
 * complex function management code :
 **********************************************************************/
void
really_destroy_complex_func(ComplexFunction *cf)
{
	if( cf )
	{
		if( cf->magic == MAGIC_COMPLEX_FUNC )
		{
			register int i ;

			cf->magic = 0 ;
			if( cf->name )
				free( cf->name );
			if( cf->items )
			{
				for( i = 0 ; i < cf->items_num ; i++ )
					free_func_data( &(cf->items[i]) );
				free( cf->items );
			}
			free( cf );
		}
	}
}

void
complex_function_destroy(ASHashableValue value, void *data)
{
	ComplexFunction *cf = data ;

	if( (char*)value )
		free( (char*)value );
	if( cf && cf->magic == MAGIC_COMPLEX_FUNC )
	{
		if( cf->name == (char*)value )
			cf->name = NULL ;
		really_destroy_complex_func(cf);
	}else if(data)
		free(data);
}

void
init_list_of_funcs(struct ASHashTable **list, Bool force)
{
	if( list == NULL ) return ;

	if( force && *list != NULL )
		destroy_ashash( list );

	if( *list == NULL )
		*list = create_ashash( 0, casestring_hash_value,
								  casestring_compare,
								  complex_function_destroy);
}

/* list could be NULL here : */
ComplexFunction *
new_complex_func( struct ASHashTable *list, char *name )
{
	ComplexFunction *cf = NULL ;

	if( name == NULL )
		return NULL ;
	/* enlisting complex function is optional */
	cf = (ComplexFunction*) safecalloc (1, sizeof(ComplexFunction));
	cf->name = mystrdup(name);
	cf->magic = MAGIC_COMPLEX_FUNC ;
	if( list )
	{
		remove_hash_item( list, AS_HASHABLE(name), NULL, True);  /* we want only one copy */
		if( add_hash_item( list, AS_HASHABLE(cf->name), cf) != ASH_Success )
		{
			really_destroy_complex_func( cf );
			cf = NULL ;
		}
	}
	return cf;
}

ComplexFunction *
find_complex_func( struct ASHashTable *list, char *name )
{
	ASHashData hdata = {0} ;
	if( name && list )
		if( get_hash_item( list, AS_HASHABLE(name), &hdata.vptr) != ASH_Success )
			hdata.vptr = NULL ; /* we are being paranoid */
	return (ComplexFunction *)hdata.vptr ;
}

void
free_minipixmap_data (MinipixmapData *minipixmap) {
#ifdef DEBUG_ALLOCS
	LOCAL_DEBUG_OUT ("filename = \"%s\", image = %p", minipixmap->filename, minipixmap->image);
#endif	
	if( minipixmap->filename )
		free(minipixmap->filename);
	if( minipixmap->image )	{
		safe_asimage_destroy (minipixmap->image);
		minipixmap->image = NULL ;
	}
}

/***************************************************************
 * MenuData code :
 ***************************************************************/
/****************************************************************************
 * Implementing create-destroy functions :
 ****************************************************************************/
void
menu_data_item_destroy(MenuDataItem *mdi)
{
#ifdef DEBUG_ALLOCS			
LOCAL_DEBUG_CALLER_OUT( "menu_data_item_destroy(\"%s\",%p)", ((mdi && mdi->fdata)?(mdi->fdata->name):"NULL"), mdi);
#endif
	if( mdi )
	{
		if( mdi->magic == MAGIC_MENU_DATA_ITEM )
		{
			int i;
			mdi->magic = 0 ;
#ifdef DEBUG_ALLOCS			
LOCAL_DEBUG_OUT( "freeing func data %p", mdi->fdata );
#endif
			if( mdi->fdata )
			{
				free_func_data( mdi->fdata );
				free( mdi->fdata );
			}
			for ( i = 0 ; i < MINIPIXMAP_TypesNum ; ++i) 
				free_minipixmap_data (&(mdi->minipixmap[i]));

			if (mdi->item != NULL)
				free (mdi->item);
			if (mdi->item2 != NULL)
				free (mdi->item2);
			if (mdi->comment != NULL)
				free (mdi->comment);
		}
		free(mdi);
	}
}

void
purge_menu_data_items(MenuData *md)
{
	if( md )
	{
		MenuDataItem *mdi ;
		while( (mdi=md->first) != NULL )
		{
			md->first = mdi->next ;
			mdi->next = NULL ;
			menu_data_item_destroy( mdi );
		}
	}
}

void
destroy_menu_data( MenuData **pmd ) 
{
	if( pmd && *pmd) 
	{
		MenuData *md = *pmd ;
		if( md->magic == MAGIC_MENU_DATA )
		{	
#ifdef DEBUG_ALLOCS
LOCAL_DEBUG_CALLER_OUT( "menu_data_destroy(\"%s\", %p)", md->name?md->name:"(null)", md );
#endif
			if( md->name )
				free( md->name );
			if( md->comment )
				free( md->comment );

			purge_menu_data_items(md);
			md->magic = 0 ;
			free(md);
			*pmd = NULL ; 
		}
	}				 
}

MenuData *
create_menu_data( char *name ) 
{	
	MenuData *md = (MenuData*) safecalloc (1, sizeof(MenuData));
	md->name = mystrdup(name);
	md->magic = MAGIC_MENU_DATA ;
	return md;
}

MenuData    *
new_menu_data( ASHashTable *list, char *name )
{
	ASHashData hdata = {0} ;
	MenuData *md = NULL ;

	if( name == NULL )
		return NULL ;
	if( list == NULL ) return NULL;

	if( get_hash_item( list, AS_HASHABLE(name), &hdata.vptr) == ASH_Success )
		return (MenuData*)hdata.vptr;

	md = create_menu_data( name ); 

	if( add_hash_item( list, AS_HASHABLE(md->name), md) != ASH_Success )
		destroy_menu_data( &md );
	return md;
}

MenuData*
find_menu_data( ASHashTable *list, char *name )
{
	ASHashData hdata = {0} ;

	if( name && list )
		if( get_hash_item( list, AS_HASHABLE(name), &hdata.vptr) != ASH_Success )
			hdata.vptr = NULL ; /* we are being paranoid */
	return (MenuData*)hdata.vptr;
}



MenuDataItem *
new_menu_data_item( MenuData *menu )
{
	MenuDataItem * mdi = NULL;
	if( menu )
	{
		mdi = safecalloc(1, sizeof(MenuDataItem));
		mdi->magic = MAGIC_MENU_DATA_ITEM ;
		mdi->prev = menu->last ;
		if( menu->first == NULL )
			menu->first = mdi ;
		else
			menu->last->next = mdi ;
		menu->last = mdi ;
		++(menu->items_num);
	}
	return mdi;
}

void
assign_minipixmaps( MenuDataItem *mdi, MinipixmapData *minipixmaps )
{
	if( mdi && minipixmaps )
	{
		int i;
		for ( i = 0 ; i < MINIPIXMAP_TypesNum ; ++i) {
			LOCAL_DEBUG_OUT ("type = %d, filename = \"%s\"", i, minipixmaps[i].filename);
			if (minipixmaps[i].filename) {
				free_minipixmap_data (&(mdi->minipixmap[i]));
				mdi->minipixmap[i].filename = mystrdup (minipixmaps[i].filename);
				mdi->minipixmap[i].image = minipixmaps[i].image;
			}
		}
	}
}

Bool 
is_web_background (FunctionData *fdata)
{
	return (fdata->func == F_CHANGE_BACKGROUND_FOREIGN && is_url (fdata->text));
}

Bool 
check_fdata_availability( FunctionData *fdata )
{
	if( fdata == NULL ) 
		return False ;
	if ( fdata->func < F_ExecToolStart || fdata->func > F_ExecToolEnd )
	{	
		if(IsSwallowFunc(fdata->func) || IsExecFunc(fdata->func) )
		{
			LOCAL_DEBUG_OUT( "now really checking availability for \"%s\"", fdata->name?fdata->name:"nameless" );
			if (!is_executable_in_path (fdata->text))
			{
				LOCAL_DEBUG_OUT( "unavailable :  \"%s\"", fdata->name?fdata->name:"nameless" );
				return False ;
			}
		}else if( fdata->func == F_CHANGE_BACKGROUND_FOREIGN && !is_web_background (fdata))
		{
			ASImageFileTypes type = check_asimage_file_type( fdata->text );
			LOCAL_DEBUG_OUT( "foreign back image \"%s\" has type %d", fdata->text, type );
			if( type > ASIT_Supported ) 
			{
				LOCAL_DEBUG_OUT( "foreign back image of unknown type :  \"%s\"", fdata->text );
				return False;
			}
		}
	}else if ( fdata->func == F_ExecInTerm )
	{
		char *target = strstr( fdata->text, "-e ");
		target = target? target+3 : fdata->text;
		if (!is_executable_in_path (target))
		{
			LOCAL_DEBUG_OUT( "unavailable :  \"%s\", target = \"%s\"", fdata->name?fdata->name:"nameless", target );
			return False;
		}
	}

	return True;
}

static void
check_availability( MenuDataItem *mdi )
{
	clear_flags( mdi->flags, MD_Disabled );
#ifndef NO_AVAILABILITYCHECK
	LOCAL_DEBUG_OUT( "checking availability for \"%s\"", mdi->fdata->name?mdi->fdata->name:"nameless" );
	check_fdata_availability( mdi->fdata );
#endif /* NO_AVAILABILITYCHECK */
}

MenuDataItem *
add_menu_data_item( MenuData *menu, int func, char *name, MinipixmapData *minipixmaps )
{
	MenuDataItem *mdi = new_menu_data_item(menu);

	if( mdi )
	{
		mdi->fdata->func = func ;
		mdi->fdata->name = mystrdup(name);
		check_availability( mdi );
		assign_minipixmaps( mdi, minipixmaps );
	}
	return mdi;
}

MenuDataItem *
add_menu_fdata_item( MenuData *menu, FunctionData *fdata, MinipixmapData *minipixmaps)
{
	MenuDataItem *mdi = NULL;

	if( fdata )
		if( (mdi = new_menu_data_item(menu)) != NULL )
		{
			mdi->fdata = safecalloc( 1, sizeof(FunctionData) );
			copy_func_data( mdi->fdata, fdata);
			memset( fdata, 0x00, sizeof( FunctionData ) );
			parse_menu_item_name (mdi, &(mdi->fdata->name)) ;
			check_availability( mdi );
			assign_minipixmaps( mdi, minipixmaps );
			
			LOCAL_DEBUG_OUT( "mdi_fdata_encoding = %d, fdata_encoding = %d",
			mdi->fdata->name_encoding, fdata->name_encoding );
		}
	return mdi;
}

ASImage *
check_scale_menu_pmap( ASImage *im, ASFlagType flags ) 
{	
	if( im )
	{
		int w = im->width ;
		int h = im->height ;
		if( w > h )
		{
			if( w > MAX_MENU_ITEM_HEIGHT )
			{
				w = MAX_MENU_ITEM_HEIGHT ;
				h = (h * w)/im->width ;
				if( h == 0 )
					h = 1 ;
			}
		}else if( h > MAX_MENU_ITEM_HEIGHT )
		{
			h = MAX_MENU_ITEM_HEIGHT ;
			w = (w * h)/im->height ;
			if( w == 0 )
				w = 1 ;
		}
		if( get_flags( flags, MD_ScaleMinipixmapDown ) )
		{
			int tmp_h = asxml_var_get(ASXMLVAR_MenuFontSize)+8;				
			int tmp_w = (w*tmp_h)/h ; 
			if( w  > tmp_w || h > tmp_h ) 
			{
				w = tmp_w ; 
				h = tmp_h ; 	   
			}	 
		}else if( get_flags( flags, MD_ScaleMinipixmapUp ) )
		{
			int tmp_w = asxml_var_get(ASXMLVAR_MinipixmapWidth);				   
			int tmp_h = asxml_var_get(ASXMLVAR_MinipixmapHeight) ; 
			if( w  > tmp_w || h > tmp_h ) 
			{
				w = tmp_w ; 
				h = tmp_h ; 	   
			}	 
		}

		if( w != im->width || h != im->height )
		{
			return scale_asimage( ASDefaultVisual, im, w, h, ASA_ASImage, 100, ASIMAGE_QUALITY_DEFAULT );
		}
	}
	return im;
}

void
free_menu_pmaps( MenuData *menu)
{
	MenuDataItem *curr ;
	LOCAL_DEBUG_OUT( "menu = %p, image_manager = %p", menu, ASDefaultScr->image_manager );

	for( curr = menu?menu->first:NULL ; curr != NULL ; curr = curr->next ) {	
		int i;
		for ( i = 0 ; i < MINIPIXMAP_TypesNum ; ++i)
			if( curr->minipixmap[i].image )	{
				safe_asimage_destroy(curr->minipixmap[i].image);
				curr->minipixmap[i].image = NULL ; 
			}
	}
}

void
load_menuitem_pmap (MenuDataItem *mdi, MinipixmapTypes type, Bool force)
{
	MinipixmapData *minipixmap = &(mdi->minipixmap[type]);
	int func = mdi->fdata->func;
	char *filename = minipixmap->filename;

	if (minipixmap->image != NULL)
		return;

	if (filename == NULL) {
		if (type == MINIPIXMAP_Icon){
			if (func == F_CHANGE_BACKGROUND_FOREIGN && !is_web_background (mdi->fdata)) 
				filename = mdi->fdata->text;
		} else if (type == MINIPIXMAP_Preview){
			if (func == F_CHANGE_BACKGROUND_FOREIGN || func == F_CHANGE_BACKGROUND )
				filename = mdi->fdata->text;
		} 
	}

	LOCAL_DEBUG_OUT ("type = %d, filename = \"%s\", fdata->text = \"%s\"", type, filename, mdi->fdata->text);
	if (filename)
	{
		char *cachedFileName = NULL;
		int h = MAX_MENU_ITEM_HEIGHT ; 
		if (type == MINIPIXMAP_Preview)
			h = ASDefaultScr->MyDisplayHeight/5;
		else if( get_flags( mdi->flags, MD_ScaleMinipixmapDown ) )
			h = asxml_var_get(ASXMLVAR_MenuFontSize)+8;				
		else if( get_flags( mdi->flags, MD_ScaleMinipixmapUp ) )
			h = asxml_var_get(ASXMLVAR_MinipixmapHeight) ; 
		LOCAL_DEBUG_OUT( "minipixmap target height = %d for \"%s\"", h, minipixmap->filename);

		if (func == F_CHANGE_BACKGROUND || func == F_CHANGE_BACKGROUND_FOREIGN) {
			Bool use_thumbnail = True;
			if (is_web_background (mdi->fdata) && is_url (filename)) {
				int s1 = 0, s2 = 0;
				cachedFileName = make_session_webcache_file (Session, filename);
				use_thumbnail = check_download_complete (0, cachedFileName, &s1, &s2);
				use_thumbnail = use_thumbnail && ((s1 > 0 || s2 == s1));
				if (use_thumbnail) 
					filename = cachedFileName;
			}
			
			if (type == MINIPIXMAP_Preview) {
				if (is_web_background (mdi->fdata) && is_url (filename)){
					if (!use_thumbnail && force && minipixmap->loadCount == 0) {
						spawn_download (filename, cachedFileName);
						minipixmap->loadCount++;
					}
				} else if (filename != minipixmap->filename) {
					use_thumbnail = force;
				}
			}

			if (use_thumbnail)
				minipixmap->image = get_thumbnail_asimage (ASDefaultScr->image_manager, filename, 0, h, AS_THUMBNAIL_PROPORTIONAL|AS_THUMBNAIL_DONT_ENLARGE );
			destroy_string (&cachedFileName);
		 } else
			minipixmap->image = load_environment_icon ("apps", filename, h);
		LOCAL_DEBUG_OUT( "minipixmap = \"%s\", minipixmap_image = %p",  filename, minipixmap->image );
	}
}


void
reload_menuitem_pmap( MenuDataItem *mdi, MinipixmapTypes type, Bool force )
{
	MinipixmapData *minipixmap = &(mdi->minipixmap[type]);

	LOCAL_DEBUG_OUT ("type = %d, fdata->text = \"%s\"", type, mdi->fdata->text);

	if (minipixmap->image)
	{
		if (minipixmap->filename != NULL && !force) 
		{
			if (get_asimage_file_type( ASDefaultScr->image_manager, minipixmap->filename) != ASIT_XMLScript)
			{
				ASImage *im = minipixmap->image ; 
				if (im->imageman != NULL && im->imageman != ASDefaultScr->image_manager) {
					/* need to engage in trickery : */
					relocate_asimage (ASDefaultScr->image_manager, im);
				}
				/* fprintf( stderr, "minipixmap \"%s\" is not an XML script - skipping\n", minipixmap ); */
				return;
			}
		}
		safe_asimage_destroy(minipixmap->image);
		minipixmap->image = NULL ; 
	}

	load_menuitem_pmap (mdi, type, False);
}

void
reload_menu_pmaps( MenuData *menu, Bool force )
{
	MenuDataItem *curr ;
	LOCAL_DEBUG_OUT( "menu = %p, image_manager = %p", menu, ASDefaultScr->image_manager );
	if( menu && ASDefaultScr->image_manager ) 
		for( curr = menu->first ; curr != NULL ; curr = curr->next )
		{	
			int i;
			for (i = 0 ; i < MINIPIXMAP_TypesNum ; ++i)
				reload_menuitem_pmap (curr, i, force);
		}
}


/* this is very often used function so we optimize it as good as we can */
int
parse_menu_item_name (MenuDataItem * item, char **name)
{
	register int  i;
	register char *ptr = *name;

	if (ptr == NULL || item == NULL)
		return -1;

	for (i = 0; ptr[i] && ptr[i] != '\t';  i++);

	if (ptr[i] == '\0')
	{
		item->item = *name;
		*name = NULL;						   /* that will prevent us from memory deallocation */
		item->item2 = NULL;
	} else
	{
		item->item = mystrndup (*name, i);
		item->item2 = mystrdup (&(ptr[i+1]));
	}

	if( (ptr = item->item) != NULL )
		for (i = 0; ptr[i] ; i++)
			if( ptr[i] == '_' )
				ptr[i] = ' ';

	if( (ptr = item->item2) != NULL )
		for (i = 0; ptr[i] ; i++)
			if( ptr[i] == '_' )
				ptr[i] = ' ';

	return 0;
}

MenuDataItem     *
menu_data_item_from_func (MenuData * menu, FunctionData * fdata, Bool do_check_availability)
{
	MenuDataItem     *item = NULL;

	if( fdata != NULL )
	{	
		MinipixmapTypes mtype = GetMinipixmapType(fdata->func);
		if (mtype < MINIPIXMAP_TypesNum)
		{
			if (menu->last)
				item = menu->last;
			else
			{
				item = new_menu_data_item (menu);
				item->fdata = fdata;
			}
			if( fdata->func == F_SMALL_MINIPIXMAP ) 
				set_flags( item->flags, MD_ScaleMinipixmapDown );
			else if( fdata->func == F_LARGE_MINIPIXMAP ) 
				set_flags( item->flags, MD_ScaleMinipixmapUp );

			LOCAL_DEBUG_OUT ("type = %d, filename = \"%s\"", mtype, fdata->name);
			set_string (&(item->minipixmap[mtype].filename), mystrdup(fdata->name));
		} else
		{
			item = new_menu_data_item(menu);
			if (parse_menu_item_name (item, &(fdata->name)) >= 0)
				item->fdata = fdata;
			if( do_check_availability )
				check_availability(item);
		}
		if( item == NULL || item->fdata != fdata )
		{
			free_func_data (fdata);					   /* insurance measure */
			free( fdata );
		}
	}
	return item ;
}

int
compare_func_data_name(const void *a, const void *b) 
{
	FunctionData *fda = *(FunctionData **)a;
	FunctionData *fdb = *(FunctionData **)b;

	return strcmp(fda->name ? fda->name : "", fdb->name ? fdb->name : "");
}



void
print_func_data(const char *file, const char *func, int line, FunctionData *data)
{
	fprintf( stderr, "%s:%s:%s:%d>!!FUNC ", get_application_name(), file, func, line);
	if( data == NULL )
		fprintf( stderr, "NULL Function\n");
	else
	{
		TermDef      *term = func2fterm (data->func, True);
		if( term == NULL )
			fprintf( stderr, "Invalid Function %ld\n",(long) data->func);
		else
		{
			fprintf( stderr, "%s \"%s\" text[%s] ", term->keyword, data->name?data->name:"", data->text?data->text:"" );
			fprintf( stderr, "val0[%ld%c(%ld)] ", (long)data->func_val[0], (data->unit[0]=='\0')?' ':data->unit[0],(long)data->unit_val[0] );
			fprintf( stderr, "val1[%ld%c(%ld)] ", (long)data->func_val[1], (data->unit[1]=='\0')?' ':data->unit[1],(long)data->unit_val[1] );
			fprintf( stderr, "(popup=%p)\n", data->popup );
		}
	}
}



char *format_FunctionData (FunctionData *data)
{
	char *buffer = NULL;
	if (data != NULL)
	{
		int len = 0;
		int pos = 0;
		if (data->name)
			len += 1+strlen(data->name)+1;
		if (data->text)
			len += 1+strlen(data->text)+1;
		else
			len += 64;
		buffer = safemalloc (len);
		
		if (data->name)
		{
			int i = 0;
			buffer[pos++] = '"';
			while (data->name[i]) buffer[pos++] = data->name[i++];
			buffer[pos++] = '"';
		}
		if (data->text)
		{
			int i = 0;
			if (pos)
				buffer[pos++] = ' ';
			while (data->text[i]) buffer[pos++] = data->text[i++];
		}else
		{
#define FORMAT_FUNC_VALUE(val,unit) \
		do{ int i = unsigned_int2buffer_end (&buffer[pos], len-pos, ((val) < 0)? -(val) : (val)); \
			if (val < 0) buffer[pos++] = '-'; \
			while (buffer[i]) buffer[pos++] = buffer[i++]; \
			if (unit)buffer[pos++] = unit; }while(0)
	
			if (pos)
				buffer[pos++] = ' ';
			FORMAT_FUNC_VALUE(data->func_val[0],data->unit[0]);
			buffer[pos++] = ' ';
			FORMAT_FUNC_VALUE(data->func_val[1],data->unit[1]);

#undef FORMAT_FUNC_VALUE
		}
		buffer[pos++] = '\0';
	}
	
	return buffer;
}
