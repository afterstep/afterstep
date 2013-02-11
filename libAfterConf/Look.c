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

#define LOCAL_DEBUG

#include "../configure.h"
#include "../libAfterStep/asapp.h"
#include "../libAfterStep/afterstep.h"
#include "../libAfterStep/parser.h"
#include "../libAfterStep/hints.h"
#include "../libAfterStep/mystyle.h"
#include "../libAfterStep/screen.h"
#include "../libAfterStep/mylook.h"

#include "afterconf.h"


flag_options_xref LookFlagsXref[] = {
	{MenuMiniPixmaps, LOOK_MenuMiniPixmaps_ID, 0}
	,
	{SeparateButtonTitle, LOOK_SeparateButtonTitle_ID, 0}
	,
	{0, 0, 0}

};

/**********************************************************************/
/* The following code has been backported from as-devel and remains
 * undebugged for future use. Feel free to debug it :
 *                                                      Sasha Vasko.
 **********************************************************************/


/*****************  Create/Destroy DeskBackConfig     *****************/
DesktopConfig *
GetDesktopConfig (DesktopConfig ** plist, int desk)
{

	register DesktopConfig *curr = *plist;

	while (curr)
		if (curr->desk == desk)
			return curr;
		else
			curr = curr->next;

	curr = (DesktopConfig *) safecalloc (1, sizeof (DesktopConfig));

	curr->desk = desk;
	curr->next = *plist;
	*plist = curr;

	return curr;
}

void
PrintDesktopConfig (DesktopConfig * head)
{
	DesktopConfig *cur;

	for (cur = head; cur; cur = cur->next)
		fprintf (stderr, "Desk: %d, back[%s], layout[%s]\n", cur->desk, cur->back_name, cur->layout_name);
}

void
DestroyDesktopConfig (DesktopConfig ** head)
{
	if (head)
	{
		DesktopConfig *cur = *head, *next;

		while (cur)
		{
			if (cur->back_name)
				free (cur->back_name);
			if (cur->layout_name)
				free (cur->layout_name);

			next = cur->next;
			free (cur);
			cur = next;
		}
		*head = NULL;
	}
}

DesktopConfig *
ParseDesktopOptions (DesktopConfig ** plist, ConfigItem * item, int id)
{
	DesktopConfig *config = GetDesktopConfig (plist, item->index);

	item->ok_to_free = 0;
	switch (id)
	{
	 case DESK_CONFIG_BACK:
		 set_string (&(config->back_name), item->data.string);
		 break;
	 case DESK_CONFIG_LAYOUT:
		 set_string (&(config->layout_name), item->data.string);
		 break;
	 default:
		 item->ok_to_free = 1;
	}
	return config;
}


/********************************************************************/
/* Supported Hints processing code                                  */
/********************************************************************/
ASSupportedHints *
ProcessSupportedHints (FreeStorageElem * options)
{
	ASSupportedHints *list = NULL;

	if (options)
	{
		list = create_hints_list ();
		while (options)
		{
			enable_hints_support (list, options->term->id - HINTS_ID_START);
			options = options->next;
		}
	}
	return list;
}

static FreeStorageElem **
SupportedHints2FreeStorage (SyntaxDef * syntax, FreeStorageElem ** tail, ASSupportedHints * list)
{
	register int  i;
	FreeStorageElem **new_tail;
	HintsTypes   *hints;
	int           hints_num;

	if (list == NULL)
		return tail;

	new_tail = Flag2FreeStorage (syntax, tail, LOOK_SupportedHints_ID);
	if (new_tail == tail)
		return tail;

	/* we must not free hints after the following !!!!!!!!!! */
	if ((hints = supported_hints_types (list, &hints_num)) != NULL)
	{
		tail = &((*tail)->sub);
		for (i = 0; i < hints_num; i++)
			tail = Flag2FreeStorage (&SupportedHintsSyntax, tail, HINTS_ID_START + hints[i]);
	}
	return new_tail;
}

/********************************************************************/
/* Now look processing code 					    */
/********************************************************************/

LookConfig   *
CreateLookConfig ()
{
	LookConfig   *config = (LookConfig *) safecalloc (1, sizeof (LookConfig));

	return config;
}

void
DestroyLookConfig (LookConfig * config)
{
	register int  i;

	if (config->menu_arrow != NULL)
		free (config->menu_arrow);			   /* menu arrow */
	if (config->menu_pin_on != NULL)
		free (config->menu_pin_on);			   /* menu pin */
	if (config->menu_pin_off != NULL)
		free (config->menu_pin_off);

	for (i = 0; i < 2; i++)
		if (config->text_gradient[i] != NULL)
			free (config->text_gradient[i]);   /* title text */
	for (i = 0; i < MAX_BUTTONS; i++)
	{
		if (config->normal_buttons[i] != NULL)
		{
			destroy_asbutton (config->normal_buttons[i], False);
			config->normal_buttons[i] = NULL;
		}
		if (config->icon_buttons[i] != NULL)
		{
			destroy_asbutton (config->icon_buttons[i], False);
			config->icon_buttons[i] = NULL;
		}
	}
	for (i = 0; i < BACK_STYLES; i++)
		if (config->window_styles[i] != NULL)
			free (config->window_styles[i]);

	for (i = 0; i < MENU_BACK_STYLES; i++)
		if (config->menu_styles[i] != NULL)
			free (config->menu_styles[i]);

	Destroy_balloonConfig (config->title_balloon_conf);
	Destroy_balloonConfig (config->menu_balloon_conf);
	DestroyMyStyleDefinitions (&(config->style_defs));
	DestroyMyFrameDefinitions (&(config->frame_defs));
	DestroyFreeStorage (&(config->more_stuff));
	DestroyMyBackgroundConfig (&(config->back_defs));
	DestroyDesktopConfig (&(config->desk_configs));


	if (config->menu_frame)
		free (config->menu_frame);

	if (config->supported_hints)
		destroy_hints_list (&(config->supported_hints));

	free (config);
}


void
spread_back_param_array (int *trg, int *src, int count)
{
	register int  i = 0, k;

	if (trg == NULL || src == NULL || count == 0)
		return;
	for (k = 0; i < count && i < 3 && k < BACK_STYLES; i++)
		trg[k++] = src[i];
	for (k = BACK_STYLES; i < count && k < BACK_STYLES + MENU_BACK_STYLES; i++)
		trg[k++] = src[i];
}

void
merge_depreciated_options (LookConfig * config, FreeStorageElem * Storage)
{
	FreeStorageElem *pCurr;
	MyStyleDefinition **mystyles_list = &(config->style_defs);
	ConfigItem    item;
	register int  i;

	char         *button_title_styles[BACK_STYLES] = { ICON_STYLE_FOCUS,
		ICON_STYLE_STICKY,
		ICON_STYLE_UNFOCUS,
		NULL
	};
	char         *ButtonColors[2] = { NULL, NULL };
	char         *ButtonTextureColors[2] = { NULL, NULL };
	char         *ButtonPixmap = NULL;
	int           ButtonTextureType = -1;


#define SET_WINDOW		0
#define SET_MENU		1
#define SET_ICON		2
#define SET_NUM			3
	char         *Fonts[SET_NUM];

	char         *MenuColors[MENU_BACK_STYLES][2];
	char         *MenuTextureColors[MENU_BACK_STYLES][2];
	char         *MenuPixmaps[MENU_BACK_STYLES];

	char         *WinColors[BACK_STYLES][2];
	char         *WinTextureColors[BACK_STYLES][2];
	char         *WinPixmaps[BACK_STYLES];

	int           TitleTextStyle = -1;

	int           TextureTypes[BACK_STYLES + MENU_BACK_STYLES];
	int          *array = NULL;


#define CHANGED_WINDOW		(0x01<<0)
#define CHANGED_MENU		(0x01<<1)
#define CHANGED_ICON		(0x01<<2)
	unsigned long change_flags = 0;

	/* start initialization */
	memset (&(Fonts[0]), 0x00, sizeof (char *) * SET_NUM);

	memset (&(MenuColors[0]), 0x00, sizeof (char *) * MENU_BACK_STYLES * 2);
	memset (&(MenuTextureColors[0]), 0x00, sizeof (char *) * MENU_BACK_STYLES * 2);
	memset (&(MenuPixmaps[0]), 0x00, sizeof (char *) * MENU_BACK_STYLES);

	memset (&(WinColors[0]), 0x00, sizeof (char *) * BACK_STYLES * 2);
	memset (&(WinTextureColors[0]), 0x00, sizeof (char *) * BACK_STYLES * 2);
	memset (&(WinPixmaps[0]), 0x00, sizeof (char *) * BACK_STYLES);

	for (i = 0; i < BACK_STYLES + MENU_BACK_STYLES; i++)
		TextureTypes[i] = -1;

	item.memory = NULL;
	/* end initialization */

	for (pCurr = Storage; pCurr; pCurr = pCurr->next)
	{
		register int  id;

		if (pCurr->term == NULL)
			continue;
		id = pCurr->term->id;
		if (id < LOOK_DEPRECIATED_ID_START && id >= LOOK_DEPRECIATED_ID_END)
			continue;
		if (!ReadConfigItem (&item, pCurr))
			continue;

		switch (id)
		{									   /* fonts : */
		 case LOOK_Font_ID:
			 set_string_value (&Fonts[SET_MENU], item.data.string, &change_flags, CHANGED_MENU);
			 break;
		 case LOOK_WindowFont_ID:
			 set_string_value (&Fonts[SET_WINDOW], item.data.string, &change_flags, CHANGED_WINDOW);
			 break;
		 case LOOK_IconFont_ID:
			 set_string_value (&Fonts[SET_ICON], item.data.string, &change_flags, CHANGED_ICON);
			 break;
			 /* colors */
		 case LOOK_MTitleForeColor_ID:
			 set_string_value (&MenuColors[MENU_BACK_TITLE][0], item.data.string, &change_flags, CHANGED_MENU);
			 break;
		 case LOOK_MTitleBackColor_ID:
			 set_string_value (&MenuColors[MENU_BACK_TITLE][1], item.data.string, &change_flags, CHANGED_MENU);
			 break;
		 case LOOK_MenuForeColor_ID:
			 set_string_value (&MenuColors[MENU_BACK_ITEM][0], item.data.string, &change_flags, CHANGED_MENU);
			 break;
		 case LOOK_MenuBackColor_ID:
			 set_string_value (&MenuColors[MENU_BACK_ITEM][1], item.data.string, &change_flags, CHANGED_MENU);
			 break;
		 case LOOK_MenuHiForeColor_ID:
			 set_string_value (&MenuColors[MENU_BACK_HILITE][0], item.data.string, &change_flags, CHANGED_MENU);
			 break;
		 case LOOK_MenuHiBackColor_ID:
			 set_string_value (&MenuColors[MENU_BACK_HILITE][1], item.data.string, &change_flags, CHANGED_MENU);
			 break;
		 case LOOK_MenuStippleColor_ID:
			 set_string_value (&MenuColors[MENU_BACK_STIPPLE][0], item.data.string, &change_flags, CHANGED_MENU);
			 break;
		 case LOOK_StdForeColor_ID:
			 set_string_value (&WinColors[BACK_UNFOCUSED][0], item.data.string, &change_flags, CHANGED_WINDOW);
			 break;
		 case LOOK_StdBackColor_ID:
			 set_string_value (&WinColors[BACK_UNFOCUSED][1], item.data.string, &change_flags, CHANGED_WINDOW);
			 break;
		 case LOOK_StickyForeColor_ID:
			 set_string_value (&WinColors[BACK_STICKY][0], item.data.string, &change_flags, CHANGED_WINDOW);
			 break;
		 case LOOK_StickyBackColor_ID:
			 set_string_value (&WinColors[BACK_STICKY][1], item.data.string, &change_flags, CHANGED_WINDOW);
			 break;
		 case LOOK_HiForeColor_ID:
			 set_string_value (&WinColors[BACK_FOCUSED][0], item.data.string, &change_flags, CHANGED_WINDOW);
			 break;
		 case LOOK_HiBackColor_ID:
			 set_string_value (&WinColors[BACK_FOCUSED][1], item.data.string, &change_flags, CHANGED_WINDOW);
			 break;
		 case LOOK_TitleTextMode_ID:
			 TitleTextStyle = item.data.integer;
			 set_flags (change_flags, CHANGED_WINDOW);
			 break;

		 case LOOK_TextureTypes_ID:
			 array = &(TextureTypes[0]);	   /* will process it down below */
			 break;
		 case LOOK_TitleTextureColor_ID:	   /* title */
			 if (pCurr->argc >= 2)
			 {
				 set_string_value (&WinTextureColors[BACK_FOCUSED][0], item.data.string, &change_flags, CHANGED_WINDOW);
				 set_string_value (&WinTextureColors[BACK_FOCUSED][1], mystrdup (pCurr->argv[1]), &change_flags,
								   CHANGED_WINDOW);
			 }
			 break;
		 case LOOK_UTitleTextureColor_ID:	   /* unfoc tit */
			 if (pCurr->argc >= 2)
			 {
				 set_string_value (&WinTextureColors[BACK_UNFOCUSED][0], item.data.string, &change_flags,
								   CHANGED_WINDOW);
				 set_string_value (&WinTextureColors[BACK_UNFOCUSED][1], mystrdup (pCurr->argv[1]), &change_flags,
								   CHANGED_WINDOW);
			 }
			 break;
		 case LOOK_STitleTextureColor_ID:	   /* stic tit */
			 if (pCurr->argc >= 2)
			 {
				 set_string_value (&WinTextureColors[BACK_STICKY][0], item.data.string, &change_flags, CHANGED_WINDOW);
				 set_string_value (&WinTextureColors[BACK_STICKY][1], mystrdup (pCurr->argv[1]), &change_flags,
								   CHANGED_WINDOW);
			 }
			 break;
		 case LOOK_MTitleTextureColor_ID:	   /* menu title */
			 if (pCurr->argc >= 2)
			 {
				 set_string_value (&MenuTextureColors[MENU_BACK_TITLE][0], item.data.string, &change_flags,
								   CHANGED_MENU);
				 set_string_value (&MenuTextureColors[MENU_BACK_TITLE][1], mystrdup (pCurr->argv[1]), &change_flags,
								   CHANGED_MENU);
			 }
			 break;
		 case LOOK_MenuTextureColor_ID:	   /* menu items */
			 if (pCurr->argc >= 2)
			 {
				 set_string_value (&MenuTextureColors[MENU_BACK_ITEM][0], item.data.string, &change_flags,
								   CHANGED_MENU);
				 set_string_value (&MenuTextureColors[MENU_BACK_ITEM][1], mystrdup (pCurr->argv[1]), &change_flags,
								   CHANGED_MENU);
			 }
			 break;
		 case LOOK_MenuHiTextureColor_ID:	   /* sel items */
			 if (pCurr->argc >= 2)
			 {
				 set_string_value (&MenuTextureColors[MENU_BACK_HILITE][0], item.data.string, &change_flags,
								   CHANGED_MENU);
				 set_string_value (&MenuTextureColors[MENU_BACK_HILITE][1], mystrdup (pCurr->argv[1]), &change_flags,
								   CHANGED_MENU);
			 }
			 break;
		 case LOOK_MenuPixmap_ID:			   /* menu entry */
			 set_string_value (&MenuPixmaps[MENU_BACK_ITEM], item.data.string, &change_flags, CHANGED_MENU);
			 break;
		 case LOOK_MenuHiPixmap_ID:		   /* hil m entr */
			 set_string_value (&MenuPixmaps[MENU_BACK_HILITE], item.data.string, &change_flags, CHANGED_MENU);
			 break;
		 case LOOK_MTitlePixmap_ID:		   /* menu title */
			 set_string_value (&MenuPixmaps[MENU_BACK_TITLE], item.data.string, &change_flags, CHANGED_MENU);
			 break;
		 case LOOK_TitlePixmap_ID:			   /* foc tit */
			 set_string_value (&WinPixmaps[BACK_FOCUSED], item.data.string, &change_flags, CHANGED_WINDOW);
			 break;
		 case LOOK_UTitlePixmap_ID:		   /* unfoc tit */
			 set_string_value (&WinPixmaps[BACK_UNFOCUSED], item.data.string, &change_flags, CHANGED_WINDOW);
			 break;
		 case LOOK_STitlePixmap_ID:		   /* stick tit */
			 set_string_value (&WinPixmaps[BACK_STICKY], item.data.string, &change_flags, CHANGED_WINDOW);
			 break;

		 case LOOK_ButtonTextureType_ID:
			 ButtonTextureType = item.data.integer;
			 break;
		 case LOOK_ButtonBgColor_ID:
			 set_string_value (&ButtonColors[1], item.data.string, &change_flags, CHANGED_ICON);
			 break;
		 case LOOK_ButtonTextureColor_ID:
			 if (pCurr->argc >= 2)
			 {
				 set_string_value (&ButtonTextureColors[0], item.data.string, &change_flags, CHANGED_ICON);
				 set_string_value (&ButtonTextureColors[1], mystrdup (pCurr->argv[1]), &change_flags, CHANGED_ICON);
			 }
			 break;
		 case LOOK_ButtonPixmap_ID:
			 set_string_value (&ButtonPixmap, item.data.string, &change_flags, CHANGED_ICON);
			 break;
		 default:
			 item.ok_to_free = 1;
		}
		if (array != NULL)
		{
			if (item.data.int_array.size > BACK_STYLES + MENU_BACK_STYLES)
				item.data.int_array.size = BACK_STYLES + MENU_BACK_STYLES;
			spread_back_param_array (array, item.data.int_array.array, item.data.int_array.size);
			item.ok_to_free = 1;

			if (item.data.int_array.size > 0)
			{
				set_flags (change_flags, CHANGED_WINDOW);
				if (item.data.int_array.size > 3)
					set_flags (change_flags, CHANGED_MENU);
			}
			array = NULL;
		}
	}

	ReadConfigItem (&item, NULL);

	if (get_flags (change_flags, CHANGED_WINDOW))
		for (i = 0; i < BACK_STYLES; i++)
		{
			MergeMyStyleText (mystyles_list, config->window_styles[i], Fonts[SET_WINDOW], WinColors[i][0],
							  WinColors[i][1], TitleTextStyle);
			MergeMyStyleTextureOld (mystyles_list, config->window_styles[i], TextureTypes[i],
									WinTextureColors[i][0], WinTextureColors[i][0], WinPixmaps[i]);

			if (WinColors[i][0])
				free (WinColors[i][0]);
			if (WinColors[i][1])
				free (WinColors[i][1]);
			if (WinTextureColors[i][0])
				free (WinTextureColors[i][0]);
			if (WinTextureColors[i][1])
				free (WinTextureColors[i][1]);
			if (WinPixmaps[i])
				free (WinPixmaps[i]);
		}
	if (get_flags (change_flags, CHANGED_MENU))
		for (i = 0; i < MENU_BACK_STYLES; i++)
		{
			MergeMyStyleText (mystyles_list, config->menu_styles[i], Fonts[SET_MENU], MenuColors[i][0],
							  MenuColors[i][0], -1);
			MergeMyStyleTextureOld (mystyles_list, config->menu_styles[i], TextureTypes[i + BACK_STYLES],
									MenuTextureColors[i][0], MenuTextureColors[i][0], MenuPixmaps[i]);
			if (MenuColors[i][0])
				free (MenuColors[i][0]);
			if (MenuColors[i][1])
				free (MenuColors[i][1]);
			if (MenuTextureColors[i][0])
				free (MenuTextureColors[i][0]);
			if (MenuTextureColors[i][1])
				free (MenuTextureColors[i][1]);
			if (MenuPixmaps[i])
				free (MenuPixmaps[i]);
		}
	if (get_flags (change_flags, CHANGED_ICON))
	{
		for (i = 0; i < BACK_STYLES; i++)
		{
			MergeMyStyleText (mystyles_list, button_title_styles[i], Fonts[SET_ICON], ButtonColors[0], ButtonColors[1],
							  -1);
			MergeMyStyleTextureOld (mystyles_list, button_title_styles[i], ButtonTextureType,
									ButtonTextureColors[0], ButtonTextureColors[0], ButtonPixmap);
		}
		if (ButtonColors[0])
			free (ButtonColors[0]);
		if (ButtonColors[1])
			free (ButtonColors[1]);
		if (ButtonTextureColors[0])
			free (ButtonTextureColors[0]);
		if (ButtonTextureColors[1])
			free (ButtonTextureColors[1]);
		if (ButtonPixmap)
			free (ButtonPixmap);
	}

	for (i = 0; i < SET_NUM; i++)
		if (Fonts[i])
			free (Fonts[i]);

}


LookConfig   *
ParseLookOptions (const char *filename, char *myname)
{
	ConfigData    cd;
	ConfigDef    *ConfigReader;
	LookConfig   *config = CreateLookConfig ();
	FreeStorageElem *Storage = NULL, *pCurr;
	ConfigItem    item;
	MyFrameDefinition **frames_tail = &(config->frame_defs);
	MyBackgroundConfig **backs_tail = &(config->back_defs);

	cd.filename = filename;
	ConfigReader = InitConfigReader (myname, &LookSyntax, CDT_Filename, cd, NULL);
	if (!ConfigReader)
		return config;

	item.memory = NULL;
	PrintConfigReader (ConfigReader);
	ParseConfig (ConfigReader, &Storage);

	/* getting rid of all the crap first */
	StorageCleanUp (&Storage, &(config->more_stuff), CF_DISABLED_OPTION);
	config->title_balloon_conf = Process_balloonOptions (Storage, NULL, TITLE_BALLOON_ID_START);
	config->menu_balloon_conf = Process_balloonOptions (Storage, NULL, MENU_BALLOON_ID_START);

	config->style_defs = free_storage2MyStyleDefinitionsList (Storage);

	for (pCurr = Storage; pCurr; pCurr = pCurr->next)
	{
		register int  id;

		if (pCurr->term == NULL)
			continue;
		id = pCurr->term->id;
		if (id == BGR_MYBACKGROUND)
		{
			if ((*backs_tail = ParseMyBackgroundOptions (pCurr->sub, myname)) != NULL)
				backs_tail = &((*backs_tail)->next);
			continue;
		}

		if (id < LOOK_SUPPORTED_ID_START && id > LOOK_SUPPORTED_ID_END)
			continue;
		if (ReadFlagItem (&(config->set_flags), &(config->flags), pCurr, LookFlagsXref))
			continue;
		if (!ReadConfigItem (&item, pCurr))
			continue;
		if (id >= LOOK_WindowStyle_ID_START && id < LOOK_WindowStyle_ID_END)
		{
			id -= LOOK_WindowStyle_ID_START;
			set_string (&(config->window_styles[id]), item.data.string);
		} else if (id >= LOOK_MenuStyle_ID_START && id < LOOK_MenuStyle_ID_END)
		{
			id -= LOOK_MenuStyle_ID_START;
			set_string (&(config->menu_styles[id]), item.data.string);
		} else
		{
			switch (pCurr->term->id)
			{
			 case LOOK_IconBox_ID:
				 {
					 register int  bnum = config->icon_boxes_num++;

					 config->icon_boxes = saferealloc (config->icon_boxes, config->icon_boxes_num * sizeof (ASBox));
					 config->icon_boxes[bnum] = item.data.box;
				 }
				 break;
#define SET_ONOFF_FLAG(i,f,sf,v) if(i)set_flags((f),(v));else clear_flags((f),(v)); set_flags((sf),(v))
			 case LOOK_MArrowPixmap_ID:	   /* menu arrow */
				 set_string (&(config->menu_arrow), item.data.string);
				 break;
			 case LOOK_MenuPinOn_ID:		   /* menu pin */
				 set_string (&(config->menu_pin_on), item.data.string);
				 break;

			 case LOOK_TitleTextAlign_ID:
				 config->title_text_align = item.data.integer;
				 set_flags (config->set_flags, LOOK_TitleTextAlign);
				 break;
			 case LOOK_TitleButtonSpacing_ID:
				 config->title_button_spacing = item.data.integer;
				 set_flags (config->set_flags, LOOK_TitleButtonSpacing);
				 break;
			 case LOOK_TitleButtonStyle_ID:
				 config->title_button_style = item.data.integer;
				 set_flags (config->set_flags, LOOK_TitleButtonStyle);
				 break;
			 case LOOK_TitleButtonXOffset_ID:
				 config->title_button_x_offset = item.data.integer;
				 set_flags (config->set_flags, LOOK_TitleButtonXOffset);
				 break;
			 case LOOK_TitleButtonYOffset_ID:
				 config->title_button_y_offset = item.data.integer;
				 set_flags (config->set_flags, LOOK_TitleButtonYOffset);
				 break;
			 case LOOK_TitleButton_ID:
				 if (item.data.button != NULL)
				 {
					 if (config->normal_buttons[item.index])
						 destroy_asbutton (config->normal_buttons[item.index], False);
					 config->normal_buttons[item.index] = item.data.button;
				 } else
					 item.ok_to_free = 1;
				 break;
			 case LOOK_ResizeMoveGeometry_ID:
				 config->resize_move_geometry = item.data.geometry;
				 set_flags (config->set_flags, LOOK_ResizeMoveGeometry);
				 break;
			 case LOOK_StartMenuSortMode_ID:
				 config->start_menu_sort_mode = item.data.integer;
				 set_flags (config->set_flags, LOOK_StartMenuSortMode);
				 break;
			 case LOOK_DrawMenuBorders_ID:
				 config->draw_menu_borders = item.data.integer;
				 set_flags (config->set_flags, LOOK_DrawMenuBorders);
				 break;
			 case LOOK_ButtonSize_ID:
				 config->button_size = item.data.box;
				 set_flags (config->set_flags, LOOK_ButtonSize);
				 break;

			 case LOOK_RubberBand_ID:
				 config->rubber_band = item.data.integer;
				 set_flags (config->set_flags, LOOK_RubberBand);
				 break;
			 case LOOK_ShadeAnimationSteps_ID:
				 config->shade_animation_steps = item.data.integer;
				 set_flags (config->set_flags, LOOK_ShadeAnimationSteps);
				 break;

			 case MYFRAME_START_ID:
				 frames_tail = ProcessMyFrameOptions (pCurr->sub, frames_tail);
				 item.ok_to_free = 1;
				 break;

			 case LOOK_DeskBack_ID:
				 break;

			 case LOOK_SupportedHints_ID:
				 if (config->supported_hints)
					 destroy_hints_list (&(config->supported_hints));
				 config->supported_hints = ProcessSupportedHints (pCurr->sub);
				 item.ok_to_free = 1;
				 break;

			 default:
				 item.ok_to_free = 1;
			}
		}
	}
	if (config->icon_boxes_num >= MAX_BOXES)
		config->icon_boxes_num = MAX_BOXES;

	ReadConfigItem (&item, NULL);

	merge_depreciated_options (config, Storage);

	DestroyConfig (ConfigReader);
	DestroyFreeStorage (&Storage);
	return config;
}

/* returns:
 *            0 on success
 *              1 if data is empty
 *              2 if ConfigWriter cannot be initialized
 *
 */
int
WriteLookOptions (const char *filename, char *myname, LookConfig * config, unsigned long flags)
{
	ConfigData    cd;
	ConfigDef    *ConfigWriter = NULL;
	FreeStorageElem *Storage = NULL, **tail = &Storage;
	register int  i;

	if (config == NULL)
		return 1;
	cd.filename = filename;
	if ((ConfigWriter = InitConfigWriter (myname, &LookSyntax, CDT_Filename, cd)) == NULL)
		return 2;

	CopyFreeStorage (&Storage, config->more_stuff);

	/* building free storage here */
	/* MyStyles go first */
	if (config->style_defs)
		*tail = MyStyleDefinitionsList2free_storage (config->style_defs, &LookSyntax);

	/* then MyFrames */
	if (config->frame_defs)
		tail = MyFrameDefs2FreeStorage (&LookSyntax, tail, config->frame_defs);

	/* Window Styles */
	for (i = 0; i < BACK_STYLES; i++)
		if (config->window_styles[i])
			tail = QuotedString2FreeStorage (&LookSyntax, tail, NULL, config->window_styles[i],
											 LOOK_WindowStyle_ID_START + i);

	/* Menu Styles */
	for (i = 0; i < MENU_BACK_STYLES; i++)
		if (config->menu_styles[i])
			tail = QuotedString2FreeStorage (&LookSyntax, tail, NULL, config->menu_styles[i],
											 LOOK_MenuStyle_ID_START + i);

	/* Menu Icons */
	if (config->menu_arrow)
		tail = String2FreeStorage (&LookSyntax, tail, config->menu_arrow, LOOK_MArrowPixmap_ID);
	if (config->menu_pin_on)
		tail = String2FreeStorage (&LookSyntax, tail, config->menu_pin_on, LOOK_MenuPinOn_ID);
	/* Other Misc stuff */
	if (get_flags (config->set_flags, LOOK_StartMenuSortMode))
		tail = Integer2FreeStorage (&LookSyntax, tail, NULL, config->start_menu_sort_mode, LOOK_StartMenuSortMode_ID);
	if (get_flags (config->set_flags, LOOK_DrawMenuBorders))
		tail = Integer2FreeStorage (&LookSyntax, tail, NULL, config->draw_menu_borders, LOOK_DrawMenuBorders_ID);
	if (get_flags (config->set_flags, LOOK_RubberBand))
		tail = Integer2FreeStorage (&LookSyntax, tail, NULL, config->rubber_band, LOOK_RubberBand_ID);
	if (get_flags (config->set_flags, LOOK_ShadeAnimationSteps))
		tail =
			Integer2FreeStorage (&LookSyntax, tail, NULL, config->shade_animation_steps, LOOK_ShadeAnimationSteps_ID);
	/* flags : */
	tail = Flags2FreeStorage (&LookSyntax, tail, LookFlagsXref, config->set_flags, config->flags);

	/* ResizeMove window geometry */
	if (get_flags (config->set_flags, LOOK_ResizeMoveGeometry))
		tail = Geometry2FreeStorage (&LookSyntax, tail, &(config->resize_move_geometry), LOOK_ResizeMoveGeometry_ID);
	/* Icon boxes and size */
	for (i = 0; i < config->icon_boxes_num; i++)
		tail = Box2FreeStorage (&LookSyntax, tail, &(config->icon_boxes[i]), LOOK_IconBox_ID);
	if (get_flags (config->set_flags, LOOK_ButtonSize))
		tail = Box2FreeStorage (&LookSyntax, tail, &(config->button_size), LOOK_ButtonSize_ID);
	/* TitleButtons config: */
/*    if (config->balloon_conf)
        tail = balloon2FreeStorage (&LookSyntax, tail, config->balloon_conf); */

	if (get_flags (config->set_flags, LOOK_TitleTextAlign))
		tail = Integer2FreeStorage (&LookSyntax, tail, NULL, config->title_text_align, LOOK_TitleTextAlign_ID);
	if (get_flags (config->set_flags, LOOK_TitleButtonStyle))
		tail = Integer2FreeStorage (&LookSyntax, tail, NULL, config->title_button_style, LOOK_TitleButtonStyle_ID);
	if (get_flags (config->set_flags, LOOK_TitleButtonXOffset))
		tail = Integer2FreeStorage (&LookSyntax, tail, NULL, config->title_button_x_offset, LOOK_TitleButtonXOffset_ID);
	if (get_flags (config->set_flags, LOOK_TitleButtonYOffset))
		tail = Integer2FreeStorage (&LookSyntax, tail, NULL, config->title_button_y_offset, LOOK_TitleButtonYOffset_ID);
	if (get_flags (config->set_flags, LOOK_TitleButtonSpacing))
		tail = Integer2FreeStorage (&LookSyntax, tail, NULL, config->title_button_spacing, LOOK_TitleButtonSpacing_ID);

	/* then Supported Hints : */
	if (config->supported_hints)
		tail = SupportedHints2FreeStorage (&LookSyntax, tail, config->supported_hints);

	/* Normal Title Buttons : */
	for (i = 0; i < MAX_BUTTONS; i++)
		if (config->normal_buttons[i] != NULL)
			tail = ASButton2FreeStorage (&LookSyntax, tail, i, config->normal_buttons[i], LOOK_TitleButton_ID);

	/* writing config into the file */
	cd.filename = filename;
	WriteConfig (ConfigWriter, Storage, CDT_Filename, &cd, flags);
	DestroyFreeStorage (&Storage);
	DestroyConfig (ConfigWriter);

	if (Storage)
	{
		fprintf (stderr, "\n%s:Config Writing warning: Not all Free Storage discarded! Trying again...", myname);
		DestroyFreeStorage (&Storage);
		fprintf (stderr, (Storage != NULL) ? " failed." : " success.");
	}
	return 0;
}

#if 0
MyLook       *
LookConfig2MyLook (struct LookConfig * config, MyLook * look,
				   struct ASImageManager * imman, struct ASFontManager * fontman,
				   Bool free_resources, Bool do_init, unsigned long what_flags)
{
	register int  i;

	if (look)
	{
		if (look->magic != MAGIC_MYLOOK)
			look = mylook_create ();
		else if (do_init)
			mylook_init (look, free_resources, what_flags);
	} else
		look = mylook_create ();

	if (get_flags (what_flags, LL_Flags))
		look->flags = (look->flags & ~config->set_flags) | config->flags;

	if (get_flags (what_flags, LL_MyStyles))
	{
		MyStyleDefinition *pCurr;

		look->styles_list = mystyle_list_init ();
		for (pCurr = config->style_defs; pCurr; pCurr = pCurr->next)
			mystyle_list_create_from_definition (look->styles_list, pCurr, imman, fontman);

/*	DestroyMyStyleDefinitions (list); */
		if (config->style_defs != NULL)
			mystyle_list_fix_styles (look->styles_list, imman, fontman);

		if (get_flags (what_flags, LL_MSMenu))
		{									   /* menu MyStyles */
			for (i = 0; i < MENU_BACK_STYLES; i++)
				if (config->menu_styles[i] != NULL)
					look->MSMenu[i] = mystyle_list_new (look->styles_list, config->menu_styles[i]);
		}

		if (get_flags (what_flags, LL_MSWindow))
		{									   /* focussed, unfocussed, sticky window styles */
			for (i = 0; i < BACK_STYLES; i++)
				if (config->window_styles[i] != NULL)
					look->MSWindow[i] = mystyle_list_new (look->styles_list, config->window_styles[i]);
		}
	}

	if (get_flags (what_flags, LL_DeskBacks))
	{
		MyBackgroundConfig *pcurr;
		MyBackground *myback;

		init_deskbacks_list (look);
		for (pcurr = config->back_defs; pcurr; pcurr = pcurr->next)
			if ((myback = render_myback (look, pcurr, imman)) != NULL)
				add_myback_to_list (look->backs_list, myback, imman);
	}
	if (get_flags (what_flags, LL_DeskConfigs))
	{
		register DesktopConfig *pcurr;

		init_deskconfigs_list (look);

		for (pcurr = config->desk_configs; pcurr; pcurr = pcurr->next)
		{
			MyDesktopConfig *dc = safecalloc (1, sizeof (MyDesktopConfig));

			dc->desk = pcurr->desk;
			dc->back_name = pcurr->back_name;
			dc->layout_name = pcurr->layout_name;
			pcurr->back_name = pcurr->layout_name = NULL;

			add_deskconfig_to_list (look->desk_configs, dc);
		}
	}

	if (get_flags (what_flags, LL_MenuIcons))
	{
		if (config->menu_arrow)
			look->MenuArrow = filename2asicon (config->menu_arrow, imman);
		if (config->menu_pin_on)
			look->MenuPinOn = filename2asicon (config->menu_pin_on, imman);
		if (config->menu_pin_off)
			look->MenuPinOff = filename2asicon (config->menu_pin_off, imman);
	}

	if (get_flags (what_flags, LL_Buttons))
	{										   /* TODO: Titlebuttons - we 'll move them into  MyStyles */
		check_ascontextbuttons (&(look->normal_buttons), ASC_TitleButton_Start, ASC_TitleButton_End);
		check_ascontextbuttons (&(look->icon_buttons), ASC_TitleButton_Start, ASC_TitleButton_End);
		/* we will translate button numbers from the user input */
		for (i = 0; i <= ASC_TitleButton_End - ASC_TitleButton_Start; i++)
		{
			look->normal_buttons.buttons[i] = dup_asbutton (config->normal_buttons[i]);
			if (look->normal_buttons.buttons[i])
			{
				int           s;

				for (s = 0; s < ASB_StateCount; ++s)
					if (look->normal_buttons.buttons[i]->shapes[s])
						load_asicon (look->normal_buttons.buttons[i]->shapes[s], NULL, imman);

				look->buttons[i] = look->normal_buttons.buttons[i]->shapes[ASB_State_Up];
				look->dbuttons[i] = look->normal_buttons.buttons[i]->shapes[ASB_State_Down];
			} else
			{
				look->buttons[i] = NULL;
				look->dbuttons[i] = NULL;
			}

			look->icon_buttons.buttons[i] = dup_asbutton (config->icon_buttons[i]);
			if (look->icon_buttons.buttons[i])
			{
				int           s;

				for (s = 0; s < ASB_StateCount; ++s)
					if (look->icon_buttons.buttons[i]->shapes[s])
						load_asicon (look->icon_buttons.buttons[i]->shapes[s], NULL, imman);
			}
		}
	}

	if (get_flags (what_flags, LL_SizeWindow))
	{
		MyStyle      *focus_style = look->MSWindow[BACK_FOCUSED];
		int           width = -1, height = -1;

		if (focus_style)
		{
			int           max_digits = 1;
			char         *test_str;
			unsigned long test_val = (Scr.VxMax + Scr.MyDisplayWidth) * 3;

			if (test_val < (Scr.VyMax + Scr.MyDisplayHeight) * 3)
				test_val = (Scr.VyMax + Scr.MyDisplayHeight) * 3;
			while ((test_val / (10 * max_digits)) > 0)
				max_digits++;

			test_str = safemalloc (max_digits * 4 + 11);
			sprintf (test_str, " %lu x %lu %+ld %+ld ", test_val, test_val, test_val, test_val);
/*			mystyle_get_text_geometry(focus_style, test_str, strlen(test_str), &width, &height); */
			free (test_str);
		}

		if (get_flags (config->set_flags, LOOK_ResizeMoveGeometry))
			look->resize_move_geometry = config->resize_move_geometry;
		else
			look->resize_move_geometry.flags = 0;
		if (!get_flags (look->resize_move_geometry.flags, WidthValue) && width > 0)
		{
			look->resize_move_geometry.width = width;
			set_flags (look->resize_move_geometry.flags, WidthValue);
		}

		if (!get_flags (look->resize_move_geometry.flags, HeightValue) && height > 0)
		{
			look->resize_move_geometry.height = height;
			set_flags (look->resize_move_geometry.flags, HeightValue);
		}
	}

	if (get_flags (what_flags, LL_Layouts))
	{
		MyFrameDefinition *frame_def = config->frame_defs;

		look->TitleTextAlign = get_flags (config->set_flags, LOOK_TitleTextAlign) ? config->title_text_align : 0;	/* alignment of title bar text */
		look->TitleButtonSpacing =
			get_flags (config->set_flags, LOOK_TitleButtonSpacing) ? config->title_button_spacing : 3;
		look->TitleButtonStyle = get_flags (config->set_flags, LOOK_TitleButtonStyle) ? config->title_button_style : 0;	/* 0 - afterstep, 1 - WM old, 2 - free hand */
		look->TitleButtonXOffset =
			get_flags (config->set_flags, LOOK_TitleButtonXOffset) ? config->title_button_x_offset : 0;
		look->TitleButtonYOffset =
			get_flags (config->set_flags, LOOK_TitleButtonYOffset) ? config->title_button_y_offset : 0;
		look->TitleButtonStyle %= 3;
		switch (look->TitleButtonStyle)
		{
		 case 0:							   /* traditional AfterStep style */
			 look->TitleButtonXOffset[0] = look->TitleButtonXOffset[1] = 3;
			 look->TitleButtonYOffset[0] = look->TitleButtonYOffset[1] = 3;
			 break;
		 case 1:
			 look->TitleButtonXOffset[0] = look->TitleButtonXOffset[1] = 1;
			 look->TitleButtonYOffset[0] = look->TitleButtonYOffset[1] = 1;
			 break;
		 case 2:							   /* advanced AfterStep style */
			 /* nothing to do here - user must supply values for x/y offsets */
			 break;
		}
		mylook_init_layouts (look);

		while (frame_def != NULL)
		{
			add_frame_to_list (look->layouts, filenames2myframe (frame_def->name, frame_def->parts, imman));
			frame_def = frame_def->next;
		}
		if (config->menu_frame)
		{
			ASHashData    hdata = { 0 };
			if (get_hash_item (look->layouts, (ASHashableValue) config->menu_frame, &hdata.vptr) != ASH_Success)
				look->menu_frame = NULL;
			else
				look->menu_frame = hdata.vptr;
		}
	}

	if (get_flags (what_flags, LL_Buttons) && look->layouts)
	{
		update_titlebar_buttons (look->layouts, NULL, False, &(look->normal_buttons));
		update_titlebar_buttons (look->layouts, NULL, True, &(look->icon_buttons));
	}

	if (get_flags (what_flags, LL_Boundary))
		look->BoundaryWidth = BOUNDARY_WIDTH;  /* not a configuration option yet */

	if (get_flags (what_flags, LL_MenuParams))
	{
		look->DrawMenuBorders = get_flags (config->set_flags, LOOK_DrawMenuBorders) ? config->draw_menu_borders : 1;
		look->StartMenuSortMode =
			get_flags (config->set_flags, LOOK_StartMenuSortMode) ? config->start_menu_sort_mode : SORTBYALPHA;
	}

	if (get_flags (what_flags, LL_Icons))
	{
		for (i = 0; i < config->icon_boxes_num; i++)
		{
			look->IconBoxes[i] = config->icon_boxes[i];
			while (look->IconBoxes[i].left < 0)
				look->IconBoxes[i].left += Scr.MyDisplayWidth;
			if (look->IconBoxes[i].left == 0 && get_flags (look->IconBoxes[i].flags, LeftNegative))
				look->IconBoxes[i].left = Scr.MyDisplayWidth;

			while (look->IconBoxes[i].top < 0)
				look->IconBoxes[i].top += Scr.MyDisplayHeight;
			if (look->IconBoxes[i].top == 0 && get_flags (look->IconBoxes[i].flags, TopNegative))
				look->IconBoxes[i].top = Scr.MyDisplayHeight;

			while (look->IconBoxes[i].right < 0)
				look->IconBoxes[i].right += Scr.MyDisplayWidth;
			if (look->IconBoxes[i].right == 0 && get_flags (look->IconBoxes[i].flags, RightNegative))
				look->IconBoxes[i].right = Scr.MyDisplayWidth;

			while (look->IconBoxes[i].bottom < 0)
				look->IconBoxes[i].bottom += Scr.MyDisplayHeight;
			if (look->IconBoxes[i].bottom == 0 && get_flags (look->IconBoxes[i].flags, BottomNegative))
				look->IconBoxes[i].bottom = Scr.MyDisplayHeight;
		}
		look->NumBoxes = config->icon_boxes_num;
		if (get_flags (config->set_flags, LOOK_ButtonSize))
		{
			look->ButtonWidth = config->button_size.left;
			look->ButtonHeight = config->button_size.top;
		}
	}

	if (get_flags (what_flags, LL_Misc))
	{
		look->RubberBand = get_flags (config->set_flags, LOOK_RubberBand) ? config->rubber_band : 0;
		look->ShadeAnimationSteps =
			get_flags (config->set_flags, LOOK_ShadeAnimationSteps) ? config->shade_animation_steps : 12;
	}

	if (get_flags (what_flags, LL_Balloons))
	{
/*	
		look->balloon_look = balloon_look_new ();
		if (config->menu_balloon_conf)
			BalloonConfig2BalloonLook (look->menu_balloon_look, config->balloon_conf);
*/
	}

	if (get_flags (what_flags, LL_SupportedHints))
	{
		if (config->supported_hints)
		{
			look->supported_hints = config->supported_hints;
			config->supported_hints = NULL;
		} else
		{									   /* Otherwise we should enable all the hints possible in standard order : */
			look->supported_hints = create_hints_list ();
			enable_hints_support (look->supported_hints, HINTS_ICCCM);
			enable_hints_support (look->supported_hints, HINTS_Motif);
			enable_hints_support (look->supported_hints, HINTS_Gnome);
			enable_hints_support (look->supported_hints, HINTS_KDE);
			enable_hints_support (look->supported_hints, HINTS_ExtendedWM);
			enable_hints_support (look->supported_hints, HINTS_ASDatabase);
			enable_hints_support (look->supported_hints, HINTS_GroupLead);
			enable_hints_support (look->supported_hints, HINTS_Transient);
		}
	}

	return look;
}

#endif


/******************************************************************************
 * Test cases and utilities
 ******************************************************************************/
