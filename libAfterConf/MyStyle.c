/*
 * Copyright (c) 1999 Sasha Vasko <sasha at aftercode.net>
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */


#define LOCAL_DEBUG
#include "../configure.h"
#include "../libAfterStep/asapp.h"
#include "../libAfterStep/afterstep.h"
#include "../libAfterStep/mystyle.h"
#include "../libAfterStep/parser.h"
#include "../libAfterStep/screen.h"

#include "afterconf.h"

#define DEBUG_MYSTYLE

TermDef       MyStyleTerms[] = {
/* including MyStyles definitions here so we can process them correctly */
	MYSTYLE_TERMS,
	{0, NULL, 0, 0, 0}
};

SyntaxDef     MyStyleSyntax = {
	'\n',
	'\0',
	MyStyleTerms,
	0,										   /* use default hash size */
    '\t',
    "",
    "",
    "Look MyStyle definition",
	"MyStyle",
	"defines combination of color, font, style, background to be used together",
	NULL,
	0
};


/*****************************************************************************
 *
 * This routine is responsible for reading and parsing the MyStyle settings
 *
 ****************************************************************************/

MyStyleDefinition *
AddMyStyleDefinition (MyStyleDefinition ** tail)
{
	MyStyleDefinition *new_def = safecalloc (1, sizeof (MyStyleDefinition));

	new_def->next = *tail;
	*tail = new_def;

	return new_def;
}

void
free_MSD_back_grad (MyStyleDefinition *msd)
{
	if( msd->back_grad_npoints > 0 )
	{
		if( msd->back_grad_colors )
		{
			register int i = msd->back_grad_npoints ;
			while( --i >= 0 )
				if( msd->back_grad_colors[i] )
					free( msd->back_grad_colors[i] );
			free( msd->back_grad_colors );
		}
		if( msd->back_grad_offsets )
			free( msd->back_grad_offsets );
	}
}

void
DestroyMyStyleDefinitions (MyStyleDefinition ** list)
{
	if (*list)
	{
		MyStyleDefinition *pnext = *list ;
		while( pnext != NULL )
		{
			MyStyleDefinition *pdef = pnext ;
			pnext = pdef->next ;
			if (pdef->name)
				free (pdef->name);
			if (pdef->inherit)
			{
				int i = pdef->inherit_num ;
				while( --i >= 0 )
					if (pdef->inherit[i])
						free (pdef->inherit[i]);
				free (pdef->inherit);
			}
			if( pdef->font)
				free (pdef->font);
			if( pdef->fore_color)
				free (pdef->fore_color);
			if( pdef->back_color)
				free (pdef->back_color);
			if( pdef->back_pixmap)
				free (pdef->back_pixmap);
			free_MSD_back_grad (pdef);
			DestroyFreeStorage (&(pdef->more_stuff));

			free (pdef);
		}
		*list = NULL ;
	}
}

void
PrintMyStyleDefinitions (MyStyleDefinition * list)
{
#ifdef DEBUG_MYSTYLE
	while(list)
	{
		if (list->name)
			fprintf (stderr, "\nMyStyle \"%s\"", list->name);
		if (list->inherit)
		{
			int i ;
			for( i = 0 ; i < list->inherit_num ; ++i )
		 		if (list->inherit[i])
					fprintf (stderr, "\n\tInherit \"%s\"", list->inherit[i]);
		}
		fprintf (stderr, "\n\tInheritCount %d", list->inherit_num);
		if (list->font)
			fprintf (stderr, "\n\tFont %s", list->font);
		if (list->fore_color)
			fprintf (stderr, "\n\tForeColor %s", list->fore_color);
		if (list->back_color)
			fprintf (stderr, "\n\tBackColor %s", list->back_color);
		fprintf (stderr, "\n\tTextStyle %d", list->text_style);
		if( get_flags( list->flags, MYSTYLE_DRAW_TEXT_BACKGROUND ) )
			fprintf (stderr, "\n\tDrawTextBackground");
		if( list->texture_type > 0 )
		{
			if( list->texture_type > TEXTURE_GRADIENT_END )
			{
				if (list->back_pixmap)
					fprintf (stderr, "\n\tBackPixmap %d %s", list->texture_type, list->back_pixmap);
				else
					fprintf (stderr, "\n\tBackPixmap %d", list->texture_type);
			}else
			{
				register int i = 0 ;
				fprintf (stderr, "\n\tBackMultiGradient %d", list->back_grad_type );
				while( i < list->back_grad_npoints  )
				{
					fprintf (stderr, "\t%s %f", list->back_grad_colors[i], list->back_grad_offsets[i]);
					++i ;
				}
			}
		}
		if (get_flags( list->flags, MYSTYLE_FINISHED) )
			fprintf (stderr, "\n~MyStyle");
		list = list->next ;
	}
#endif
}

MyStyleDefinition **
ProcessMyStyleOptions (FreeStorageElem * options, MyStyleDefinition ** tail)
{
	ConfigItem    item;

	item.memory = NULL;

	for (; options; options = options->next)
	{
		if (options->term == NULL)
			continue;
		if (options->term->id < MYSTYLE_ID_START || options->term->id > MYSTYLE_ID_END)
			continue;

		if (options->term->type == TT_FLAG)
		{
			switch (options->term->id)
			{
			 case MYSTYLE_DRAWTEXTBACKGROUND_ID:
				 set_flags((*tail)->flags, MYSTYLE_DRAW_TEXT_BACKGROUND);
				 break;
			 case MYSTYLE_DONE_ID:
				 if (*tail != NULL)
					 if ((*tail)->name != NULL)
					 {
						 set_flags( (*tail)->flags, MYSTYLE_FINISHED);
						 while (*tail)
							 tail = &((*tail)->next);	/* just in case */
						 break;
					 }
				 show_error ("Unexpected MyStyle definition termination. Ignoring.", "Unknown");
				 DestroyMyStyleDefinitions (tail);
				 break;
			 default:
				 ReadConfigItem (&item, NULL);
				 return tail;
			}
			continue;
		}

		if (!ReadConfigItem (&item, options))
			continue;

		if (*tail == NULL)
			AddMyStyleDefinition (tail);
#define SET_SET_FLAG1(f)  SET_SET_FLAG((**tail),f)
		switch (options->term->id)
		{
		 case MYSTYLE_START_ID:
			if ((*tail)->name != NULL)
			{
				show_error ("Previous MyStyle definition [%s] was not terminated correctly, and will be ignored.", (*tail)->name);
				DestroyMyStyleDefinitions (tail);
				AddMyStyleDefinition (tail);
			}
			(*tail)->name = item.data.string;
			break;
		 case MYSTYLE_INHERIT_ID:
		 	{
				int pos = ((*tail)->inherit_num) ;
				(*tail)->inherit = realloc( (*tail)->inherit, sizeof(char*)*(pos+1) );
				(*tail)->inherit[pos] = item.data.string;
				++((*tail)->inherit_num);
			}
			break;
		 case MYSTYLE_FONT_ID:
			set_string_value( &((*tail)->font), item.data.string, NULL, 0);
			break;
		 case MYSTYLE_FORECOLOR_ID:
			set_string_value( &((*tail)->fore_color), item.data.string, NULL, 0);
			break;
		 case MYSTYLE_BACKCOLOR_ID:
			set_string_value( &((*tail)->back_color), item.data.string, NULL, 0);
			break;
		 case MYSTYLE_TEXTSTYLE_ID:
			(*tail)->text_style = item.data.integer;
			set_flags( (*tail)->flags, MYSTYLE_TEXT_STYLE_SET );
			break;
		 case MYSTYLE_BACKGRADIENT_ID:
			if( options->argc != 3 )
				show_error( "invalid number of colors in BackGradient definition (%d)", options->argc );
			else
			{
				MyStyleDefinition *msd = *tail ;
				free_MSD_back_grad (msd);

				msd->back_grad_type = item.data.integer;
				msd->texture_type = mystyle_parse_old_gradient (msd->back_grad_type, 0, 0, NULL);
				msd->back_grad_npoints = 2 ;
				msd->back_grad_colors = safecalloc( 2, sizeof(char*));
				msd->back_grad_colors[0] = mystrdup (options->argv[1]);
				msd->back_grad_colors[1] = mystrdup (options->argv[2]);
				msd->back_grad_offsets = safecalloc( 2, sizeof(double));
				msd->back_grad_offsets[0] = 0.0 ;
				msd->back_grad_offsets[1] = 1.0 ;
			}
			break;
		 case MYSTYLE_BACKMULTIGRADIENT_ID:
			if( options->argc < 4 )
				show_error( "invalid number of colors in BackMultiGradient definition (%d)", options->argc );
			else
			{
				MyStyleDefinition *msd = *tail ;
				int i ;
				free_MSD_back_grad (msd);

				msd->back_grad_type = item.data.integer;
				msd->texture_type = mystyle_parse_old_gradient (msd->back_grad_type, 0, 0, NULL);
				msd->back_grad_npoints = ((options->argc-1)+1)/2 ;
				msd->back_grad_colors = safecalloc( msd->back_grad_npoints, sizeof(char*));
				msd->back_grad_offsets = safecalloc( msd->back_grad_npoints, sizeof(double));
				for( i = 0 ; i < msd->back_grad_npoints ; ++i )
				{
					msd->back_grad_colors[i] = mystrdup (options->argv[i*2+1]);
					if( i*2 + 2 < options->argc )
						msd->back_grad_offsets[i] = atof(options->argv[i*2+2]);
				}
				msd->back_grad_offsets[msd->back_grad_npoints-1] = 1.0 ;
			}
			break;
		 case MYSTYLE_BACKPIXMAP_ID:
		 	{
				MyStyleDefinition *msd = *tail ;
				if (options->argc > 1)
					set_string_value( &(msd->back_pixmap), mystrdup (options->argv[1]), NULL, 0 );
				else
				{
					free(msd->back_pixmap);
					msd->back_pixmap = NULL ;
				}

				msd->texture_type = item.data.integer;
				if( msd->texture_type < TEXTURE_TEXTURED_START || msd->texture_type > TEXTURE_TEXTURED_END )
				{
		            show_error("Error in MyStyle \"%s\": unsupported texture type [%d] in BackPixmap setting. Assuming default of [128] instead.", msd->name, msd->texture_type);
					msd->texture_type = msd->back_pixmap?TEXTURE_PIXMAP:TEXTURE_TRANSPARENT ;
				}
			}
			break;
		 default:
			 item.ok_to_free = 1;
			 ReadConfigItem (&item, NULL);
			 return tail;
		}
	}
	ReadConfigItem (&item, NULL);
	return tail;
}

void          mystyle_create_from_definition (MyStyleDefinition * def);

/*
 * this function process a linked list of MyStyle definitions
 * and create MyStyle for each valid definition
 * this operation is destructive in the way that all
 * data members of definitions that are used in MyStyle will be
 * set to NULL, so to prevent them being deallocated by destroy function,
 * and/or being used in other places
 * ATTENTION !!!!!
 * MyStyleDefinitions become unusable as the result, and get's destroyed
 * pointer to a list becomes NULL !
 */
void
ProcessMyStyleDefinitions (MyStyleDefinition ** list)
{
	MyStyleDefinition *pCurr;

	if (list)
		if (*list)
		{
			for (pCurr = *list; pCurr; pCurr = pCurr->next)
				mystyle_create_from_definition (pCurr);
			DestroyMyStyleDefinitions (list);
			mystyle_fix_styles ();
		}
}

void
mystyle_create_from_definition (MyStyleDefinition * def)
{
	int           i;
	MyStyle      *style;

	if (def == NULL)
		return;
	if (!get_flags(def->flags, MYSTYLE_FINISHED) || def->name == NULL)
		return;

	if ((style = mystyle_find (def->name)) == NULL)
	{
		style = mystyle_new_with_name (def->name);
		free( def->name );
		def->name = NULL;					   /* so it wont get deallocated */
	}

	if (def->inherit)
	{
		for (i = 0; i < def->inherit_num; ++i)
		{
			MyStyle      *parent;
			if (def->inherit[i])
			{
				if ((parent = mystyle_find (def->inherit[i])) != NULL)
					mystyle_merge_styles (parent, style, True, False);
				else
					show_error ("unknown style to inherit: %s\n", def->inherit[i]);
			}
		}
	}
	if( def->font )
	{
		if (get_flags(style->user_flags, F_FONT))
		{
			unload_font (&style->font);
			clear_flags(style->user_flags, F_FONT);
		}
		if (load_font (def->font, &style->font))
			set_flags(style->user_flags, F_FONT);
	}
	if( get_flags(def->flags, MYSTYLE_TEXT_STYLE_SET ) )
	{
		set_flags( style->user_flags, F_TEXTSTYLE );
		style->text_style = def->text_style;
	}
	if( def->fore_color )
	{
		if (parse_argb_color (def->fore_color, &(style->colors.fore)) != def->fore_color)
			set_flags(style->user_flags, F_FORECOLOR);
		else
    		show_error("unable to parse Forecolor \"%s\"", def->fore_color);
	}
	if( def->back_color )
	{
		if (parse_argb_color (def->back_color, &(style->colors.back)) != def->back_color)
		{
			style->relief.fore = GetHilite (style->colors.back);
			style->relief.back = GetShadow (style->colors.back);
			set_flags(style->user_flags, F_BACKCOLOR);
		}else
    		show_error("unable to parse BackColor \"%s\"", def->back_color);
	}
	if( def->texture_type > 0 && def->texture_type <= TEXTURE_GRADIENT_END )
	{
		int           type = def->back_grad_type;
		ASGradient    gradient = {0};
		Bool          success = False;

		if( type <= TEXTURE_OLD_GRADIENT_END )
		{
			ARGB32 color1 = ARGB32_White, color2 = ARGB32_Black;
			parse_argb_color( def->back_grad_colors[0], &color1 );
			parse_argb_color( def->back_grad_colors[1], &color2 );
			if ((type = mystyle_parse_old_gradient (type, color1, color2, &gradient)) >= 0)
			{
				if (style->user_flags & F_BACKGRADIENT)
				{
					free (style->gradient.color);
					free (style->gradient.offset);
				}
				style->gradient = gradient;
				style->gradient.type = mystyle_translate_grad_type (type);
				LOCAL_DEBUG_OUT( "style %p type = %d", style, style->gradient.type );
				success = True ;
			} else
            	show_error("Error in MyStyle \"%s\": invalid gradient type %d", style->name, type);
		}else
		{
			int i ;
			gradient.npoints = def->back_grad_npoints;
			gradient.color = safecalloc(gradient.npoints, sizeof(ARGB32));
			gradient.offset = safecalloc(gradient.npoints, sizeof(double));
			for( i = 0 ; i < gradient.npoints ; ++i )
			{
				ARGB32 color = ARGB32_White;
				parse_argb_color (def->back_grad_colors[i], &color );
				gradient.color[i] = color ;
				gradient.offset[i] = def->back_grad_offsets[i] ;
			}
			gradient.offset[0] = 0.0;
			gradient.offset[gradient.npoints - 1] = 1.0;
			if (style->user_flags & F_BACKGRADIENT)
			{
				 free (style->gradient.color);
				 free (style->gradient.offset);
			}
			style->gradient = gradient;
			style->gradient.type = mystyle_translate_grad_type (type);
			success = True ;
		}
		if( success )
		{
			style->texture_type = def->texture_type;
			set_flags( style->user_flags, F_BACKGRADIENT );
		}else
		{
			if (gradient.color != NULL)
				free (gradient.color);
			if (gradient.offset != NULL)
				free (gradient.offset);
            show_error("Error in MyStyle \"%s\": bad gradient", style->name);
		}
	}else if( def->texture_type > TEXTURE_GRADIENT_END && def->texture_type <= TEXTURE_TEXTURED_END )
	{
		int type = def->texture_type;

		if ( get_flags(style->user_flags, F_BACKPIXMAP) )
		{
			mystyle_free_back_icon(style);
			clear_flags (style->user_flags, F_BACKTRANSPIXMAP | F_BACKPIXMAP);
		}
		clear_flags (style->inherit_flags, F_BACKTRANSPIXMAP | F_BACKPIXMAP);
        LOCAL_DEBUG_OUT( "calling mystyle_free_back_icon for style %p", style );

		if (type == TEXTURE_TRANSPARENT || type == TEXTURE_TRANSPARENT_TWOWAY)
		{							   /* treat second parameter as ARGB tint value : */
			if (parse_argb_color (def->back_pixmap, &(style->tint)) == def->back_pixmap)
				style->tint = TINT_LEAVE_SAME;	/* use no tinting by default */
			else if (type == TEXTURE_TRANSPARENT)
				style->tint = (style->tint >> 1) & 0x7F7F7F7F;	/* converting old style tint */
/*LOCAL_DEBUG_OUT( "tint is 0x%X (from %s)",  style->tint, tmp);*/
			set_flags (style->user_flags, F_BACKPIXMAP);
			style->texture_type = type;
		} else
        {  /* treat second parameter as an image filename : */
        	if ( load_icon(&(style->back_icon), def->back_pixmap, Scr.image_manager ))
			{
				set_flags (style->user_flags, F_BACKPIXMAP);
				if (type >= TEXTURE_TRANSPIXMAP)
					set_flags (style->user_flags, F_BACKTRANSPIXMAP);
				style->texture_type = type;
			} else
            	show_error("Parsing MyStyle \"%s\" failed to load image file \"%s\".", style->name, def->back_pixmap);
		}
		LOCAL_DEBUG_OUT ("MyStyle \"%s\": BackPixmap %d image = %p, tint = 0x%lX", style->name,
						  style->texture_type, style->back_icon.image, style->tint);
	}
	if( get_flags( def->flags, MYSTYLE_DRAW_TEXT_BACKGROUND ) )
		set_flags( style->user_flags, F_DRAWTEXTBACKGROUND );

	clear_flags(style->inherit_flags, style->user_flags );
	style->set_flags = style->inherit_flags | style->user_flags;
}


void
mystyle_parse (char *tline, FILE * fd, char **myname, int *mystyle_list)
{
    FilePtrAndData fpd ;
    ConfigDef    *ConfigReader ;
    FreeStorageElem *Storage = NULL, *more_stuff = NULL;
    MyStyleDefinition **list = (MyStyleDefinition**)mystyle_list, **tail ;
	ConfigData cd ;

    if( list == NULL )
        return;
    for( tail = list ; *tail != NULL ; tail = &((*tail)->next) );

    fpd.fp = fd ;
    fpd.data = safemalloc( 12+1+strlen(tline)+1+1 ) ;
    sprintf( fpd.data, "MyStyle %s\n", tline );
	LOCAL_DEBUG_OUT( "fd(%p)->tline(\"%s\")->fpd.data(\"%s\")", fd, tline, fpd.data );
	cd.fileptranddata = &fpd ;
    ConfigReader = InitConfigReader ((char*)myname, &MyStyleSyntax, CDT_FilePtrAndData, cd, NULL);
    free( fpd.data );

    if (!ConfigReader)
        return ;

	PrintConfigReader (ConfigReader);
	ParseConfig (ConfigReader, &Storage);
	PrintFreeStorage (Storage);

	/* getting rid of all the crap first */
    StorageCleanUp (&Storage, &more_stuff, CF_DISABLED_OPTION);
    DestroyFreeStorage (&more_stuff);

    ProcessMyStyleOptions (Storage, tail);

	DestroyConfig (ConfigReader);
	DestroyFreeStorage (&Storage);
}


MyStyleDefinition *
GetMyStyleDefinition (MyStyleDefinition ** list, const char *name, Bool add_new)
{
	register MyStyleDefinition *style = NULL;

	if (name)
	{
		for (style = *list; style != NULL; style = style->next)
			if (mystrcasecmp (style->name, name) == 0)
				break;
		if (style == NULL && add_new)
		{									   /* add new style here */
			style = AddMyStyleDefinition (list);
			style->name = mystrdup (name);
		}
	}
	return style;
}


void
MergeMyStyleText (MyStyleDefinition ** list, const char *name,
				  const char *new_font, const char *new_fcolor, const char *new_bcolor, int new_style)
{
	register MyStyleDefinition *style = NULL;

	if ((style = GetMyStyleDefinition (list, name, True)) != NULL)
	{
		if (new_font)
			set_string_value( &(style->font), mystrdup (new_font), NULL, 0 );
		if (new_fcolor)
			set_string_value( &(style->fore_color), mystrdup (new_fcolor), NULL, 0 );
		if (new_bcolor)
			set_string_value( &(style->back_color), mystrdup (new_bcolor), NULL, 0 );
		if (new_style >= 0)
		{
			style->text_style = new_style;
			set_flags(style->flags, MYSTYLE_TEXT_STYLE_SET);
		}
	}
}

void
MergeMyStyleTextureOld (MyStyleDefinition ** list, const char *name,
						int type, char *color_from, char *color_to, char *pixmap)
{
	register MyStyleDefinition *style;

	if ((style = GetMyStyleDefinition (list, name, True)) == NULL)
		return;

#if 0
    if( !get_flags (style->set_flags, F_BACKPIXMAP) )
    {
        TextureDef *t = add_texture_def( &(style->back), &(style->back_layers));
        type = pixmap_type2texture_type( type );
        if ( TEXTURE_IS_ICON(type) )
        {
            if (pixmap != NULL )
            {
                set_string_value (&(t->data.icon.image), pixmap, &(style->set_flags), F_BACKTEXTURE);
                t->type = type;
            }
        } else if ( TEXTURE_IS_GRADIENT(type) )
        {
            if (color_from != NULL && color_to != NULL)
            {
                make_old_gradient (t, type, color_from, color_to);
            }
        } else if( TEXTURE_IS_TRANSPARENT(type) && pixmap )
        {
            set_string_value (&(t->data.icon.tint), pixmap, &(style->set_flags), F_BACKTEXTURE);
            style->back[0].type = type;
        }
    }
#endif
}

FreeStorageElem **
MyStyleDef2FreeStorage (SyntaxDef * syntax, FreeStorageElem ** tail, MyStyleDefinition * def)
{
	FreeStorageElem **new_tail= NULL;
#if 0
	register int  i;
	if (def == NULL)
		return tail;

	new_tail = QuotedString2FreeStorage (syntax, tail, NULL, def->name, MYSTYLE_START_ID);
	if (new_tail == tail)
		return tail;

	tail = &((*tail)->sub);

	for (i = 0; i < def->inherit_num; i++)
		tail = QuotedString2FreeStorage (&MyStyleSyntax, tail, NULL, def->inherit[i], MYSTYLE_INHERIT_ID);

	if (get_flags (def->set_flags, F_FONT))
		tail = String2FreeStorage (&MyStyleSyntax, tail, def->font, MYSTYLE_FONT_ID);

	if (get_flags (def->set_flags, F_FORECOLOR))
		tail = String2FreeStorage (&MyStyleSyntax, tail, def->fore_color, MYSTYLE_FORECOLOR_ID);

	if (get_flags (def->set_flags, F_BACKCOLOR))
		tail = String2FreeStorage (&MyStyleSyntax, tail, def->back_color, MYSTYLE_BACKCOLOR_ID);

	if (get_flags (def->set_flags, F_TEXTSTYLE))
        tail = Integer2FreeStorage (&MyStyleSyntax, tail, NULL, def->text_style, MYSTYLE_TEXTSTYLE_ID);

    if (get_flags (def->set_flags, F_BACKPIXMAP))

		for( i = 0 ; i < def->back_layers ; i++ )
        	tail = TextureDef2FreeStorage (&MyStyleSyntax, tail, &(def->back[i]), MYSTYLE_BACKPIXMAP_ID);
	if (get_flags (def->set_flags, F_DRAWTEXTBACKGROUND))
       	tail = Integer2FreeStorage (&MyStyleSyntax, tail, NULL, def->draw_text_background, MYSTYLE_DRAWTEXTBACKGROUND_ID);

	tail = Flag2FreeStorage (&MyStyleSyntax, tail, MYSTYLE_DONE_ID);
#endif

	return new_tail;
}

FreeStorageElem **
MyStyleDefs2FreeStorage (SyntaxDef * syntax, FreeStorageElem ** tail, MyStyleDefinition * defs)
{
	while (defs)
	{
		tail = MyStyleDef2FreeStorage (syntax, tail, defs);
		defs = defs->next;
	}
	return tail;
}


