/*
 * Copyright (C) 2000 Sasha Vasko <sasha at aftercode.net>
 *
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include "../configure.h"
 
#define LOCAL_DEBUG
#include "asapp.h"
#include "afterstep.h"
#include "asdatabase.h"
#include "screen.h"
#include "functions.h"
#include "clientprops.h"
#include "hints.h"


/*********************************************************************************
 * Raw Hints must be aggregated into usable forma :
 *********************************************************************************/
static void
init_ashints (ASHints * hints)
{
	if (hints)
	{										   /* some defaults to start with : */
		hints->flags = AS_Gravity | AS_AcceptsFocus | AS_Titlebar | AS_Handles | AS_Border;
        /* can't gracefully close the window if it does not support WM_DELETE_WINDOW */
        hints->function_mask = ~(AS_FuncClose|AS_FuncPinMenu);
        hints->gravity = NorthWestGravity;
		hints->border_width = 1;
	}
}

ASFlagType
get_asdb_hint_mask (ASDatabaseRecord * db_rec)
{
	ASFlagType    hint_mask = ASFLAGS_EVERYTHING;

	if (db_rec)
	{
		if (get_flags (db_rec->set_flags, STYLE_GROUP_HINTS) && !get_flags (db_rec->flags, STYLE_GROUP_HINTS))
			clear_flags (hint_mask, (0x01 << HINTS_GroupLead));
		if (get_flags (db_rec->set_flags, STYLE_TRANSIENT_HINTS) && !get_flags (db_rec->flags, STYLE_TRANSIENT_HINTS))
			clear_flags (hint_mask, (0x01 << HINTS_Transient));
		if (get_flags (db_rec->set_flags, STYLE_MOTIF_HINTS) && !get_flags (db_rec->flags, STYLE_MOTIF_HINTS))
			clear_flags (hint_mask, (0x01 << HINTS_Motif));
		if (get_flags (db_rec->set_flags, STYLE_GNOME_HINTS) && !get_flags (db_rec->flags, STYLE_GNOME_HINTS))
			clear_flags (hint_mask, (0x01 << HINTS_Gnome));
		if (get_flags (db_rec->set_flags, STYLE_EXTWM_HINTS) && !get_flags (db_rec->flags, STYLE_EXTWM_HINTS))
			clear_flags (hint_mask, (0x01 << HINTS_ExtendedWM));
		if (get_flags (db_rec->set_flags, STYLE_XRESOURCES_HINTS) && !get_flags (db_rec->flags, STYLE_XRESOURCES_HINTS))
			clear_flags (hint_mask, (0x01 << HINTS_XResources));
	}
	return hint_mask;
}

static        ASFlagType
get_afterstep_resources (XrmDatabase db, ASStatusHints * status)
{
	ASFlagType    found = 0;

	if (db != NULL && status)
	{
		if (read_int_resource (db, "afterstep*desk", "AS*Desk", &(status->desktop)))
			set_flags (found, AS_StartDesktop);
		if (read_int_resource (db, "afterstep*layer", "AS*Layer", &(status->layer)))
			set_flags (found, AS_StartLayer);
		if (read_int_resource (db, "afterstep*viewportx", "AS*ViewportX", &(status->viewport_x)))
			set_flags (found, AS_StartViewportX);
		if (read_int_resource (db, "afterstep*viewporty", "AS*ViewportY", &(status->viewport_y)))
			set_flags (found, AS_StartViewportY);
		set_flags (status->flags, found);
	}
	return found;
}

void
merge_command_line (ASHints * clean, ASStatusHints * status, ASRawHints * raw)
{
	static XrmOptionDescRec xrm_cmd_opts[] = {
		/* Want to accept -xrm "afterstep*desk:N" as options
		 * to specify the desktop. Have to include dummy options that
		 * are meaningless since Xrm seems to allow -x to match -xrm
		 * if there would be no ambiguity. */
		{"-xrnblahblah", NULL, XrmoptionResArg, (caddr_t) NULL},
		{"-xrm", NULL, XrmoptionResArg, (caddr_t) NULL},
	};

	if ( raw == NULL)
		return;
	if (raw->wm_cmd_argc > 0 && raw->wm_cmd_argv != NULL)
	{
		XrmDatabase   cmd_db = NULL;		   /* otherwise it will try to use value as if it was database */
		ASFlagType    found;

		init_xrm ();

		XrmParseCommand (&cmd_db, xrm_cmd_opts, 2, "afterstep", &(raw->wm_cmd_argc), raw->wm_cmd_argv);
		if (status != NULL )
		{
			found = get_afterstep_resources (cmd_db, status);

			if (!get_flags (found, AS_StartDesktop))
			{									   /* checking for CDE workspace specification */
				if (read_int_resource (cmd_db, "*workspaceList", "*WorkspaceList", &(status->desktop)))
					set_flags (status->flags, AS_StartDesktop);
			}
			XrmDestroyDatabase (cmd_db);
		}
		if (raw->wm_client_machine && clean)
			clean->client_host = text_property2string (raw->wm_client_machine);

		if (raw->wm_cmd_argc > 0 && clean)
		{
			int           i;
			int           len = 0;

			/* there still are some args left ! lets save it for future use : */
			/* first check to remove any geometry settings : */
			for (i = 0; i < raw->wm_cmd_argc; i++)
				if (raw->wm_cmd_argv[i])
				{
					if (i + 1 < raw->wm_cmd_argc)
						if (raw->wm_cmd_argv[i + 1] != NULL)
						{
							register char *g = raw->wm_cmd_argv[i + 1];

							if (isdigit ((int)*g) || ((*g == '-' || *g == '+') && isdigit ((int)*(g + 1))))
								if (mystrcasecmp (raw->wm_cmd_argv[i], "-g") == 0 ||
									mystrcasecmp (raw->wm_cmd_argv[i], "-geometry") == 0)
								{
									raw->wm_cmd_argv[i] = NULL;
									raw->wm_cmd_argv[++i] = NULL;
									continue;
								}
						}
					len += strlen (raw->wm_cmd_argv[i]) + 1;
				}
			if (len > 0)
			{
				register char *trg, *src;

				if (clean->client_cmd)
					free (clean->client_cmd);
				trg = clean->client_cmd = safecalloc (1, len+raw->wm_cmd_argc*2+1);
				for (i = 0; i < raw->wm_cmd_argc; i++)
					if ((src = raw->wm_cmd_argv[i]) != NULL)
					{
						register int k ;
						Bool add_quotes = False ;
						for (k = 0; src[k]; k++)
							if( isspace(src[k]) ) 
							{
								add_quotes = True ;
								break ;
							}
						
						if( add_quotes ) 
						{
							trg[0] = '"' ;
							++trg ;
						}
						for (k = 0; src[k]; k++)
							trg[k] = src[k];
						if( add_quotes ) 
						{
							trg[k] = '"' ;
							++k ;
						}
						trg[k] = ' ';
						trg += k + 1;
					}
				if (trg > clean->client_cmd)
					trg--;
				*trg = '\0';
			}
		}
	}
}

void
check_hints_sanity (ScreenInfo * scr, ASHints * clean)
{
	if (clean)
	{
		if (clean->border_width == 0 || clean->border_width > scr->MyDisplayHeight / 2)
			clear_flags (clean->flags, AS_Border);

		if (clean->handle_width > scr->MyDisplayHeight / 2)
			clean->handle_width = 0;

		if (get_flags (clean->flags, AS_SizeInc))
		{
			if (clean->width_inc == 0)
				clean->width_inc = 1;
			if (clean->height_inc == 0)
				clean->height_inc = 1;
		}

		if (get_flags (clean->flags, AS_Aspect))
			if ((clean->min_aspect.x == 0 || clean->min_aspect.y == 0) &&
				(clean->max_aspect.x == 0 || clean->max_aspect.y == 0))
				clear_flags (clean->flags, AS_Aspect);

		if (get_flags (clean->flags, AS_Icon))
			if (clean->icon_file == NULL && clean->icon.pixmap == None)
				clear_flags (clean->flags, AS_Icon);

		if (clean->frame_name == NULL)
			clear_flags (clean->flags, AS_Frame);
        if (clean->windowbox_name == NULL)
            clear_flags (clean->flags, AS_Windowbox);
    }
}

void
check_status_sanity (ScreenInfo * scr, ASStatusHints * status)
{
	if (status)
	{
		/* we want to limit user specifyed layers to AS_LayerDesktop < layer < AS_LayerMenu
		 * which is reasonable, since you cannot be lower then Desktop and higher then Menu
		 */
		if (get_flags (status->flags, AS_StartPositionUser))
			set_flags (status->flags, AS_StartPosition);

		if (status->layer <= AS_LayerDesktop)
			status->layer = AS_LayerDesktop + 1;
		else if (status->layer >= AS_LayerMenu)
			status->layer = AS_LayerMenu - 1;

		if (status->desktop == INVALID_DESK)
			clear_flags (status->flags, AS_StartDesktop);
		LOCAL_DEBUG_OUT( "viewport_x = %d", status->viewport_x );
		status->viewport_x = FIT_IN_RANGE (0, status->viewport_x, scr->VxMax);
		LOCAL_DEBUG_OUT( "viewport_x = %d", status->viewport_x );
		status->viewport_y = FIT_IN_RANGE (0, status->viewport_y, scr->VyMax);
		if (status->width < 2)
			status->width = 2;
		if (status->height < 2)
			status->height = 2;
		if (status->border_width == 0 || status->border_width > scr->MyDisplayHeight / 2)
			clear_flags (status->flags, AS_StartBorderWidth);
	}
}

/* Hints merging functions : */
ASHints      *
merge_hints (ASRawHints * raw, ASDatabase * db, ASStatusHints * status,
			 ASSupportedHints * list, ASFlagType what, ASHints * reusable_memory)
{
	ASHints      *clean = NULL;
	ASDatabaseRecord db_rec, *pdb_rec;
	int           i;
	ASFlagType    hints_types = ASFLAGS_EVERYTHING;

	if (raw == NULL || list == NULL || what == 0)
		return NULL;
	if ((clean = reusable_memory) == NULL)
		clean = (ASHints *) safecalloc (1, sizeof (ASHints));
	else
		memset (reusable_memory, 0x00, sizeof (ASHints));
	if (status)
		memset (status, 0x00, sizeof (ASStatusHints));

	init_ashints (clean);

	/* collect the names first */
	if (get_flags (what, HINT_NAME))
	{
		for (i = 0; i < list->hints_num; i++)
			(list->merge_funcs[i]) (clean, raw, NULL, status, HINT_NAME);
		
		if( clean->names_encoding[0] == AS_Text_ASCII )
		{/* TODO: we may want to recode the name into UTF8 string to 
		    simplify later drawing */
			
		
		}
		/* we want to make sure that we have Icon Name at all times, if at all possible */
		if (clean->icon_name == NULL)
			clean->icon_name = clean->names[0];
		what &= ~HINT_NAME;
	}
	if( clean->names[0] == NULL ) 
		clean->names[0] = mystrdup("");        /* must have at least one valid name string - even if empty */
	/* we don't want to do anything else if all that was requested are names */
	if (what == 0)
		return clean;

	pdb_rec = fill_asdb_record (db, clean->names, &db_rec, False);

	if( clean->matched_name0 ) 
		free(clean->matched_name0);
	clean->matched_name0 = mystrdup(clean->names[0]);
  	clean->matched_name0_encoding = clean->names_encoding[0];  


	LOCAL_DEBUG_OUT( "printing db record %p for names %p and db %p", pdb_rec, clean->names, db );
    if (is_output_level_under_threshold (OUTPUT_LEVEL_DATABASE))
		print_asdb_matched_rec (NULL, NULL, db, pdb_rec);

	hints_types = get_asdb_hint_mask (pdb_rec);
	hints_types &= raw->hints_types | (0x01 << HINTS_ASDatabase)| (0x01 << HINTS_XResources) ;

	/* now do the rest : */
	if (what != 0)
		for (i = 0; i < list->hints_num; i++)
		{									   /* only do work if needed */
			if (get_flags (hints_types, (0x01 << list->hints_types[i])))
			{
				(list->merge_funcs[i]) (clean, raw, pdb_rec, status, what);
				LOCAL_DEBUG_OUT ("merging hints %d (of %d ) - flags == 0x%lX", i, list->hints_num, clean->flags);
			}
		}
	if (get_flags (what, HINT_STARTUP) )
		merge_command_line (clean, status, raw);

	check_hints_sanity (raw->scr, clean);
	check_status_sanity (raw->scr, status);

	/* this is needed so if user changes the list of supported hints -
	 * we could track what was used, and if we need to remerge them
	 */
	clean->hints_types_raw = raw->hints_types;
	clean->hints_types_clean = get_flags (raw->hints_types, list->hints_flags);

	return clean;
}

/*
 * few function - shortcuts to implement update of selected hints :
 */
/* returns True if protocol/function hints actually changed :*/
Bool
update_protocols (ScreenInfo * scr, Window w, ASSupportedHints * list, ASFlagType * pprots, ASFlagType * pfuncs)
{
	ASRawHints    raw;
	ASHints       clean;
	Bool          changed = False;

	if (w == None)
		return False;

	if (collect_hints (scr, w, HINT_PROTOCOL, &raw) != NULL)
	{
		if (merge_hints (&raw, NULL, NULL, list, HINT_PROTOCOL, &clean) != NULL)
		{
			if (pprots)
				if ((changed = (*pprots != clean.protocols)))
					*pprots = clean.protocols;
			if (pfuncs)
				if ((changed = (*pfuncs != clean.function_mask)))
					*pfuncs = clean.function_mask;
			destroy_hints (&clean, True);
		}
		destroy_raw_hints (&raw, True);
	}
	return changed;
}

/* returns True if protocol/function hints actually changed :*/
Bool
update_colormaps (ScreenInfo * scr, Window w, ASSupportedHints * list, CARD32 ** pcmap_windows)
{
	ASRawHints    raw;
	ASHints       clean;
	Bool          changed = False;
	register CARD32 *old_win, *new_win;


	if (w == None || pcmap_windows == NULL)
		return False;

	if (collect_hints (scr, w, HINT_COLORMAP, &raw) != NULL)
	{
		if (merge_hints (&raw, NULL, NULL, list, HINT_COLORMAP, &clean) != NULL)
		{
			old_win = *pcmap_windows;
			new_win = clean.cmap_windows;
			changed = (old_win != new_win);
			if (new_win != NULL && old_win != NULL)
			{
				while (*old_win == *new_win && *old_win != None)
				{
					old_win++;
					new_win++;
				}
				changed = (*old_win != *new_win);
			}
			if (changed)
			{
				if (*pcmap_windows != NULL)
					free (*pcmap_windows);
				*pcmap_windows = clean.cmap_windows;
				clean.cmap_windows = NULL;
			}
			destroy_hints (&clean, True);
		}
		destroy_raw_hints (&raw, True);
	}
	return changed;
}

/****************************************************************************
 * The following functions actually implement hints merging :
 ****************************************************************************/
static int
add_name_to_list (char **list, char *name, unsigned char *encoding_list, unsigned char encoding )
{
	register int  i;

	if (name == NULL)
		return -1;

	for (i = 0; i < MAX_WINDOW_NAMES; i++)
	{
		if (list[i] == NULL)
			break;
		if (encoding_list[i] == encoding && strcmp (name, list[i]) == 0)
		{
			free (name);
			return i;
		}
	}
	if (i >= MAX_WINDOW_NAMES)
	{										   /* tough luck - no more space */
		free (name);
		return -1;
	}

	for (; i > 0; i--)
	{
		list[i] = list[i - 1];
		encoding_list[i] = encoding_list[i-1] ;
	}
	list[0] = name;
	encoding_list[0] = encoding ;
	return 0;
}

static int
pointer_name_to_index_in_list (char **list, char *name)
{
	register int  i;

	if (name)
		for (i = 0; i < MAX_WINDOW_NAMES; i++)
		{
			if (list[i] == NULL)
				break;
			if (name == list[i])
				return i;
		}
	return MAX_WINDOW_NAMES;
}


static void
merge_icccm_hints (ASHints * clean, ASRawHints * raw,
				   ASDatabaseRecord * db_rec, ASStatusHints * status, ASFlagType what)
{
	if (raw == NULL)
		return;
	if (get_flags (what, HINT_NAME))
	{										   /* adding all the possible window names in order of increasing importance */
		if (raw->wm_class)
		{
			if (raw->wm_class->res_class)
			{
				clean->res_class_idx = add_name_to_list (clean->names, stripcpy (raw->wm_class->res_class), clean->names_encoding, AS_Text_ASCII );
				clean->res_class = (clean->res_class_idx < 0 )?NULL:clean->names[clean->res_class_idx];
			}
			if (raw->wm_class->res_name)
			{
				clean->res_name_idx = add_name_to_list (clean->names, stripcpy (raw->wm_class->res_name), clean->names_encoding, AS_Text_ASCII);
				clean->res_name = (clean->res_name_idx < 0 )?NULL:clean->names[clean->res_name_idx];
			}
		}
		if (raw->wm_icon_name)
		{
			clean->icon_name_idx = add_name_to_list (clean->names, text_property2string (raw->wm_icon_name), clean->names_encoding, AS_Text_ASCII);
			clean->icon_name     = (clean->icon_name_idx < 0 )?NULL:clean->names[clean->icon_name_idx];
		}

		if (raw->wm_name)
			add_name_to_list (clean->names, text_property2string (raw->wm_name), clean->names_encoding, AS_Text_ASCII);
	}

	if (get_flags (what, HINT_STARTUP) && status != NULL)
	{
		Bool          position_user = False;

		if (raw->wm_hints)
			if (get_flags (raw->wm_hints->flags, StateHint))
			{
				if (raw->wm_hints->input == IconicState)
					set_flags (status->flags, AS_StartsIconic);
				else
					clear_flags (status->flags, AS_StartsIconic);
			}

		if (raw->wm_normal_hints)
			position_user = (get_flags (raw->wm_normal_hints->flags, USPosition));

		if (!get_flags (status->flags, AS_StartPositionUser) || position_user)
		{
			status->x = raw->placement.x;
			status->y = raw->placement.y;
			set_flags (status->flags, AS_StartPosition);
			if (position_user)
				set_flags (status->flags, AS_StartPositionUser);
		}
		status->width = raw->placement.width;
		status->height = raw->placement.height;
		set_flags (status->flags, AS_StartSize);
		status->border_width = raw->border_width;
		set_flags (status->flags, AS_StartBorderWidth);
		if (status->border_width > 0)
		{
			clean->border_width = status->border_width;
			set_flags (clean->flags, AS_Border);
		}

		if (raw->wm_state == IconicState)
			set_flags (status->flags, AS_StartsIconic);
		if (raw->wm_state_icon_win != None)
			status->icon_window = raw->wm_state_icon_win;
	}

	if (get_flags (what, HINT_GENERAL))
	{
		if (raw->wm_hints)
		{
			XWMHints     *wmh = raw->wm_hints;

			if (get_flags (wmh->flags, InputHint))
			{
				if (wmh->input)
					set_flags (clean->flags, AS_AcceptsFocus);
				else
					clear_flags (clean->flags, AS_AcceptsFocus);
			}

			if (get_flags (wmh->flags, IconWindowHint) && wmh->icon_window != None)
			{
				clean->icon.window = wmh->icon_window;
				set_flags (clean->flags, AS_ClientIcon);
				clear_flags (clean->flags, AS_ClientIconPixmap);

			} else if (get_flags (wmh->flags, IconPixmapHint) && wmh->icon_pixmap != None)
			{
				clean->icon.pixmap = wmh->icon_pixmap;
				if (get_flags (wmh->flags, IconMaskHint) && wmh->icon_mask != None)
					clean->icon_mask = wmh->icon_mask;
				set_flags (clean->flags, AS_ClientIcon);
				set_flags (clean->flags, AS_ClientIconPixmap);
			}

			if (get_flags (wmh->flags, IconPositionHint))
			{
				clean->icon_x = wmh->icon_x;
				clean->icon_y = wmh->icon_y;
				set_flags (clean->flags, AS_ClientIconPosition);
			}
		}

		if (raw->wm_normal_hints)
		{
			XSizeHints   *nh = raw->wm_normal_hints;

			if (get_flags (nh->flags, PMinSize))
			{
				clean->min_width = nh->min_width;
				clean->min_height = nh->min_height;
				set_flags (clean->flags, AS_MinSize);
			}
			if (get_flags (nh->flags, PMaxSize))
			{
				clean->max_width = nh->max_width;
				clean->max_height = nh->max_height;
				set_flags (clean->flags, AS_MaxSize);
			}
			if (get_flags (nh->flags, PResizeInc))
			{
				clean->width_inc = nh->width_inc;
				clean->height_inc = nh->height_inc;
				set_flags (clean->flags, AS_SizeInc);
			}
			if (get_flags (nh->flags, PAspect))
			{
				clean->min_aspect.x = nh->min_aspect.x;
				clean->min_aspect.y = nh->min_aspect.y;
				clean->max_aspect.x = nh->max_aspect.x;
				clean->max_aspect.y = nh->max_aspect.y;
				set_flags (clean->flags, AS_Aspect);
			}
			if (get_flags (nh->flags, PBaseSize))
			{
				clean->base_width = nh->base_width;
				clean->base_height = nh->base_height;
				set_flags (clean->flags, AS_BaseSize);
			}
			if (get_flags (nh->flags, PWinGravity))
			{
				clean->gravity = nh->win_gravity;
				set_flags (clean->flags, AS_Gravity);
			}
		}

	}

	if (get_flags (what, HINT_COLORMAP))
	{
		if (raw->wm_cmap_windows && raw->wm_cmap_win_count > 0)
		{
			register int  i;

			if (clean->cmap_windows)
				free (clean->cmap_windows);
			clean->cmap_windows = safecalloc (raw->wm_cmap_win_count + 1, sizeof (CARD32));
			for (i = 0; i < raw->wm_cmap_win_count; i++)
				clean->cmap_windows[i] = raw->wm_cmap_windows[i];
			clean->cmap_windows[i] = None;
		}
	}

	if (get_flags (what, HINT_PROTOCOL))
	{
		clean->protocols |= raw->wm_protocols;
		if (get_flags (clean->protocols, AS_DoesWmDeleteWindow))
			set_flags (clean->function_mask, AS_FuncClose);
	}
}

static void
merge_parent_hints (ASParentHints * hints, ASStatusHints * status)
{
	if (get_flags (hints->flags, AS_StartDesktop))
		status->desktop = hints->desktop;
	if (get_flags (hints->flags, AS_StartViewportX))
		status->viewport_x = hints->viewport_x;
	if (get_flags (hints->flags, AS_StartViewportY))
		status->viewport_y = hints->viewport_y;
	set_flags (status->flags, hints->flags);
}

static void
merge_group_hints (ASHints * clean, ASRawHints * raw,
				   ASDatabaseRecord * db_rec, ASStatusHints * status, ASFlagType what)
{
	if (raw == NULL)
		return;
	if (raw->group_leader)
	{
		if (get_flags (what, HINT_STARTUP) && status != NULL)
			merge_parent_hints (raw->group_leader, status);

		if (get_flags (what, HINT_GENERAL))
			clean->group_lead = raw->group_leader->parent;
	}
}

static void
merge_transient_hints (ASHints * clean, ASRawHints * raw,
					   ASDatabaseRecord * db_rec, ASStatusHints * status, ASFlagType what)
{
	if (raw == NULL)
		return;
	if (raw->transient_for)
	{
		if (get_flags (what, HINT_STARTUP) && status != NULL)
			merge_parent_hints (raw->transient_for, status);

		if (get_flags (what, HINT_GENERAL))
		{
			clean->transient_for = raw->transient_for->parent;
			set_flags (clean->flags, AS_Transient);
			set_flags (clean->flags, AS_ShortLived);
		}
	}
}

static void
decode_flags (ASFlagType * dst_flags, ASFlagsXref * xref, ASFlagType set_flags, ASFlagType flags)
{
	if (dst_flags == NULL || set_flags == 0)
		return;
	LOCAL_DEBUG_CALLER_OUT( "dst_flags = %lX, set_flags = %lX, flags = 0x%lX", *dst_flags, set_flags, flags );
	while ((*xref)[0] != 0)
	{
		ASFlagType    to_set;
		ASFlagType    to_clear;
		int           point;

		if (get_flags (set_flags, (*xref)[0]))
		{
			point = (get_flags (flags, (*xref)[0])) ? 1 : 3;
			to_set = (*xref)[point];
			to_clear = (*xref)[point + 1];
			if (to_set != 0)
				set_flags (*dst_flags, to_set);
			if (to_clear != 0)
				clear_flags (*dst_flags, to_clear);
		}
		xref++;
	}
}

static void
encode_flags (ASFlagType * dst_flags, ASFlagsXref * xref, ASFlagType set_flags, ASFlagType flags)
{
	if (dst_flags == NULL || set_flags == 0)
		return;

	while ((*xref)[0] != 0)
	{
		if (get_flags (set_flags, (*xref)[0]))
		{
			if (((flags & (*xref)[1]) == (*xref)[1]) && (flags & (*xref)[2]) == 0)
				set_flags (*dst_flags, (*xref)[0]);
			else if (((flags & (*xref)[3]) == (*xref)[3]) && (flags & (*xref)[4]) == 0)
				clear_flags (*dst_flags, (*xref)[0]);
		}
		xref++;
	}
}

#define decode_simple_flags(dst,xref,flags) decode_flags((dst),(xref),ASFLAGS_EVERYTHING,(flags))
#define encode_simple_flags(dst,xref,flags) encode_flags((dst),(xref),ASFLAGS_EVERYTHING,(flags))

void
check_motif_hints_sanity (MwmHints * motif_hints)
{

	if (get_flags (motif_hints->decorations, MWM_DECOR_ALL))
		motif_hints->decorations = MWM_DECOR_EVERYTHING & ~(motif_hints->decorations);

	/* Now I have the un-altered decor and functions, but with the
	   * ALL attribute cleared and interpreted. I need to modify the
	   * decorations that are affected by the functions */
	if( get_flags( motif_hints->flags, MWM_HINTS_FUNCTIONS )  )
	{
		if (get_flags (motif_hints->functions, MWM_FUNC_ALL))
			motif_hints->functions = MWM_FUNC_EVERYTHING & ~(motif_hints->functions);
		if (!get_flags (motif_hints->functions, MWM_FUNC_RESIZE))
			motif_hints->decorations &= ~MWM_DECOR_RESIZEH;
		/* MWM_FUNC_MOVE has no impact on decorations. */
		if (!get_flags (motif_hints->functions, MWM_FUNC_MINIMIZE))
			motif_hints->decorations &= ~MWM_DECOR_MINIMIZE;
		if (!get_flags (motif_hints->functions, MWM_FUNC_MAXIMIZE))
			motif_hints->decorations &= ~MWM_DECOR_MAXIMIZE;
		/* MWM_FUNC_CLOSE has no impact on decorations. */
	}
	/* This rule is implicit, but its easier to deal with if
	   * I take care of it now */
	if (motif_hints->decorations & (MWM_DECOR_MENU | MWM_DECOR_MINIMIZE | MWM_DECOR_MAXIMIZE))
		motif_hints->decorations |= MWM_DECOR_TITLE;
}

static ASFlagsXref mwm_decor_xref[] = {		   /*Flag              Set if Set    ,Clear if Set,Set if Clear, Clear if Clear  */
	{MWM_DECOR_BORDER, AS_Border, 0, 0, AS_Border | AS_Frame},
	{MWM_DECOR_RESIZEH, AS_Handles, 0, 0, AS_Handles},
	{MWM_DECOR_TITLE, AS_Titlebar, 0, 0, AS_Titlebar},
	{0, 0, 0, 0, 0}
};
static ASFlagsXref mwm_decor_func_xref[] = {   /*Flag              Set if Set,  Clear if Set, Set if Clear, Clear if Clear  */
	{MWM_DECOR_RESIZEH, AS_FuncResize, 0, 0, AS_Handles},
	{MWM_DECOR_MENU, AS_FuncPopup, 0, 0, AS_FuncPopup},
	{MWM_DECOR_MINIMIZE, AS_FuncMinimize, 0, 0, AS_FuncMinimize},
	{MWM_DECOR_MAXIMIZE, AS_FuncMaximize, 0, 0, AS_FuncMaximize},
	{0, 0, 0, 0, 0}
};
static ASFlagsXref mwm_func_xref[] = {		   /*Flag              Set if Set,  Clear if Set, Set if Clear, Clear if Clear  */
	{MWM_FUNC_RESIZE, AS_FuncResize, 0, 0, AS_FuncResize},
	{MWM_FUNC_MOVE, AS_FuncMove, 0, 0, AS_FuncMove},
	{MWM_FUNC_MINIMIZE, AS_FuncMinimize, 0, 0, AS_FuncMinimize},
	{MWM_FUNC_MAXIMIZE, AS_FuncMaximize, 0, 0, AS_FuncMaximize},
	{MWM_FUNC_CLOSE, AS_FuncClose | AS_FuncKill, 0, 0, AS_FuncClose | AS_FuncKill},
	{0, 0, 0, 0, 0}
};

static void
merge_motif_hints (ASHints * clean, ASRawHints * raw,
				   ASDatabaseRecord * db_rec, ASStatusHints * status, ASFlagType what)
{
	CARD32        decor = MWM_DECOR_EVERYTHING;
	CARD32        funcs = MWM_FUNC_EVERYTHING;

	if (raw == NULL)
		return;
	if (raw->motif_hints)
	{
		INT32         input_mode = 0;

		if (get_flags (raw->motif_hints->flags, MWM_HINTS_INPUT_MODE))
			input_mode = raw->motif_hints->inputMode;

		if (get_flags (what, HINT_STARTUP) && status && input_mode != 0)
		{
			if (input_mode == MWM_INPUT_SYSTEM_MODAL)
			{
				status->layer = AS_LayerUrgent;
				set_flags (status->flags, AS_StartLayer);
				set_flags (status->flags, AS_StartsSticky);
			}
			if (input_mode == MWM_INPUT_FULL_APPLICATION_MODAL)
			{
				status->layer = AS_LayerTop;
				set_flags (status->flags, AS_StartLayer);
			}
		}

		check_motif_hints_sanity (raw->motif_hints);

		if (get_flags (raw->motif_hints->flags, MWM_HINTS_FUNCTIONS))
			funcs = raw->motif_hints->functions;
		if (get_flags (raw->motif_hints->flags, MWM_HINTS_DECORATIONS))
			decor = raw->motif_hints->decorations;

		/* finally we can apply conglomerated hints to our flags : */
		if (get_flags (what, HINT_GENERAL))
		{
			decode_simple_flags (&(clean->flags), mwm_decor_xref, decor);
			LOCAL_DEBUG_OUT( "motif decor = 0x%lX, clean_flags = 0x%lX", decor, clean->flags );
		}
		if (get_flags (what, HINT_PROTOCOL))
		{
			decode_simple_flags (&(clean->function_mask), mwm_decor_func_xref, decor);
			decode_simple_flags (&(clean->function_mask), mwm_func_xref, funcs);
		}
	}
}

static ASFlagsXref gnome_state_xref[] = {	   /*Flag                    Set if Set,  Clear if Set, Set if Clear, Clear if Clear  */
	{WIN_STATE_STICKY, AS_StartsSticky, 0, 0, 0},
	{WIN_STATE_MINIMIZED, AS_StartsIconic, 0, 0, 0},
	{WIN_STATE_MAXIMIZED_VERT, AS_StartsMaximizedY, 0, 0, 0},
	{WIN_STATE_MAXIMIZED_HORIZ, AS_StartsMaximizedX, 0, 0, 0},
	{WIN_STATE_HIDDEN, 0, 0, 0, 0},
	{WIN_STATE_SHADED, AS_StartsShaded, 0, 0, 0},
	{WIN_STATE_HID_WORKSPACE, 0, 0, 0, 0},
	{WIN_STATE_HID_TRANSIENT, 0, 0, 0, 0},
	{WIN_STATE_FIXED_POSITION, 0, 0, 0, 0},
	{WIN_STATE_ARRANGE_IGNORE, 0, 0, 0, 0},
	{0, 0, 0, 0, 0}
};

static ASFlagsXref gnome_hints_xref[] = {	   /*Flag                    Set if Set,  Clear if Set, Set if Clear, Clear if Clear  */
	{WIN_HINTS_SKIP_FOCUS, 0, AS_AcceptsFocus, 0, 0},
	{WIN_HINTS_SKIP_WINLIST, AS_SkipWinList, 0, 0, 0},
	{WIN_HINTS_SKIP_TASKBAR, AS_SkipWinList, 0, 0, 0},
	{WIN_HINTS_GROUP_TRANSIENT, 0, 0, 0, 0},
	{WIN_HINTS_FOCUS_ON_CLICK, AS_ClickToFocus, 0, 0, 0},
	{0, 0, 0, 0, 0}
};

#define GNOME_AFFECTED_STATE  (AS_StartsSticky|AS_StartsIconic| \
		                       AS_StartsMaximizedY|AS_StartsMaximizedX| \
		   					   AS_StartsShaded)

Bool
decode_gnome_state (ASFlagType state, ASHints * clean, ASStatusHints * status)
{
	Bool          changed = False;
	ASFlagType    new_state = 0, disable_func = 0;

	decode_simple_flags (&new_state, gnome_state_xref, state);
	if ((new_state ^ (status->flags & GNOME_AFFECTED_STATE)) != 0)
	{
		changed = True;
		status->flags = new_state | (status->flags & ~GNOME_AFFECTED_STATE);
	}
	if (get_flags (new_state, WIN_STATE_FIXED_POSITION))
		disable_func = AS_FuncMove;

	if ((disable_func ^ (clean->function_mask & AS_FuncMove)) != 0)
	{
		changed = True;
		clean->function_mask = disable_func | (clean->function_mask & ~AS_FuncMove);
	}
	return changed;
}

static        ASFlagType
encode_gnome_state (ASHints * clean, ASStatusHints * status)
{
	ASFlagType    new_state = 0;

	if (status)
		encode_simple_flags (&new_state, gnome_state_xref, status->flags);

	if (get_flags (clean->function_mask, AS_FuncMove))
		set_flags (new_state, WIN_STATE_FIXED_POSITION);

	return new_state;
}

static void
merge_gnome_hints (ASHints * clean, ASRawHints * raw,
				   ASDatabaseRecord * db_rec, ASStatusHints * status, ASFlagType what)
{
	register GnomeHints *gh;

	if (raw == NULL)
		return;
	gh = &(raw->gnome_hints);
	if (gh->flags == 0)
		return;

	if (get_flags (what, HINT_STARTUP) && status != NULL)
	{
		if (get_flags (gh->flags, GNOME_LAYER))
		{
			status->layer = (gh->layer - WIN_LAYER_NORMAL) >> 1;
			set_flags (status->flags, AS_StartLayer);
		}
		if (get_flags (gh->flags, GNOME_WORKSPACE))
		{
			status->desktop = gh->workspace;
			set_flags (status->flags, AS_StartDesktop);
		}
		if (get_flags (gh->flags, GNOME_STATE) && gh->state != 0)
		{
			decode_gnome_state (gh->state, clean, status);
		}
	}

	if (get_flags (what, HINT_GENERAL))
	{
		if (get_flags (gh->flags, GNOME_HINTS) && gh->hints != 0)
		{
			decode_simple_flags (&(clean->flags), gnome_hints_xref, gh->hints);
		}
	}
}

static ASFlagsXref extwm_state_xref[] = {	   /*Flag                    Set if Set,  Clear if Set, Set if Clear, Clear if Clear  */
	{EXTWM_StateSticky, AS_StartsSticky, 0, 0, 0},
	{EXTWM_StateMaximizedV, AS_StartsMaximizedY, 0, 0, 0},
	{EXTWM_StateMaximizedH, AS_StartsMaximizedX, 0, 0, 0},
	{EXTWM_StateShaded, AS_StartsShaded, 0, 0, 0},
	{0, 0, 0, 0, 0}
};

#define EXTWM_AFFECTED_STATE  (AS_StartsSticky|AS_StartsMaximizedY| \
                               AS_StartsMaximizedX|AS_StartsShaded)

static ASFlagType extwm_types_start_properties[][3] = {
	{EXTWM_TypeDesktop, AS_LayerOtherDesktop, AS_StartsSticky},
	{EXTWM_TypeDock, AS_LayerService, AS_StartsSticky},
	{EXTWM_TypeToolbar, AS_LayerNormal, 0},
	{EXTWM_TypeMenu, AS_LayerUrgent, AS_StartsSticky},
	{EXTWM_TypeDialog, AS_LayerTop, 0},
	{EXTWM_TypeNormal, AS_LayerNormal, 0},  
	{EXTWM_TypeUtility, AS_LayerTop, 0}, 	
	{EXTWM_TypeSplash, AS_LayerTop, AS_ShortLived },
	{0, 0, 0}
};
static ASFlagsXref extwm_type_xref[] = {	   /*Flag              Set if Set,      Clear if Set,     Set if Clear,    Clear if Clear  */
	{EXTWM_TypeDesktop, AS_SkipWinList | AS_DontCirculate, AS_Titlebar | AS_Handles | AS_Frame, 0, 0},
	{EXTWM_TypeDock, AS_AvoidCover, AS_Handles | AS_Frame | AS_Titlebar, 0, 0},
	{EXTWM_TypeToolbar, 0, AS_Handles | AS_Frame , 0, 0},
	{EXTWM_TypeMenu, AS_SkipWinList | AS_DontCirculate, AS_Handles | AS_Frame, 0, 0},
	{EXTWM_TypeDialog, AS_ShortLived, 0 /*may need to remove handles */ , 0, 0},
	{0, 0, 0, 0, 0}
};
static ASFlagsXref extwm_type_func_mask[] = {  /*Flag             Set if Set,  Clear if Set,     Set if Clear  , Clear if Clear  */
	{EXTWM_TypeDesktop, 0, AS_FuncResize | AS_FuncMinimize | AS_FuncMaximize | AS_FuncMove, 0, 0},
	{EXTWM_TypeDock, 0, AS_FuncResize | AS_FuncMinimize | AS_FuncMaximize, 0, 0},
	{EXTWM_TypeToolbar, 0, AS_FuncResize, 0, 0},
	{EXTWM_TypeMenu, 0, AS_FuncResize | AS_FuncMinimize | AS_FuncMaximize, 0, 0},
	{0, 0, 0, 0, 0}
};

ASFlagType 
extwm_state2as_state_flags( ASFlagType extwm_flags )
{
	ASFlagType as_flags	= 0 ;
	decode_simple_flags (&as_flags, extwm_state_xref, extwm_flags);
	return as_flags;
}	   

static void
merge_extwm_hints (ASHints * clean, ASRawHints * raw,
				   ASDatabaseRecord * db_rec, ASStatusHints * status, ASFlagType what)
{

	register ExtendedWMHints *eh;

	if (raw == NULL)
		return;
	eh = &(raw->extwm_hints);

	if (get_flags (what, HINT_NAME))
	{
		if (eh->name)
			add_name_to_list (clean->names, text_property2string (eh->name), clean->names_encoding, AS_Text_UTF8);
		if (eh->icon_name)
			clean->icon_name_idx = add_name_to_list (clean->names, text_property2string (eh->icon_name), clean->names_encoding, AS_Text_UTF8);
		if (eh->visible_name)
			add_name_to_list (clean->names, text_property2string (eh->visible_name), clean->names_encoding, AS_Text_UTF8);
		if (eh->visible_icon_name)
			clean->icon_name_idx = add_name_to_list (clean->names, text_property2string (eh->visible_icon_name), clean->names_encoding, AS_Text_UTF8);
		clean->icon_name = (clean->icon_name_idx <0 )?NULL: clean->names[clean->icon_name_idx] ;
	}

	if (get_flags (what, HINT_STARTUP) && status != NULL)
	{
		if (get_flags (eh->flags, EXTWM_DESKTOP))
		{
			if (eh->desktop == 0xFFFFFFFF)
				set_flags (status->flags, AS_StartsSticky);
			else
			{
				status->desktop = eh->desktop;
				set_flags (status->flags, AS_StartDesktop);
			}
		}
		/* window state hints : */
		if (get_flags (eh->flags, EXTWM_StateEverything))
		{
			if (get_flags (eh->flags, EXTWM_StateModal))
			{
				status->layer = AS_LayerTop;
				set_flags (status->flags, AS_StartLayer);
			}
			decode_simple_flags (&(status->flags), extwm_state_xref, eh->flags);
		}
		/* window type hints : */
		if (get_flags (eh->flags, (EXTWM_TypeEverything & (~EXTWM_TypeNormal))))
		{
			register int  i;

			for (i = 0; extwm_types_start_properties[i][0] != 0; i++)
				if (get_flags (eh->flags, extwm_types_start_properties[i][0]))
				{
					status->layer = extwm_types_start_properties[i][1];
					set_flags (status->flags, AS_StartLayer);
					set_flags (status->flags, extwm_types_start_properties[i][2]);
				}
		}
	}
	if (get_flags (what, HINT_GENERAL))
	{
		if (get_flags (eh->flags, EXTWM_PID))
		{
			clean->pid = eh->pid;
			set_flags (clean->flags, AS_PID);
		}
        if (get_flags (eh->flags, EXTWM_StateSkipTaskbar))
            set_flags (clean->flags, AS_SkipWinList);

		if (get_flags (eh->flags, EXTWM_TypeEverything))
		{
			decode_simple_flags (&(clean->flags), extwm_type_xref, eh->flags);
		}
	}
	if (get_flags (what, HINT_PROTOCOL))
	{
		if (get_flags (eh->flags, EXTWM_DoesWMPing))
			set_flags (clean->protocols, AS_DoesWmPing);
		if (get_flags (eh->flags, EXTWM_NAME) && eh->name != NULL)
			set_flags (clean->protocols, AS_NeedsVisibleName);
		if (get_flags (eh->flags, EXTWM_VISIBLE_NAME) && eh->visible_name != NULL)
			set_flags (clean->protocols, AS_NeedsVisibleName);
		if (get_flags (eh->flags, EXTWM_TypeEverything))
			decode_simple_flags (&(clean->function_mask), extwm_type_func_mask, eh->flags);
	}
}

static void
merge_xresources_hints (ASHints * clean, ASRawHints * raw,
						ASDatabaseRecord * db_rec, ASStatusHints * status, ASFlagType what)
{
	XrmDatabase   db = NULL;
	extern XrmDatabase as_xrm_user_db;
	char         *rm_string;


	if (raw == NULL || status == NULL || !get_flags (what, HINT_STARTUP))
		return;

	init_xrm ();

	if ((rm_string = XResourceManagerString (dpy)) != NULL)
		db = XrmGetStringDatabase (rm_string);
	else
		db = as_xrm_user_db;

	if (db)
	{
		get_afterstep_resources (db, status);
		if (db != as_xrm_user_db)
			XrmDestroyDatabase (db);
	}
}

void
merge_asdb_hints (ASHints * clean, ASRawHints * raw, ASDatabaseRecord * db_rec, ASStatusHints * status, ASFlagType what)
{
	static ASFlagsXref asdb_startup_xref[] = { /*Flag                    Set if Set    ,Clear if Set    , Set if Clear, Clear if Clear  */
		{STYLE_STICKY, AS_StartsSticky, 0, 0, AS_StartsSticky},
		{STYLE_START_ICONIC, AS_StartsIconic, 0, 0, AS_StartsIconic},
		{STYLE_PPOSITION, AS_StartPositionUser, 0, 0, AS_StartPosition},
		{0, 0, 0, 0, 0}
	};
	static ASFlagsXref asdb_hints_xref[] = {   /*Flag                  Set if Set      ,Clear if Set    ,Set if Clear       ,Clear if Clear  */
		{STYLE_TITLE, AS_Titlebar, 0, 0, AS_Titlebar},
		{STYLE_CIRCULATE, 0, AS_DontCirculate, AS_DontCirculate, 0},
        {STYLE_WINLIST, 0, AS_SkipWinList, AS_SkipWinList, 0},
		{STYLE_ICON_TITLE, AS_IconTitle, 0, 0, AS_IconTitle},
		{STYLE_FOCUS, AS_AcceptsFocus, 0, 0, AS_AcceptsFocus},
		{STYLE_AVOID_COVER, AS_AvoidCover, 0, 0, AS_AvoidCover},
		{STYLE_VERTICAL_TITLE, AS_VerticalTitle, 0, 0, AS_VerticalTitle},
		{STYLE_HANDLES, AS_Handles, 0, 0, AS_Handles},
		{STYLE_FOCUS_ON_MAP, AS_FocusOnMap, 0, 0, AS_FocusOnMap},
		{STYLE_LONG_LIVING, 0, AS_ShortLived, AS_ShortLived, 0},
		{0, 0, 0, 0, 0}
	};

	LOCAL_DEBUG_CALLER_OUT ("0x%lX", what);

	if ( db_rec == NULL)
		return;
	if (get_flags (what, HINT_STARTUP) && status != NULL  )
	{
		if (get_flags (db_rec->set_data_flags, STYLE_STARTUP_DESK))
		{
			status->desktop = db_rec->desk;
			set_flags (status->flags, AS_StartDesktop);
		}
		if (get_flags (db_rec->set_data_flags, STYLE_VIEWPORTX))
		{
			status->viewport_x = db_rec->viewport_x;
			LOCAL_DEBUG_OUT( "viewport_x = %d", status->viewport_x );
			set_flags (status->flags, AS_StartViewportX);
		}
		if (get_flags (db_rec->set_data_flags, STYLE_VIEWPORTY))
		{
			status->viewport_y = db_rec->viewport_y;
			set_flags (status->flags, AS_StartViewportY);
		}
		if (get_flags (db_rec->set_data_flags, STYLE_LAYER))
		{
			status->layer = db_rec->layer;
			set_flags (status->flags, AS_StartLayer);
		}

		/*not exactly clean solution for the default geometry, but I don't see any other way : */
		if ( raw != NULL && get_flags (db_rec->set_data_flags, STYLE_DEFAULT_GEOMETRY) && !get_flags (status->flags, AS_StartPositionUser))
		{
			register ASGeometry *g = &(db_rec->default_geometry);

			if (get_flags (g->flags, WidthValue))
				status->width = g->width;
			else if (!get_flags (status->flags, AS_StartSize))	/* defaulting to the startup width */
				status->width = raw->placement.width;

			if (get_flags (g->flags, HeightValue))
				status->height = g->height;
			else if (!get_flags (status->flags, AS_StartSize))	/* defaulting to the startup height */
				status->height = raw->placement.height;

			set_flags (status->flags, AS_StartSize);

			if (get_flags (g->flags, XValue))
				status->x = get_flags (g->flags, XNegative) ?
					raw->scr->MyDisplayWidth - raw->border_width * 2 - status->width - g->x : g->x;
			else if (!get_flags (status->flags, AS_StartPosition))
				status->x = raw->placement.x;

			if (get_flags (g->flags, YValue))
				status->y = get_flags (g->flags, YNegative) ?
					raw->scr->MyDisplayHeight - raw->border_width * 2 - status->height - g->y : g->y;
			else if (!get_flags (status->flags, AS_StartPosition))
				status->y = raw->placement.y;

			if (get_flags (g->flags, YValue | XValue))
				set_flags (status->flags, AS_StartPosition | AS_StartPositionUser);
		}
		/* taking care of startup status flags : */
		decode_flags (&(status->flags), asdb_startup_xref, db_rec->set_flags, db_rec->flags);
	}
	if (get_flags (what, HINT_GENERAL))
	{
		if (get_flags (db_rec->set_data_flags, STYLE_ICON))
			set_string_value (&(clean->icon_file), mystrdup (db_rec->icon_file), &(clean->flags), AS_Icon);

		if (get_flags (db_rec->set_data_flags, STYLE_BORDER_WIDTH))
		{
			clean->border_width = db_rec->border_width;
			set_flags (clean->flags, AS_Border);
		}
		if (get_flags (db_rec->set_data_flags, STYLE_HANDLE_WIDTH))
		{
			clean->handle_width = db_rec->resize_width;
			set_flags (clean->flags, AS_Handles);
		}
		if (get_flags (db_rec->set_data_flags, STYLE_GRAVITY))
		{
			clean->gravity = db_rec->gravity;
			set_flags (clean->flags, AS_Gravity);
		}
		if (get_flags (db_rec->set_data_flags, STYLE_FRAME))
			set_string_value (&(clean->frame_name), mystrdup (db_rec->frame_name), &(clean->flags), AS_Frame);
        if (get_flags (db_rec->set_data_flags, STYLE_WINDOWBOX))
            set_string_value (&(clean->windowbox_name), mystrdup (db_rec->windowbox_name), &(clean->flags), AS_Windowbox);
        if (get_flags (db_rec->set_data_flags, STYLE_MYSTYLES))
		{
			register int  i;

			for (i = 0; i < BACK_STYLES; i++)
				if (db_rec->window_styles[i])
					set_string_value (&(clean->mystyle_names[i]), mystrdup (db_rec->window_styles[i]), NULL, 0);
		}
		/* taking care of flags : */
		decode_flags (&(clean->flags), asdb_hints_xref, db_rec->set_flags, db_rec->flags);

		clean->disabled_buttons = (~(db_rec->buttons)) & (db_rec->set_buttons);
	}
}

/* Don't forget to Cleanup after yourself : */
void
destroy_hints (ASHints * clean, Bool reusable)
{
	if (clean)
	{
		register int  i;

		for (i = 0; i < MAX_WINDOW_NAMES; i++)
			if (clean->names[i] == NULL)
				break;
			else
				free (clean->names[i]);

		if (clean->cmap_windows)
			free (clean->cmap_windows);
		if (clean->icon_file)
			free (clean->icon_file);
		if (clean->frame_name)
			free (clean->frame_name);
        if (clean->windowbox_name)
            free (clean->windowbox_name);
        for (i = 0; i < BACK_STYLES; i++)
			if (clean->mystyle_names[i])
				free (clean->mystyle_names[i]);

		if (clean->client_host)
			free (clean->client_host);
		if (clean->client_cmd)
			free (clean->client_cmd);

		if (reusable)						   /* we are being paranoid */
			memset (clean, 0x00, sizeof (ASHints));
		else
			free (clean);
	}
}

Bool
update_property_hints (Window w, Atom property, ASHints * hints, ASStatusHints * status)
{
	ASRawHints    raw;
	Bool          changed = False;

	if (status == NULL || hints == NULL)
		return False;
	memset (&raw, 0x00, sizeof (ASRawHints));
	raw.wm_state_icon_win = status->icon_window;
	raw.scr = &Scr;
	if (handle_client_property_update (w, property, &raw))
	{
		/* Here we are only interested in properties updtaed by the Window Manager : */
		if (property == _XA_WM_STATE)
		{
            unsigned long  new_state = (raw.wm_state == IconicState) ? AS_Iconic : 0;

			if ((changed = ((new_state ^ (status->flags & AS_Iconic)) != 0 ||
							raw.wm_state_icon_win != status->icon_window)))
			{
				status->icon_window = raw.wm_state_icon_win;
				status->flags = (status->flags & (~AS_Iconic)) | new_state;
			}
		} else if (property == _XA_WIN_LAYER)
		{
			if ((changed = (raw.gnome_hints.layer != status->layer)))
				status->layer = raw.gnome_hints.layer;
		} else if (property == _XA_WIN_STATE)
		{
			changed = decode_gnome_state (raw.gnome_hints.state, hints, status);
		} else if (property == _XA_WIN_WORKSPACE)
		{
			if ((changed = (raw.gnome_hints.workspace != status->desktop)))
				status->desktop = raw.gnome_hints.workspace;
		} else if (property == _XA_NET_WM_DESKTOP)	/* Extended WM-Hints : */
		{
			if ((changed = (raw.extwm_hints.desktop != status->desktop)))
				status->desktop = raw.extwm_hints.desktop;
		} else if (property == _XA_NET_WM_STATE)
		{
			ASFlagType    new_state = 0;

			decode_simple_flags (&new_state, extwm_state_xref, raw.extwm_hints.flags);
			if ((changed = (((status->flags & EXTWM_AFFECTED_STATE) ^ new_state) != 0)))
				status->flags = new_state | (status->flags & EXTWM_AFFECTED_STATE);
		}
		destroy_raw_hints (&raw, True);

	}
	return changed;
}

/* same as above only for window manager : */
void
update_cmd_line_hints (Window w, Atom property, 
					   ASHints * hints, ASStatusHints * status)
{
	ASRawHints    raw;

	memset (&raw, 0x00, sizeof (ASRawHints));
	raw.scr = &Scr;
	show_debug (__FILE__, __FUNCTION__, __LINE__, "trying to handle property change for WM_COMMAND");
	if (handle_manager_property_update (w, property, &raw))
	{
		merge_command_line (hints, status, &raw);
		destroy_raw_hints (&raw, True);		
	}
}

/* same as above only for window manager : */
Bool
update_property_hints_manager (Window w, Atom property, ASSupportedHints * list,
							   ASDatabase * db, ASHints * hints, ASStatusHints * status)
{
	ASRawHints    raw;
	Bool          changed = False;

	memset (&raw, 0x00, sizeof (ASRawHints));
	raw.scr = &Scr;
	if (status)
		raw.wm_state_icon_win = status->icon_window;
	show_debug (__FILE__, __FUNCTION__, __LINE__, "trying to handle property change");
	if (handle_manager_property_update (w, property, &raw))
	{
		ASHints       clean;

		show_debug (__FILE__, __FUNCTION__, __LINE__, "property update handled");
		if (property == _XA_WM_STATE)
		{
			if (status)
			{
                unsigned long  new_state = (raw.wm_state == IconicState) ? AS_Iconic : 0;

				if ((changed = ((new_state ^ (status->flags & AS_Iconic)) != 0 ||
								raw.wm_state_icon_win != status->icon_window)))
				{
					status->icon_window = raw.wm_state_icon_win;
					status->flags = (status->flags & (~AS_Iconic)) | new_state;
				}
			}
			return changed;
		}
		if (hints && merge_hints (&raw, db, NULL, list, HINT_ANY, &clean) != NULL)
		{

			show_debug (__FILE__, __FUNCTION__, __LINE__, "hints merged");
			if (property == XA_WM_NAME || property == XA_WM_ICON_NAME ||
				property == _XA_NET_WM_NAME || property == _XA_NET_WM_ICON_NAME ||
				property == _XA_NET_WM_VISIBLE_NAME || property == _XA_NET_WM_VISIBLE_ICON_NAME)
			{
				int           i;

				if( hints->names_encoding[0] == clean.names_encoding[0] && mystrcmp(hints->names[0], clean.names[0]) != 0 )
				    changed = True ;
				else if(  mystrcmp(hints->res_name, clean.res_name) != 0 )
				    changed = True ;
				else if( mystrcmp(hints->res_class, clean.res_class) != 0 )
				    changed = True ;
				else if( mystrcmp(hints->icon_name, clean.icon_name) != 0 )
				    changed = True ;

				for (i = 0; i < MAX_WINDOW_NAMES; ++i)
					if (hints->names[i] == NULL)
						break;
					else
						free (hints->names[i]);

				for (i = 0; i < MAX_WINDOW_NAMES; ++i)
				{
					hints->names_encoding[i] = clean.names_encoding[i] ;
					hints->names[i] = clean.names[i];
					clean.names[i] = NULL;
				}
				hints->res_name = clean.res_name;
				hints->res_name_idx = clean.res_name_idx;
				hints->res_class = clean.res_class;
				hints->res_class_idx = clean.res_class_idx;
				hints->icon_name = clean.icon_name;
				hints->icon_name_idx = clean.icon_name_idx;
				show_debug (__FILE__, __FUNCTION__, __LINE__, "names set");

				hints->flags = clean.flags ;
				hints->function_mask = clean.function_mask ;
			} else if (property == XA_WM_HINTS)
			{

			} else if (property == XA_WM_NORMAL_HINTS)
			{

			} else if (property == _XA_WM_PROTOCOLS)
			{

			} else if (property == _XA_WM_COLORMAP_WINDOWS)
			{
			}
		}
		destroy_hints (&clean, True);
		destroy_raw_hints (&raw, True);		
	} else
		show_debug (__FILE__, __FUNCTION__, __LINE__, "failed to handle property update");

	return changed;
}

static        Bool
compare_strings (char *str1, char *str2)
{
	if (str1 != NULL && str2 != NULL)
		return (strcmp (str1, str2) != 0);
	return (str1 != str2);
}

Bool
compare_names (ASHints * old, ASHints * hints)
{
	register int  i;

	if (old != NULL && hints != NULL)
	{
		for (i = 0; i < MAX_WINDOW_NAMES; i++)
			if (compare_strings (old->names[i], hints->names[i]))
				return True;
			else if (old->names[i] == NULL)
				break;
	} else if (old != hints)
		return True;

	return False;
}

ASFlagType
compare_hints (ASHints * old, ASHints * hints)
{
	ASFlagType    changed = 0;

	if (old != NULL && hints != NULL)
	{
		if (compare_strings (old->names[0], hints->names[0]))
			set_flags (changed, AS_HintChangeName);
		if (compare_strings (old->res_class, hints->res_class))
			set_flags (changed, AS_HintChangeClass);
		if (compare_strings (old->res_name, hints->res_name))
			set_flags (changed, AS_HintChangeResName);
		if (compare_strings (old->icon_name, hints->icon_name))
			set_flags (changed, AS_HintChangeIconName);
	} else if (old != hints)
		changed = AS_HintChangeEverything;
	return changed;
}

ASFlagType
function2mask (int function)
{
	static ASFlagType as_function_masks[F_MODULE_FUNC_START - F_WINDOW_FUNC_START] = {
		AS_FuncMove,						   /* F_MOVE,               30 */
		AS_FuncResize,						   /* F_RESIZE,             */
		0,									   /* F_RAISE,              */
		0,									   /* F_LOWER,              */
		0,									   /* F_RAISELOWER,         */
		0,									   /* F_PUTONTOP,           */
		0,									   /* F_PUTONBACK,          */
		0,									   /* F_SETLAYER,           */
		0,									   /* F_TOGGLELAYER,        */
		0,									   /* F_SHADE,              */
		AS_FuncKill,						   /* F_DELETE,             */
		0,									   /* F_DESTROY,            */
		AS_FuncClose,						   /* F_CLOSE,              */
		AS_FuncMinimize,					   /* F_ICONIFY,            */
		AS_FuncMaximize,					   /* F_MAXIMIZE,           */
		0,									   /* F_STICK,              */
		0,									   /* F_FOCUS,              */
        0,                                     /* F_CHANGEWINDOW_UP     */
        0,                                     /* F_CHANGEWINDOW_DOWN   */
        0,                                     /* F_GOTO_BOOKMARK     */
        0,                                     /* F_GETHELP,            */
		0,									   /* F_PASTE_SELECTION,    */
		0,									   /* F_CHANGE_WINDOWS_DESK, */
        0,                                     /* F_BOOKMARK_WINDOW,  */
        AS_FuncPinMenu                         /* F_PIN_MENU           */
	};

	if (function == F_POPUP)
		return AS_FuncPopup;
	if (function <= F_WINDOW_FUNC_START || (function) >= F_MODULE_FUNC_START)
		return 0;
	return as_function_masks[function - (F_WINDOW_FUNC_START + 1)];
}

/****************************************************************************
 * Initial placement/anchor management code
 ****************************************************************************/

void
constrain_size (ASHints * hints, ASStatusHints * status, unsigned int max_width, unsigned int max_height)
{
    int minWidth = 1, minHeight = 1;
    int xinc = 1, yinc = 1, delta;
    int baseWidth = 0, baseHeight = 0;
    int clean_width  = status->width -(status->frame_size[FR_W]+status->frame_size[FR_E]);
    int clean_height = status->height -(status->frame_size[FR_N]+status->frame_size[FR_S]);

	if (get_flags (hints->flags, AS_MinSize))
	{
		if (minWidth < hints->min_width)
			minWidth = hints->min_width;
		if (minHeight < hints->min_height)
			minHeight = hints->min_height;
	}else if( get_flags (hints->flags, AS_BaseSize))
	{
		if (minWidth < hints->base_width)
			minWidth = hints->base_width;
		if (minHeight < hints->base_height)
			minHeight = hints->base_height;
	}

	if (get_flags (hints->flags, AS_BaseSize))
	{
		baseWidth = hints->base_width;
		baseHeight = hints->base_height;
	}else if (get_flags (hints->flags, AS_MinSize))
	{
		baseWidth = minWidth ;
		baseHeight = minHeight ;
	}

	if (get_flags (hints->flags, AS_MaxSize))
	{
		if (max_width == 0 || max_width > hints->max_width)
			max_width = hints->max_width;
		if (max_height == 0 || max_height > hints->max_height)
			max_height = hints->max_height;
	} else
	{
		if (max_width == 0)
            max_width = MAX ((unsigned int)minWidth, clean_width);
		if (max_height == 0)
            max_height = MAX ((unsigned int)minHeight, clean_height);
	}
	LOCAL_DEBUG_OUT( "base_size = %dx%d, min_size = %dx%d, curr_size = %dx%d, max_size = %dx%d", baseWidth, baseHeight, minWidth, minHeight, clean_width, clean_height, max_width, max_height );
	/* First, clamp to min and max values  */
    clean_width = FIT_IN_RANGE (minWidth, clean_width, max_width);
    clean_height = FIT_IN_RANGE (minHeight, clean_height, max_height);
	LOCAL_DEBUG_OUT( "clumped_size = %dx%d", clean_width, clean_height );

	/* Second, fit to base + N * inc */
	if (get_flags (hints->flags, AS_SizeInc))
	{
		xinc = hints->width_inc;
		yinc = hints->height_inc;
        clean_width = (((clean_width - baseWidth) / xinc) * xinc) + baseWidth;
        clean_height = (((clean_height - baseHeight) / yinc) * yinc) + baseHeight;
		LOCAL_DEBUG_OUT( "inced_size = %dx%d", clean_width, clean_height );
	}

	/* Third, adjust for aspect ratio */
#define maxAspectX hints->max_aspect.x
#define maxAspectY hints->max_aspect.y
#define minAspectX hints->min_aspect.x
#define minAspectY hints->min_aspect.y
#define makemult(a,b) ((b==1) ? (a) : (((int)((a)/(b))) * (b)) )

	/* The math looks like this:

	 * minAspectX    dwidth     maxAspectX
	 * ---------- <= ------- <= ----------
	 * minAspectY    dheight    maxAspectY
	 *
	 * If that is multiplied out, then the width and height are
	 * invalid in the following situations:
	 *
	 * minAspectX * dheight > minAspectY * dwidth
	 * maxAspectX * dheight < maxAspectY * dwidth
	 *
	 */

	if (get_flags (hints->flags, AS_Aspect))
	{
        if ((minAspectX * clean_height > minAspectY * clean_width))
		{
            delta = makemult (minAspectX * clean_height / minAspectY - clean_width, xinc);
            if (clean_width + delta <= max_width)
                clean_width += delta;
			else
			{
                delta = makemult (clean_height - clean_width * minAspectY / minAspectX, yinc);
                if (clean_height - delta >= minHeight)
                    clean_height -= delta;
			}
		}
        if (maxAspectX * clean_height < maxAspectY * clean_width)
		{
            delta = makemult (clean_width * maxAspectY / maxAspectX - clean_height, yinc);
            if (clean_height + delta <= max_height)
                clean_height += delta;
			else
			{
                delta = makemult (clean_width - maxAspectX * clean_height / maxAspectY, xinc);
                if (clean_width - delta >= minWidth)
                    clean_width -= delta;
			}
		}
		LOCAL_DEBUG_OUT( "aspected_size = %dx%d", clean_width, clean_height );
	}
    status->width = clean_width + status->frame_size[FR_W]+status->frame_size[FR_E] ;
    status->height = clean_height + status->frame_size[FR_N]+status->frame_size[FR_S] ;
}

static int    _as_gravity_offsets[11][2] = {
	{0, 0},									   /* ForgetGravity */
	{-1, -1},								   /* NorthWestGravity */
	{2, -1},								   /* NorthGravity */
	{1, -1},								   /* NorthEastGravity */
	{-1, 2},								   /* WestGravity */
	{0, 0},									   /* CenterGravity */
	{1, 2},									   /* EastGravity */
	{-1, 1},								   /* SouthWestGravity */
	{2, 1},									   /* SouthGravity */
	{1, 1},									   /* SouthEastGravity */
	{2, 2}									   /* StaticGravity */
};

void
get_gravity_offsets (ASHints * hints, int *xp, int *yp)
{
	register int  g = NorthWestGravity;

	if (get_flags (hints->flags, AS_Gravity))
		g = hints->gravity;

	if (g < ForgetGravity || g > StaticGravity)
		*xp = *yp = 0;
	else
	{
		*xp = (int)_as_gravity_offsets[g][0];
		*yp = (int)_as_gravity_offsets[g][1];
	}
}

int
translate_asgeometry (ScreenInfo * scr, ASGeometry * asg, int *px, int *py, unsigned int *pwidth, unsigned int *pheight)
{
	int           grav = NorthWestGravity;
	unsigned int  width = 1, height = 1;

	if (scr == NULL)
		scr = &Scr;

	if (asg)
	{
		if (get_flags (asg->flags, XNegative))
		{
			if (get_flags (asg->flags, YNegative))
				grav = SouthEastGravity;
			else
				grav = NorthEastGravity;
		} else if (get_flags (asg->flags, YNegative))
			grav = SouthWestGravity;
	}
	if (get_flags (asg->flags, WidthValue))
	{
		width = asg->width;
		if (pwidth)
			*pwidth = width;
	} else if (pwidth)
		width = *pwidth;

	if (get_flags (asg->flags, HeightValue))
	{
		height = asg->height;
		if (pheight)
			*pheight = height;
	} else if (pheight)
		height = *pheight;

	if (get_flags (asg->flags, XValue) && px)
	{
		if (get_flags (asg->flags, XNegative))
		{
			if (asg->x <= 0)
				*px = scr->MyDisplayWidth + asg->x;
			else
				*px = scr->MyDisplayWidth - asg->x;
			*px -= width;
		} else
			*px = asg->x;
	}
	if (get_flags (asg->flags, YValue) && py)
	{
		if (get_flags (asg->flags, YNegative))
		{
			if (asg->x <= 0)
				*py = scr->MyDisplayHeight + asg->y;
			else
				*py = scr->MyDisplayHeight - asg->y;
			*py -= height;
		} else
			*py = asg->y;
	}

	return grav;
}

int
make_anchor_pos (ASStatusHints * status, int pos, unsigned int size, int vpos, int grav, int max_pos)
{											   /* anchor position is always in virtual coordinates */
	unsigned int  bw = 0;

	if (get_flags (status->flags, AS_StartBorderWidth))
		bw = status->border_width;

	/* position of the sticky window is stored in real coordinates */
	if (!get_flags (status->flags, AS_Sticky))
		pos += vpos;

	/* trying to place window partly on screen, unless user really wants it */
	if (!get_flags (status->flags, AS_StartPositionUser))
	{
		if (pos > max_pos)
			pos = max_pos;
		else if (pos + size < 16)
			pos = 16 - size;
	}

	switch (grav)
	{
	 case 0:								   /* Center */
		 pos += bw + (size >> 1);
		 break;
	 case 1:								   /* East */
		 pos += bw + size + bw;
		 break;
	 case 2:								   /* Static */
		 pos += bw;
		 break;
	 default:								   /* West */
		 break;
	}
	return pos;
}

/* reverse transformation */
int
make_status_pos (ASStatusHints * status, int pos, unsigned int size, int vpos, int grav)
{											   /* status position is always in real coordinates */
	unsigned int  bw = 0;

	if (get_flags (status->flags, AS_StartBorderWidth))
		bw = status->border_width;

	/* position of the sticky window is stored in real coordinates */
	if (!get_flags (status->flags, AS_Sticky))
		pos -= vpos;

	switch (grav)
	{
	 case 0:								   /* Center */
		 pos -= bw + (size >> 1);
		 break;
	 case 1:								   /* East */
		 pos -= bw + size + bw;
		 break;
	 case 2:								   /* Static */
		 pos -= bw;
		 break;
	 default:								   /* West */
		 break;
	}
	return pos;
}

void
make_detach_pos (ASHints * hints, ASStatusHints * status, XRectangle *anchor, int *detach_x, int *detach_y)
{
    unsigned int  bw = 0;
    int x = 0, y = 0 ;
    int grav_x, grav_y ;

    if (hints == NULL || status == NULL || anchor == NULL )
        return ;

	if (get_flags (status->flags, AS_StartBorderWidth))
		bw = status->border_width;

    get_gravity_offsets (hints, &grav_x, &grav_y);

    /* position of the sticky window is stored in real coordinates */
    x = anchor->x ;
    y = anchor->y ;
	if (!get_flags (status->flags, AS_Sticky))
    {
        x -= status->viewport_x;
        y -= status->viewport_y;
    }

    if( detach_x )
    {
        APPLY_GRAVITY (grav_x, x, anchor->width, bw, bw);
        *detach_x = x ;
    }
    if( detach_y )
    {
        APPLY_GRAVITY (grav_y, y, anchor->height, bw, bw);
        *detach_y = y ;
    }
}

/***** New Code : *****************************************************/

/*
 * frame_size[] must be set to the real frame decoration size if window
 * is being moved by us or to border_width if moved by client.
 *
 * width and height must be that of a client
 *
 * x, y are of top left corner of the client.
 * 		North  		Central 		South 			Static
 * y	-FR_N       +height/2       +height+FR_S    +0
 *
 * 		West   		Central 		East  			Static
 * x	-FR_W       +width/2       +width+FR_E      +0
 */

void
status2anchor (XRectangle * anchor, ASHints * hints, ASStatusHints * status, unsigned int vwidth, unsigned int vheight)
{
	if (get_flags (status->flags, AS_Size))
	{
		constrain_size (hints, status, vwidth, vheight);
        anchor->width = status->width - (status->frame_size[FR_W]+status->frame_size[FR_E]);
        anchor->height = status->height - (status->frame_size[FR_N]+status->frame_size[FR_S]);
	}

	if (get_flags (status->flags, AS_Position))
	{
		int           grav_x = -1, grav_y = -1;
        int           offset;

		get_gravity_offsets (hints, &grav_x, &grav_y);

		LOCAL_DEBUG_OUT( "grav_x = %d, width = %d, bw1 = %d, bw2 = %d, status_x = %d", 
						 grav_x, anchor->width, status->frame_size[FR_W], status->frame_size[FR_E], status->x );
        offset = 0 ;
		APPLY_GRAVITY (grav_x, offset, anchor->width, status->frame_size[FR_W], status->frame_size[FR_E]);
		anchor->x = status->x - offset;

		LOCAL_DEBUG_OUT( "grav_y = %d, height = %d, bw1 = %d, bw2 = %d, status_y = %d", 
						 grav_y, anchor->height, status->frame_size[FR_N], status->frame_size[FR_S], status->y );

        offset = 0;
		APPLY_GRAVITY (grav_y, offset, anchor->height, status->frame_size[FR_N], status->frame_size[FR_S]);
		anchor->y = status->y - offset;

		LOCAL_DEBUG_OUT( "anchor = %+d%+d", anchor->x, anchor->y );
		if (!get_flags (status->flags, AS_Sticky))
		{
			anchor->x += status->viewport_x;
			anchor->y += status->viewport_y;
		}
		LOCAL_DEBUG_OUT( "anchor = %+d%+d", anchor->x, anchor->y );
	}
}

void
anchor2status (ASStatusHints * status, ASHints * hints, XRectangle * anchor)
{
	int           grav_x = -1, grav_y = -1;
    int           offset;

    status->width = anchor->width+status->frame_size[FR_W]+status->frame_size[FR_E];
    status->height = anchor->height+status->frame_size[FR_N]+status->frame_size[FR_S];
	set_flags (status->flags, AS_Size);

	get_gravity_offsets (hints, &grav_x, &grav_y);

	LOCAL_DEBUG_OUT( "grav_x = %d, width = %d, bw1 = %d, bw2 = %d, anchor_x = %d", 
						 grav_x, anchor->width, status->frame_size[FR_W], status->frame_size[FR_E], anchor->x );
    offset = 0 ;
    APPLY_GRAVITY (grav_x, offset, anchor->width, status->frame_size[FR_W], status->frame_size[FR_E]);
	status->x = anchor->x + offset;

	LOCAL_DEBUG_OUT( "grav_y = %d, height = %d, bw1 = %d, bw2 = %d, anchor_y = %d", 
						 grav_y, anchor->height, status->frame_size[FR_N], status->frame_size[FR_S], anchor->y );

    offset = 0;
	APPLY_GRAVITY (grav_y, offset, anchor->height, status->frame_size[FR_N], status->frame_size[FR_S]);
	status->y = anchor->y + offset;

	LOCAL_DEBUG_OUT( "status = %+d%+d", status->x, status->y );
	if (!get_flags (status->flags, AS_Sticky))
	{
		status->x -= status->viewport_x;
		status->y -= status->viewport_y;
	}
	LOCAL_DEBUG_OUT( "status = %+d%+d", status->x, status->y );
	set_flags (status->flags, AS_Position);
}


/***** Old Code : *****************************************************/

ASFlagType
change_placement (ScreenInfo * scr, ASHints * hints, ASStatusHints * status, XPoint * anchor,
				  ASStatusHints * new_status, int vx, int vy, ASFlagType what)
{
	ASFlagType    todo = 0;
	int           grav_x, grav_y;
	register int  max_x = scr->MyDisplayWidth - 16;
	register int  max_y = scr->MyDisplayHeight - 16;
	int           new_x, new_y;

	if (!get_flags (what, AS_Size | AS_Position) || status == NULL || new_status == NULL)
		return 0;

	new_x = status->x - (vx - status->viewport_x);
	new_y = status->y - (vy - status->viewport_y);
	get_gravity_offsets (hints, &grav_x, &grav_y);
	if (!get_flags (status->flags, AS_Sticky))
	{
		max_x += scr->VxMax;
		max_y += scr->VyMax;
	}

	if (get_flags (what, AS_Size))
	{
		constrain_size (hints, new_status, scr->VxMax + scr->MyDisplayWidth, scr->VyMax + scr->MyDisplayHeight);
		if (new_status->width != status->width)
		{
			status->width = new_status->width;
			set_flags (todo, TODO_RESIZE_X);
			if (grav_x == 0 || grav_x == 1)
				set_flags (todo, TODO_MOVE_X);
		}
		if (new_status->height != status->height)
		{
			status->height = new_status->height;
			set_flags (todo, TODO_RESIZE_Y);
			if (grav_y == 0 || grav_y == 1)
				set_flags (todo, TODO_MOVE_Y);
		}
	}

	if (!get_flags (what, AS_BorderWidth) && new_status->border_width != status->border_width)
	{
		status->border_width = new_status->border_width;
		set_flags (todo, TODO_MOVE);
	}

	if (!get_flags (status->flags, AS_Sticky) && (status->viewport_x != vx || status->viewport_y != vy))
		set_flags (todo, TODO_MOVE);

	if (get_flags (what, AS_Position))
	{
		if (status->x != new_status->x)
		{
			new_x = new_status->x;
			set_flags (todo, TODO_MOVE_X);
		} else if (make_anchor_pos (status, new_x, status->width, vx, max_x, grav_x) != anchor->x)
			set_flags (todo, TODO_MOVE_X);

		if (status->y != new_status->y)
		{
			new_y = new_status->y;
			set_flags (todo, TODO_MOVE_Y);
		} else if (make_anchor_pos (status, new_y, status->height, vy, max_y, grav_y) != anchor->y)
			set_flags (todo, TODO_MOVE_Y);
	}
	if (get_flags (todo, TODO_MOVE_X))
	{
		anchor->x = make_anchor_pos (status, new_x, status->width, vx, max_x, grav_x);
		status->x = make_status_pos (status, anchor->x, status->width, vx, grav_x);
	}
	if (get_flags (todo, TODO_MOVE_Y))
	{
		anchor->y = make_anchor_pos (status, new_y, status->height, vy, max_y, grav_y);
		status->y = make_status_pos (status, anchor->y, status->height, vy, grav_y);
	}
	if (!get_flags (status->flags, AS_Sticky))
	{
		status->viewport_x = vx;
		status->viewport_y = vy;
	}

	return what;
}

int
calculate_viewport (int *pos, unsigned int size, unsigned int scr_vpos, unsigned int scr_size, int max_viewport)
{
	int           viewport = -1;

	if (*pos >= scr_size)
		viewport = *pos / scr_size;
	else if (*pos + size < 0)
	{
		if (*pos + scr_vpos > 0)
			viewport = (scr_vpos + *pos) / scr_size;
		else
			viewport = 0;
	} else
		return scr_vpos;

	viewport *= scr_size;
	viewport = MIN (viewport, max_viewport);
	*pos += scr_vpos - viewport;
	return viewport;
}

int
gravitate_position (int pos, unsigned int size, unsigned int scr_size, int grav, unsigned int bw)
{
	if (pos < 0 || pos + size > scr_size)
		return pos;
	if (grav == 1)							   /* East or South gravity */
		pos = (int)scr_size - (int)(pos + bw + size + bw);

	return pos;
}

/***********************************************************************************
 * we build a command line here, so we can restart an app with exactly the same
 * parameters:
 ***********************************************************************************/
char         *
make_client_geometry_string (ScreenInfo * scr, ASHints * hints, ASStatusHints * status, XRectangle * anchor, int vx, int vy, char **pure_geometry)
{
	char         *geom = NULL;
	int           detach_x, detach_y;
	int           grav_x, grav_y;
	int           bw = 0;
	int width, height, unit_width, unit_height ;

	if (hints == NULL || status == NULL || anchor == NULL)
		return NULL;

	if (get_flags (status->flags, AS_StartBorderWidth))
		bw = status->border_width;

    make_detach_pos (hints, status, anchor, &detach_x, &detach_y);

    vx = calculate_viewport (&detach_x, anchor->width, vx, scr->MyDisplayWidth, scr->VxMax);
    vy = calculate_viewport (&detach_y, anchor->height, vy, scr->MyDisplayHeight, scr->VyMax);

	get_gravity_offsets (hints, &grav_x, &grav_y);

    detach_x = gravitate_position (detach_x, anchor->width, bw, scr->MyDisplayWidth, grav_x);
    detach_y = gravitate_position (detach_y, anchor->height, bw, scr->MyDisplayHeight, grav_y);

    width = anchor->width ;
    height = anchor->height ;

    unit_width  = ( hints->width_inc  > 0 )?(width - hints->base_width  ) / hints->width_inc  : width  ;
	unit_height = ( hints->height_inc > 0 )?(height - hints->base_height) / hints->height_inc : height ;

	if( pure_geometry )
	{
		*pure_geometry = safemalloc (15+1+15+1+15+1+15+1 /* large enough */ );
		sprintf ( *pure_geometry, "%ux%u%+d%+d ", width, height, detach_x, detach_y);
	}
	geom = safemalloc (15+1+15+1+15+1+15+1 /* large enough */ );
	sprintf (geom, "%ux%u%+d%+d ", unit_width, unit_height, detach_x, detach_y);
	return geom;
}

char         *
make_client_command (ScreenInfo * scr, ASHints * hints, ASStatusHints * status, XRectangle * anchor, int vx, int vy)
{
	char         *client_cmd = NULL;
	char 		 *geom = make_client_geometry_string ( scr, hints, status, anchor, vx, vy, NULL );

	if (hints->client_cmd == NULL || geom == NULL )
		return NULL;

	/* supplying everything as : -xrm "afterstep*desk:N" */
	client_cmd = safemalloc (strlen (hints->client_cmd) + 11 + strlen(geom) + 1 + 1 );
	sprintf (client_cmd, "%s -geometry %s ", hints->client_cmd, geom );
    /*, status->desktop, status->layer, status->viewport_x, status->viewport_y*/
	return client_cmd;
}

/***********************************************************************
 * Setrting Hints on the window :
 ***********************************************************************/
static        Bool
client_hints2wm_hints (XWMHints * wm_hints, ASHints * hints, ASStatusHints * status)
{
	memset (wm_hints, 0x00, sizeof (XWMHints));

	if (status)
	{
		wm_hints->flags = StateHint;
		if (get_flags (status->flags, AS_StartsIconic))
			wm_hints->initial_state = IconicState;
		else
			wm_hints->initial_state = WithdrawnState;
	}
	/* does this application rely on the window manager to get keyboard input? */
	if (get_flags (hints->flags, AS_AcceptsFocus))
	{
		set_flags (wm_hints->flags, InputHint);
		wm_hints->input = True;
	}

	/* window to be used as icon */
	if (get_flags (hints->flags, AS_ClientIcon))
	{
		if (!get_flags (hints->flags, AS_ClientIconPixmap))
		{
			wm_hints->icon_window = hints->icon.window;
			set_flags (wm_hints->flags, IconWindowHint);
		} else
		{									   /* pixmap to be used as icon */
			set_flags (wm_hints->flags, IconPixmapHint);
			wm_hints->icon_pixmap = hints->icon.pixmap;

			if (hints->icon_mask)
			{								   /* pixmap to be used as mask for icon_pixmap */
				set_flags (wm_hints->flags, IconMaskHint);
				wm_hints->icon_mask = hints->icon_mask;
			}
		}
	}

	/* initial position of icon */
	if (get_flags (hints->flags, AS_ClientIconPosition))
	{
		set_flags (wm_hints->flags, IconPositionHint);
		wm_hints->icon_x = hints->icon_x;
		wm_hints->icon_y = hints->icon_y;
	}

	if (hints->group_lead)
	{
		set_flags (wm_hints->flags, WindowGroupHint);
		wm_hints->window_group = hints->group_lead;
	}

/*	if( get_flags( hints->flags, AS_AvoidCover ) )
		set_flags( wm_hints->flags, UrgencyHint );
 */
	return (wm_hints->flags != StateHint || wm_hints->initial_state == IconicState);
}

static        Bool
client_hints2size_hints (XSizeHints * size_hints, ASHints * hints, ASStatusHints * status)
{
	memset (size_hints, 0x00, sizeof (XSizeHints));
	if (status)
	{
		if (get_flags (status->flags, AS_StartPosition | AS_StartPositionUser))
		{
			if (get_flags (status->flags, AS_StartPositionUser))
				set_flags (size_hints->flags, USPosition);
			else
				set_flags (size_hints->flags, PPosition);
		}

		if (get_flags (status->flags, AS_StartSize | AS_StartSizeUser))
		{
			if (get_flags (status->flags, AS_StartSizeUser))
				set_flags (size_hints->flags, USSize);
			else
				set_flags (size_hints->flags, PSize);
		}
	}
	if (get_flags (hints->flags, AS_MinSize))
	{
		size_hints->min_width = hints->min_width;
		size_hints->min_height = hints->min_height;
		set_flags (size_hints->flags, PMinSize);
	}
	if (get_flags (hints->flags, AS_MaxSize))
	{
		size_hints->max_width = hints->max_width;
		size_hints->max_height = hints->max_height;
		set_flags (size_hints->flags, PMaxSize);
	}
	if (get_flags (hints->flags, AS_SizeInc))
	{
		size_hints->width_inc = hints->width_inc;
		size_hints->height_inc = hints->height_inc;
		set_flags (size_hints->flags, PResizeInc);
	}
	if (get_flags (hints->flags, AS_Aspect))
	{
		size_hints->min_aspect.x = hints->min_aspect.x;
		size_hints->min_aspect.y = hints->min_aspect.y;
		size_hints->max_aspect.x = hints->max_aspect.x;
		size_hints->max_aspect.y = hints->max_aspect.y;
		set_flags (size_hints->flags, PAspect);
	}
	if (get_flags (hints->flags, AS_BaseSize))
	{
		size_hints->base_width = hints->base_width;
		size_hints->base_height = hints->base_height;
		set_flags (size_hints->flags, PBaseSize);
	}
	if (get_flags (hints->flags, AS_Gravity))
	{
		size_hints->win_gravity = hints->gravity;
		set_flags (size_hints->flags, PWinGravity);
	}
	return (size_hints->flags != 0);
}

static        Bool
client_hints2wm_protocols (ASFlagType * protocols, ASHints * hints)
{
	if (protocols == NULL || hints == NULL)
		return False;

	*protocols = hints->protocols & (AS_DoesWmDeleteWindow | AS_DoesWmTakeFocus);
	return (*protocols != 0);
}

static        Bool
client_hints2motif_hints (MwmHints * motif_hints, ASHints * hints, ASStatusHints * status)
{
	ASFlagType tmp ;
	memset (motif_hints, 0x00, sizeof (MwmHints));

	if (status)
	{
		if (get_flags (status->flags, AS_StartLayer))
		{
			if (status->layer == AS_LayerUrgent && get_flags (status->flags, AS_StartsSticky))
			{
				motif_hints->inputMode = MWM_INPUT_SYSTEM_MODAL;
				set_flags (motif_hints->flags, MWM_HINTS_INPUT_MODE);
			} else if (status->layer == AS_LayerTop)
			{
				motif_hints->inputMode = MWM_INPUT_SYSTEM_MODAL;
				set_flags (motif_hints->flags, MWM_INPUT_FULL_APPLICATION_MODAL);
			}
		}
	}
	/* finally we can apply conglomerated hints to our flags : */
	tmp = motif_hints->decorations ;
	encode_simple_flags (&tmp, mwm_decor_xref, hints->flags);
	encode_simple_flags (&tmp, mwm_decor_func_xref, hints->function_mask);
	motif_hints->decorations = tmp ;
	tmp = motif_hints->functions ;
	encode_simple_flags (&tmp, mwm_func_xref, hints->function_mask);
	motif_hints->functions = tmp ;

	check_motif_hints_sanity (motif_hints);

	if (motif_hints->functions != 0)
		set_flags (motif_hints->flags, MWM_HINTS_FUNCTIONS);
	if (motif_hints->decorations != 0)
		set_flags (motif_hints->flags, MWM_HINTS_DECORATIONS);

	return (motif_hints->flags != 0);
}

static        Bool
client_hints2gnome_hints (GnomeHints * gnome_hints, ASHints * hints, ASStatusHints * status)
{
	ASFlagType tmp = 0 ;
	memset (gnome_hints, 0x00, sizeof (GnomeHints));

	if (status)
	{
		if (get_flags (status->flags, AS_StartLayer))
		{
			gnome_hints->layer = (status->layer << 1) + WIN_LAYER_NORMAL;
			set_flags (gnome_hints->flags, GNOME_LAYER);
		}

		if (get_flags (status->flags, AS_StartDesktop))
		{
			gnome_hints->workspace = status->desktop;
			set_flags (gnome_hints->flags, GNOME_WORKSPACE);
		}
	}
	gnome_hints->state = encode_gnome_state (hints, status);
	if (gnome_hints->state != 0)
		set_flags (gnome_hints->flags, GNOME_STATE);

	tmp = gnome_hints->hints ;
	encode_simple_flags (&tmp, gnome_hints_xref, hints->flags);
	gnome_hints->hints = tmp ;
	if (gnome_hints->hints != 0)
		set_flags (gnome_hints->flags, GNOME_HINTS);

	return (gnome_hints->flags != 0);
}

static        Bool
client_hints2extwm_hints (ExtendedWMHints * extwm_hints, ASHints * hints, ASStatusHints * status)
{
	memset (extwm_hints, 0x00, sizeof (ExtendedWMHints));

	if (status)
	{
		if (get_flags (status->flags, AS_StartsSticky))
		{
			extwm_hints->desktop = 0xFFFFFFFF;
			set_flags (extwm_hints->flags, EXTWM_DESKTOP);
		} else if (get_flags (status->flags, AS_StartDesktop))
		{
			extwm_hints->desktop = status->desktop;
			set_flags (extwm_hints->flags, EXTWM_DESKTOP);
		}
		/* window state hints : */
		if (get_flags (status->flags, AS_StartLayer) && status->layer >= AS_LayerTop)
		{
			set_flags (extwm_hints->flags, EXTWM_StateModal);
			encode_simple_flags (&(extwm_hints->flags), extwm_state_xref, status->flags);
		}
		if (get_flags (status->flags, AS_SkipWinList))
			set_flags (extwm_hints->flags, EXTWM_StateSkipTaskbar);
		
		/* window type hints : */
		if (get_flags (status->flags, AS_StartLayer) && status->layer != AS_LayerNormal)
		{
			register int  i;

			for (i = 0; extwm_types_start_properties[i][0] != 0; i++)
				if (status->layer == extwm_types_start_properties[i][1] &&
					(get_flags (status->flags, extwm_types_start_properties[i][2]) ||
					 extwm_types_start_properties[i][2] == 0))
				{
					set_flags (extwm_hints->flags, extwm_types_start_properties[i][0]);
				}
		}
	}

	encode_simple_flags (&(extwm_hints->flags), extwm_type_xref, hints->flags);
	encode_simple_flags (&(extwm_hints->flags), extwm_type_func_mask, hints->function_mask);

	if (hints->pid >= 0 && get_flags (hints->flags, AS_PID))
	{
		set_flags (extwm_hints->flags, EXTWM_PID);
		extwm_hints->pid = hints->pid;
	}
	if (get_flags (hints->protocols, AS_DoesWmPing))
		set_flags (extwm_hints->flags, EXTWM_DoesWMPing);

	return (extwm_hints->flags != 0);
}


Bool
set_all_client_hints (Window w, ASHints * hints, ASStatusHints * status, Bool set_command)
{
	XWMHints      wm_hints;
	XSizeHints    size_hints;
	ASFlagType    protocols;
	MwmHints      mwm_hints;
	GnomeHints    gnome_hints;
	ExtendedWMHints extwm_hints;

	if (w == None || hints == NULL)
		return False;

	set_client_names (w, hints->names[0], hints->icon_name, hints->res_class, hints->res_name);

	if (client_hints2wm_hints (&wm_hints, hints, status))
		XSetWMHints (dpy, w, &wm_hints);

	if (get_flags (hints->flags, AS_Transient))
		XSetTransientForHint (dpy, w, hints->transient_for);

	if (client_hints2size_hints (&size_hints, hints, status))
		XSetWMNormalHints (dpy, w, &size_hints);
	
	if (client_hints2extwm_hints (&extwm_hints, hints, status))
		set_extwm_hints (w, &extwm_hints);

	if ( client_hints2wm_protocols (&protocols, hints) || 
		 get_flags( extwm_hints.flags, EXTWM_DoesWMPing) )
		set_client_protocols (w, protocols, extwm_hints.flags);

#if 0
	if (set_command)
	{
		char         *host_name = safecalloc (MAXHOSTNAME + 1, sizeof (char));

		if (mygethostname (host_name, MAXHOSTNAME))
			set_text_property (w, XA_WM_CLIENT_MACHINE, &host_name, 1, TPE_String);

		if (MyArgs.saved_argc > 0 && MyArgs.saved_argv)
			set_text_property (w, XA_WM_COMMAND, MyArgs.saved_argv, MyArgs.saved_argc, TPE_String);
		free (host_name);
	}
#endif
	if (client_hints2motif_hints (&mwm_hints, hints, status))
		set_multi32bit_property (w, _XA_MwmAtom, XA_CARDINAL, 4,
								 mwm_hints.flags, mwm_hints.functions, mwm_hints.decorations, mwm_hints.inputMode);

	if (client_hints2gnome_hints (&gnome_hints, hints, status))
		set_gnome_hints (w, &gnome_hints);

	return True;
}


/***********************************************************************************
 * Hints printing functions :
 ***********************************************************************************/

void
print_clean_hints (stream_func func, void *stream, ASHints * clean)
{
	register int  i;

	if (!pre_print_check (&func, &stream, clean, "No hints available(NULL)."))
		return;
	for (i = 0; i < MAX_WINDOW_NAMES && clean->names[i]; i++)
	{
		func (stream, "CLEAN.NAMES[%d] = \"%s\";\n", i, clean->names[i]);
		func (stream, "CLEAN.NAMES_ENCODING[%d] = %d;\n", i, clean->names_encoding[i]);
	}
	if (clean->icon_name)
	{
		func (stream, "CLEAN.icon_name = \"%s\";\n", clean->icon_name);
		func (stream, "CLEAN.icon_name_encoding = %d;\n", clean->names_encoding[clean->icon_name_idx]);
	}
	if (clean->res_name)
	{
		func (stream, "CLEAN.res_name = \"%s\";\n", clean->res_name);
		func (stream, "CLEAN.res_name_encoding = %d;\n", clean->names_encoding[clean->res_name_idx]);
	}
	if (clean->res_class)
	{
		func (stream, "CLEAN.res_class = \"%s\";\n", clean->res_class);
		func (stream, "CLEAN.res_class_encoding = %d;\n", clean->names_encoding[clean->res_class_idx]);
	}
	func (stream, "CLEAN.flags = 0x%lX;\n", clean->flags);
	func (stream, "CLEAN.protocols = 0x%lX;\n", clean->protocols);
	func (stream, "CLEAN.function_mask = 0x%lX;\n", clean->function_mask);

	if (get_flags (clean->flags, AS_Icon))
	{
		if (get_flags (clean->flags, AS_ClientIcon))
		{
			if (get_flags (clean->flags, AS_ClientIconPixmap))
				func (stream, "CLEAN.icon.pixmap = 0x%lX;\n", clean->icon.pixmap);
			else
				func (stream, "CLEAN.icon.window = 0x%lX;\n", clean->icon.window);
			func (stream, "CLEAN.icon_mask = 0x%lX;\n", clean->icon_mask);
			if (get_flags (clean->flags, AS_ClientIconPosition))
				func (stream, "CLEAN.icon_x = %d;\nCLEAN.icon_y = %d;\n", clean->icon_x, clean->icon_y);
		} else if (clean->icon_file)
			func (stream, "CLEAN.icon_file = \"%s\";\n", clean->icon_file);
	}
	if (get_flags (clean->flags, AS_MinSize))
		func (stream, "CLEAN.min_width = %u;\nCLEAN.min_height = %u;\n", clean->min_width, clean->min_height);
	if (get_flags (clean->flags, AS_MaxSize))
		func (stream, "CLEAN.max_width = %u;\nCLEAN.max_height = %u;\n", clean->max_width, clean->max_height);
	if (get_flags (clean->flags, AS_SizeInc))
		func (stream, "CLEAN.width_inc = %u;\nCLEAN.height_inc = %u;\n", clean->width_inc, clean->height_inc);
	if (get_flags (clean->flags, AS_Aspect))
	{
		func (stream, "CLEAN.min_aspect.x = %d;\nCLEAN.min_aspect.y = %d;\n", clean->min_aspect.x, clean->min_aspect.y);
		func (stream, "CLEAN.max_aspect.x = %d;\nCLEAN.max_aspect.y = %d;\n", clean->max_aspect.x, clean->max_aspect.y);
	}
	if (get_flags (clean->flags, AS_BaseSize))
		func (stream, "CLEAN.base_width = %u;\nCLEAN.base_height = %u;\n", clean->base_width, clean->base_height);
	if (get_flags (clean->flags, AS_Gravity))
		func (stream, "CLEAN.gravity = %d;\n", clean->gravity);
	if (get_flags (clean->flags, AS_Border))
		func (stream, "CLEAN.border_width = %u;\n", clean->border_width);
	if (get_flags (clean->flags, AS_Handles))
		func (stream, "CLEAN.handle_width = %u;\n", clean->handle_width);

	if (clean->group_lead)
		func (stream, "CLEAN.group_lead = 0x%lX;\n", clean->group_lead);
	if (get_flags (clean->flags, AS_Transient))
		func (stream, "CLEAN.transient_for = 0x%lX;\n", clean->transient_for);

	if (clean->cmap_windows)
		for (i = 0; clean->cmap_windows[i] != None; i++)
			func (stream, "CLEAN.cmap_windows[%d] = 0x%lX;\n", i, clean->cmap_windows[i]);

	if (get_flags (clean->flags, AS_PID))
		func (stream, "CLEAN.pid = %d;\n", clean->pid);
	if (clean->frame_name && get_flags (clean->flags, AS_Frame))
		func (stream, "CLEAN.frame_name = \"%s\";\n", clean->frame_name);
    if (clean->windowbox_name && get_flags (clean->flags, AS_Windowbox))
        func (stream, "CLEAN.windowbox_name = \"%s\";\n", clean->windowbox_name);

	for (i = 0; i < BACK_STYLES; i++)
		if (clean->mystyle_names[i])
			func (stream, "CLEAN.mystyle_names[%d] = \"%s\";\n", i, clean->mystyle_names[i]);

	func (stream, "CLEAN.disabled_buttons = 0x%lX;\n", clean->disabled_buttons);

	func (stream, "CLEAN.hints_types_raw = 0x%lX;\n", clean->hints_types_raw);
	func (stream, "CLEAN.hints_types_clean = 0x%lX;\n", clean->hints_types_clean);

	if (clean->client_host)
		func (stream, "CLEAN.client_host = \"%s\";\n", clean->client_host);
	if (clean->client_cmd)
		func (stream, "CLEAN.client_cmd = \"%s\";\n", clean->client_cmd);

}

void
print_status_hints (stream_func func, void *stream, ASStatusHints * status)
{
	if (!pre_print_check (&func, &stream, status, "No status available(NULL)."))
		return;

	func (stream, "STATUS.flags = 0x%lX;\n", status->flags);

	if (get_flags (status->flags, AS_StartPositionUser))
	{
		func (stream, "STATUS.user_x = %d;\n", status->x);
		func (stream, "STATUS.user_y = %d;\n", status->y);
	} else
	{
		func (stream, "STATUS.x = %d;\n", status->x);
		func (stream, "STATUS.y = %d;\n", status->y);
	}
	if (get_flags (status->flags, AS_Size))
	{
		func (stream, "STATUS.width = %d;\n", status->width);
		func (stream, "STATUS.height = %d;\n", status->height);
	}
	if (get_flags (status->flags, AS_Desktop))
		func (stream, "STATUS.desktop = %d;\n", status->desktop);
	if (get_flags (status->flags, AS_Layer))
		func (stream, "STATUS.desktop = %d;\n", status->layer);
	if (get_flags (status->flags, AS_StartViewportX | AS_StartViewportX))
	{
		func (stream, "STATUS.viewport_x = %d;\n", status->viewport_x);
		func (stream, "STATUS.viewport_y = %d;\n", status->viewport_y);
	}
}

/*********************************************************************************
 *  serialization for purpose of inter-module communications                     *
 *********************************************************************************/
void
serialize_string (char *string, ASVector * buf)
{
	if (buf)
	{
        register CARD32 *ptr;
		register CARD8 *src = string;
		register int  i = string ? strlen (string) >> 2 : 0;	/* assume CARD32 == 4*CARD8 :)) */

		append_vector (buf, NULL, 1 + i + 1);
		ptr = VECTOR_TAIL (CARD32, *buf);
		VECTOR_USED (*buf) += i + 1;
		ptr[0] = i + 1;
        ++ptr;
		if (string == NULL)
		{
			ptr[0] = 0;
			return;
		}
		src = &(string[i << 2]);
		/* unrolling loop here : */
		ptr[i] = src[0];
        if( src[0] )
        {
            if (src[1])
            {   /* we don't really want to use bitwise operations */
                /* so we get "true" number and later can do ENDIANNES transformations */
                ptr[i] |=  ((CARD32)src[1]) <<8 ;
                if (src[2])
                    ptr[i] |= ((CARD32) src[2])<<16;
            }
        }
		while (--i >= 0)
		{
			src -= 4;
			ptr[i] =
                ((CARD32) src[0]) | ((CARD32) src[1]<<8) |((CARD32) src[2]<<16)|((CARD32) src[3]<<24);
		}
	}
}

void
serialize_CARD32_zarray (CARD32 * array, ASVector * buf)
{
	register int  i = 0;
	register CARD32 *ptr;

	if (array)
		while (array[i])
			i++;
	i++;
	append_vector (buf, NULL, 1 + i);
	ptr = VECTOR_TAIL (CARD32, *buf);
	VECTOR_USED (*buf) += i;
	ptr[0] = i;
	ptr++;
	if (array == NULL)
		ptr[0] = 0;
	else
		while (--i >= 0)
			ptr[i] = array[i];
}

Bool
serialize_clean_hints (ASHints * clean, ASVector * buf)
{
	register CARD32 *ptr;
	register int  i = 0;

	if (clean == NULL || buf == NULL)
		return False;

	/* we expect CARD32 vector here : */
	if (VECTOR_UNIT (*buf) != sizeof (CARD32))
		return False;

	append_vector (buf, NULL, ASHINTS_STATIC_DATA);
	ptr = VECTOR_TAIL (CARD32, *buf);
	ptr[i++] = clean->flags;
	ptr[i++] = clean->protocols;
	ptr[i++] = clean->function_mask;
	ptr[i++] = clean->icon.window;
	ptr[i++] = clean->icon_mask;
	ptr[i++] = clean->icon_x;
	ptr[i++] = clean->icon_y;

	ptr[i++] = clean->min_width;
	ptr[i++] = clean->min_height;
	ptr[i++] = clean->max_width;
	ptr[i++] = clean->max_height;
	ptr[i++] = clean->width_inc;
	ptr[i++] = clean->height_inc;
	ptr[i++] = clean->min_aspect.x;
	ptr[i++] = clean->min_aspect.y;
	ptr[i++] = clean->max_aspect.x;
	ptr[i++] = clean->max_aspect.y;
	ptr[i++] = clean->base_width;
	ptr[i++] = clean->base_height;

	ptr[i++] = clean->gravity;
	ptr[i++] = clean->border_width;
	ptr[i++] = clean->handle_width;

	ptr[i++] = clean->group_lead;
	ptr[i++] = clean->transient_for;

	ptr[i++] = clean->pid;

	ptr[i++] = clean->disabled_buttons;

	ptr[i++] = clean->hints_types_raw;
	ptr[i++] = clean->hints_types_clean;
	VECTOR_USED (*buf) += i;

	serialize_CARD32_zarray (clean->cmap_windows, buf);

	serialize_string (clean->icon_file, buf);
	serialize_string (clean->frame_name, buf);
    serialize_string (clean->windowbox_name, buf);

	for (i = 0; i < BACK_STYLES; i++)
		serialize_string (clean->mystyle_names[i], buf);

	serialize_string (clean->client_host, buf);
	serialize_string (clean->client_cmd, buf);

	return True;
}

Bool
serialize_names (ASHints * clean, ASVector * buf)
{
	CARD32        header[4];
	register int  i;

	if (clean == NULL || buf == NULL)
		return False;
	header[0] = pointer_name_to_index_in_list (clean->names, clean->res_name);
	header[1] = pointer_name_to_index_in_list (clean->names, clean->res_class);
	header[2] = pointer_name_to_index_in_list (clean->names, clean->icon_name);
	for (i = 0; clean->names[i] != NULL && i < MAX_WINDOW_NAMES; i++);
	header[3] = i;
	append_vector (buf, &(header[0]), 4);

	for (i = 0; clean->names[i] != NULL && i < MAX_WINDOW_NAMES; i++)
		serialize_string (clean->names[i], buf);

	return True;
}

Bool
serialize_status_hints (ASStatusHints * status, ASVector * buf)
{
	register CARD32 *ptr;
	register int  i = 0;

	if (status == NULL || buf == NULL)
		return False;

	/* we expect CARD32 vector here : */
	if (VECTOR_UNIT (*buf) != sizeof (CARD32))
		return False;

	append_vector (buf, NULL, ASSTATUSHINTS_STATIC_DATA);
	ptr = VECTOR_TAIL (CARD32, *buf);
	ptr[i++] = status->flags;

	ptr[i++] = status->x;
	ptr[i++] = status->y;
	ptr[i++] = status->width;
	ptr[i++] = status->height;
	ptr[i++] = status->border_width;
	ptr[i++] = status->viewport_x;
	ptr[i++] = status->viewport_y;
	ptr[i++] = status->desktop;
	ptr[i++] = status->layer;
	ptr[i++] = status->icon_window;
	VECTOR_USED (*buf) += i;

	return True;
}

/*********************************************************************************
 *  deserialization so that module can read out communications 	                 *
 *********************************************************************************/
char         *
deserialize_string (CARD32 ** pbuf, size_t * buf_size)
{
	char         *string;
	CARD32       *buf = *pbuf;
	size_t        len;
	register int  i;
	register char *str;

    if (*pbuf == NULL )
		return NULL;
    if( buf_size && *buf_size < 2)
        return NULL;
	len = buf[0];
    if( buf_size && len > *buf_size + 1)
		return NULL;
	buf++;
	str = string = safemalloc (len << 2);
	for (i = 0; i < len; i++)
	{
        str[0] = (buf[i] & 0x0FF);
        str[1] = (buf[i] >> 8)&0x0FF;
        str[2] = (buf[i] >> 16)&0x0FF;
        str[3] = (buf[i] >> 24)&0x0FF;
		str += 4;
	}

    if( buf_size )
        *buf_size -= len;
	*pbuf += len;

	return string;
}

CARD32       *
deserialize_CARD32_zarray (CARD32 ** pbuf, size_t * buf_size)
{
	CARD32       *array;
	CARD32       *buf = *pbuf;
	size_t        len;
	register int  i;

	if (*pbuf == NULL || *buf_size < 2)
		return NULL;
	len = buf[0];
	if (len > *buf_size + 1)
		return NULL;
	buf++;
	array = safemalloc (len * sizeof (CARD32));
	for (i = 0; i < len; i++)
		array[i] = buf[i];

	*buf_size -= len;
	*pbuf += len;

	return array;
}

ASHints      *
deserialize_clean_hints (CARD32 ** pbuf, size_t * buf_size, ASHints * reusable_memory)
{
	ASHints      *clean = reusable_memory;
	register int  i = 0;
	register CARD32 *buf = *pbuf;

	if (buf == NULL || *buf_size < ASHINTS_STATIC_DATA)
		return False;

	if (clean == NULL)
		clean = safecalloc (1, sizeof (ASHints));

	clean->flags = buf[i++];
	clean->protocols = buf[i++];
	clean->function_mask = buf[i++];

	clean->icon.window = buf[i++];
	clean->icon_mask = buf[i++];
	clean->icon_x = buf[i++];
	clean->icon_y = buf[i++];

	clean->min_width = buf[i++];
	clean->min_height = buf[i++];
	clean->max_width = buf[i++];
	clean->max_height = buf[i++];
	clean->width_inc = buf[i++];
	clean->height_inc = buf[i++];
	clean->min_aspect.x = buf[i++];
	clean->min_aspect.y = buf[i++];
	clean->max_aspect.x = buf[i++];
	clean->max_aspect.y = buf[i++];
	clean->base_width = buf[i++];
	clean->base_height = buf[i++];
	clean->gravity = buf[i++];
	clean->border_width = buf[i++];
	clean->handle_width = buf[i++];
	clean->group_lead = buf[i++];
	clean->transient_for = buf[i++];
	clean->pid = buf[i++];
	clean->disabled_buttons = buf[i++];

	clean->hints_types_raw = buf[i++];
	clean->hints_types_clean = buf[i++];

	*buf_size -= i;
	*pbuf += i;
	if (clean->cmap_windows)
		free (clean->cmap_windows);
	clean->cmap_windows = deserialize_CARD32_zarray (pbuf, buf_size);

	if (clean->icon_file)
		free (clean->icon_file);
	clean->icon_file = deserialize_string (pbuf, buf_size);
	if (clean->frame_name)
		free (clean->frame_name);
	clean->frame_name = deserialize_string (pbuf, buf_size);
    if (clean->windowbox_name)
        free (clean->windowbox_name);
    clean->windowbox_name = deserialize_string (pbuf, buf_size);

	for (i = 0; i < BACK_STYLES; i++)
	{
		if (clean->mystyle_names[i])
			free (clean->mystyle_names[i]);
		clean->mystyle_names[i] = deserialize_string (pbuf, buf_size);
	}
	if (clean->client_host)
		free (clean->client_host);
	clean->client_host = deserialize_string (pbuf, buf_size);
	if (clean->client_cmd)
		free (clean->client_cmd);
	clean->client_cmd = deserialize_string (pbuf, buf_size);

	return clean;
}

Bool
deserialize_names (ASHints * clean, CARD32 ** pbuf, size_t * buf_size)
{
	CARD32        header[4];
	CARD32       *buf = *pbuf;
	register int  i;

	if (clean == NULL || buf == NULL || *buf_size < 4)
		return False;

	header[0] = buf[0];
	header[1] = buf[1];
	header[2] = buf[2];
	header[3] = buf[3];

	if (header[3] <= 0 || header[3] >= MAX_WINDOW_NAMES)
		return False;

	*buf_size -= 4;
	*pbuf += 4;

	for (i = 0; i < header[3]; i++)
	{
		if (clean->names[i])
			free (clean->names[i]);
		clean->names[i] = deserialize_string (pbuf, buf_size);
	}
	clean->res_name = (header[0] < MAX_WINDOW_NAMES) ? clean->names[header[0]] : NULL;
	clean->res_name_idx = 0 ;
	clean->res_class = (header[1] < MAX_WINDOW_NAMES) ? clean->names[header[1]] : NULL;
	clean->res_class_idx = 0 ;
	clean->icon_name = (header[2] < MAX_WINDOW_NAMES) ? clean->names[header[2]] : NULL;
	clean->icon_name_idx = 0 ;

	return True;
}


ASStatusHints *
deserialize_status_hints (CARD32 ** pbuf, size_t * buf_size, ASStatusHints * reusable_memory)
{
	ASStatusHints *status = reusable_memory;
	register int  i = 0;
	register CARD32 *buf = *pbuf;

	if (buf == NULL || *buf_size < ASSTATUSHINTS_STATIC_DATA)
		return False;

	if (status == NULL)
		status = safecalloc (1, sizeof (ASStatusHints));

	status->flags = buf[i++];
	status->x = buf[i++];
	status->y = buf[i++];
	status->width = buf[i++];
	status->height = buf[i++];
	status->border_width = buf[i++];
	status->viewport_x = buf[i++];
	status->viewport_y = buf[i++];
	status->desktop = buf[i++];
	status->layer = buf[i++];
	status->icon_window = buf[i++];
	*buf_size -= i;
	*pbuf += i;

	return status;
}

/*********************************************************************************
 *                  List of Supported Hints management                           *
 *********************************************************************************/
ASSupportedHints *
create_hints_list ()
{
	ASSupportedHints *list;

	list = (ASSupportedHints *) safecalloc (1, sizeof (ASSupportedHints));
	return list;
}

void
destroy_hints_list (ASSupportedHints ** plist)
{
	if (*plist)
	{
		free (*plist);
		*plist = NULL;
	}
}

static        hints_merge_func
HintsTypes2Func (HintsTypes type)
{
	switch (type)
	{
	 case HINTS_ICCCM:
		 return merge_icccm_hints;
	 case HINTS_GroupLead:
		 return merge_group_hints;
	 case HINTS_Transient:
		 return merge_transient_hints;
	 case HINTS_Motif:
		 return merge_motif_hints;
	 case HINTS_Gnome:
		 return merge_gnome_hints;
	 case HINTS_ExtendedWM:
		 return merge_extwm_hints;
	 case HINTS_XResources:
		 return merge_xresources_hints;
	 case HINTS_ASDatabase:
		 return merge_asdb_hints;
	 case HINTS_Supported:
		 break;
	}
	return NULL;
}

HintsTypes
Func2HintsTypes (hints_merge_func func)
{
	if (func == merge_group_hints)
		return HINTS_GroupLead;
	else if (func == merge_transient_hints)
		return HINTS_Transient;
	else if (func == merge_motif_hints)
		return HINTS_Motif;
	else if (func == merge_gnome_hints)
		return HINTS_Gnome;
	else if (func == merge_extwm_hints)
		return HINTS_ExtendedWM;
	else if (func == merge_xresources_hints)
		return HINTS_XResources;
	else if (func == merge_asdb_hints)
		return HINTS_ASDatabase;
	return HINTS_ICCCM;
}

Bool
enable_hints_support (ASSupportedHints * list, HintsTypes type)
{
	if (list)
	{
		if (list->hints_num > HINTS_Supported)
			list->hints_num = HINTS_Supported; /* we are being paranoid */

		if (get_flags (list->hints_flags, (0x01 << type)))	/* checking for duplicates */
			return False;

		if ((list->merge_funcs[list->hints_num] = HintsTypes2Func (type)) == NULL)
			return False;

		list->hints_types[list->hints_num] = type;
		set_flags (list->hints_flags, (0x01 << type));
		list->hints_num++;
		return True;
	}
	return False;
}

Bool
disable_hints_support (ASSupportedHints * list, HintsTypes type)
{
	if (list)
	{
		register int  i;

		if (list->hints_num > HINTS_Supported)
			list->hints_num = HINTS_Supported; /* we are being paranoid */
		for (i = 0; i < list->hints_num; i++)
			if (list->hints_types[i] == type)
			{
				list->hints_num--;
				for (; i < list->hints_num; i++)
				{
					list->merge_funcs[i] = list->merge_funcs[i + 1];
					list->hints_types[i] = list->hints_types[i + 1];
				}
				list->merge_funcs[i] = NULL;
				list->hints_types[i] = HINTS_Supported;
				clear_flags (list->hints_flags, (0x01 << type));
				return True;
			}
	}
	return False;
}

HintsTypes   *
supported_hints_types (ASSupportedHints * list, int *num_return)
{
	HintsTypes   *types = NULL;
	int           curr = 0;

	if (list)
		if ((curr = list->hints_num) > 0)
			types = list->hints_types;

	if (num_return)
		*num_return = curr;
	return types;
}
