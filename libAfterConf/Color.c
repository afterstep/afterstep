/*
 * Copyright (c) 2003 Sasha Vasko <sasha at aftercode.net>
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


#include "../configure.h"

#include "../libAfterStep/asapp.h"
#include "../libAfterStep/afterstep.h"
#include "../libAfterStep/parser.h"
#include "../libAfterStep/colorscheme.h"

#include "afterconf.h"


/*****************************************************************************
 *
 * This routine is responsible for reading and parsing the colors config
 * file
 *
 ****************************************************************************/
#define COLOR_TERM(n,len)  {TF_NO_MYNAME_PREPENDING, #n, len, TT_COLOR, COLOR_##n##_ID , NULL}

TermDef       ColorTerms[] = {
	COLOR_TERM(Base,4),
	COLOR_TERM(Inactive1,9),
	COLOR_TERM(Inactive2,9),
	COLOR_TERM(Active,6),
	COLOR_TERM(InactiveText1,13),
	COLOR_TERM(InactiveText2,13),
	COLOR_TERM(ActiveText,10),
	COLOR_TERM(HighInactive,12),
	COLOR_TERM(HighActive,10),
	COLOR_TERM(HighInactiveBack,16),
	COLOR_TERM(HighActiveBack,14),
	COLOR_TERM(HighInactiveText,16),
	COLOR_TERM(HighActiveText,14),
	COLOR_TERM(DisabledText,12),
	COLOR_TERM(BaseDark,8),
	COLOR_TERM(BaseLight,9),
	COLOR_TERM(Inactive1Dark,13),
	COLOR_TERM(Inactive1Light,14),
	COLOR_TERM(Inactive2Dark,13),
	COLOR_TERM(Inactive2Light,14),
	COLOR_TERM(ActiveDark,10),
	COLOR_TERM(ActiveLight,11),
	COLOR_TERM(HighInactiveDark,16),
	COLOR_TERM(HighInactiveLight,17),
	COLOR_TERM(HighActiveDark,14),
	COLOR_TERM(HighActiveLight,15),
	COLOR_TERM(HighInactiveBackDark,20),
	COLOR_TERM(HighInactiveBackLight,21),
	COLOR_TERM(HighActiveBackDark,24),
	COLOR_TERM(HighActiveBackLight,25),
	{TF_NO_MYNAME_PREPENDING, "Angle", 5, TT_UINTEGER, COLOR_Angle_ID, NULL},

	{0, NULL, 0, 0, 0}
};


SyntaxDef     ColorSyntax = {
	'\n',
	'\0',
    ColorTerms,
	0,										   /* use default hash size */
    ' ',
	"",
	"\t",
    "AfterStep custom Color Scheme",
	NULL,
	0
};

ColorConfig*
CreateColorConfig()
{
	return safecalloc( 1, sizeof(ColorConfig));
}

void
DestroyColorConfig(ColorConfig* config)
{
	if( config )
	{
		DestroyFreeStorage (&(config->more_stuff));
		free (config);
	}
}


void
PrintColorConfig (ColorConfig *config )
{
}

ColorConfig *
ParseColorOptions (const char *filename, char *myname)
{
	ConfigDef    *ConfigReader = InitConfigReader (myname, &ColorSyntax, CDT_Filename, (void *)filename,
												   NULL);
	ColorConfig   *config = CreateColorConfig ();
	FreeStorageElem *Storage = NULL, *pCurr;
	ConfigItem    item;

	if (!ConfigReader)
		return config;

	item.memory = NULL;
	PrintConfigReader (ConfigReader);
	ParseConfig (ConfigReader, &Storage);
	PrintFreeStorage (Storage);

	/* getting rid of all the crap first */
	StorageCleanUp (&Storage, &(config->more_stuff), CF_DISABLED_OPTION);

	for (pCurr = Storage; pCurr; pCurr = pCurr->next)
	{
		int index ;
		if (pCurr->term == NULL)
			continue;
		if (!ReadConfigItem (&item, pCurr))
			continue;
		index = pCurr->term->id - COLOR_ID_START ;
		if( index >= 0 && index < ASMC_MainColors )
		{
			if( parse_argb_color( item.data.string, &(config->main_colors[index]) )!= item.data.string )
				set_flags( config->set_main_colors, (0x01<<index) );
		}else if( pCurr->term->id == COLOR_Angle_ID )
		{
			config->angle = item.data.integer ;
			set_flags( config->set_main_colors, COLOR_Angle );
		}
		item.ok_to_free = 1;
	}
	ReadConfigItem (&item, NULL);

	DestroyConfig (ConfigReader);
	DestroyFreeStorage (&Storage);
	return config;
}

/* returns:
 *		0 on success
 *		1 if data is empty
 *		2 if ConfigWriter cannot be initialized
 *
 */
int
WriteColorOptions (const char *filename, char *myname, ColorConfig * config, unsigned long flags)
{
	ConfigDef    *ColorConfigWriter = NULL;
	FreeStorageElem *Storage = NULL, **tail = &Storage;

	char color_buffer[128] ;
	int i ;

	if (config == NULL)
		return 1;
	if ((ColorConfigWriter = InitConfigWriter (myname, &ColorSyntax, CDT_Filename, (void *)filename)) == NULL)
		return 2;

	CopyFreeStorage (&Storage, config->more_stuff);

	/* building free storage here */
	for( i = 0 ; i < ASMC_MainColors ; ++i )
		if( get_flags(config->set_main_colors, 0x01<<i ) )
		{
			ARGB32 c = config->main_colors[i] ;
			CARD32 a, r, g, b, h, s, v ;
			char *tmp[1] ;
			a = ARGB32_ALPHA8(c);
			r = ARGB32_RED16(c);
			g = ARGB32_GREEN16(c);
			b = ARGB32_BLUE16(c);
			h = rgb2hsv( r, g, b, &s, &v );
			h = hue162degrees(h);
			s = val162percent(s);
			v = val162percent(v);
			r = r>>8 ;
			g = g>>8 ;
			b = b>>8 ;
			tmp[0] = &(color_buffer[0]) ;
			sprintf( color_buffer, "#%2.2lX%2.2lX%2.2lX%2.2lX  # or ahsv(%ld,%ld,%ld,%ld) or argb(%ld,%ld,%ld,%ld)",
					 a, r, g, b, a, h, s, v, a, r, g, b );
			tail = Strings2FreeStorage ( &ColorSyntax, tail, &(tmp[0]), 1, COLOR_ID_START+i );
		}

	/* colorscheme angle */
	if( get_flags( config->set_main_colors, COLOR_Angle ) )
    	tail = Integer2FreeStorage (&ColorSyntax, tail, NULL, config->angle, COLOR_Angle_ID);

	/* writing config into the file */
	WriteConfig (ColorConfigWriter, &Storage, CDT_Filename, (void **)&filename, flags);
	DestroyConfig (ColorConfigWriter);

	if (Storage)
	{
		fprintf (stderr, "\n%s:Config Writing warning: Not all Free Storage discarded! Trying again...", myname);
		DestroyFreeStorage (&Storage);
		fprintf (stderr, (Storage != NULL) ? " failed." : " success.");
	}
	return 0;
}


