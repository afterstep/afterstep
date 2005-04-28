/*
 * Copyright (C) 2000,2002 Sasha Vasko <sasha at aftercode.net>
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

#define LOCAL_DEBUG

#include "../configure.h"
#include "asapp.h"
#include "afterstep.h"
#include "parser.h"
#include "mystyle.h"
#include "screen.h"
#include "balloon.h"
#include "event.h"
#include "wmprops.h"
#include "../libAfterImage/afterimage.h"

int _as_frame_corner_xref[FRAME_SIDES+1] = {FR_NW, FR_NE, FR_SE, FR_SW, FR_NW};

unsigned int _as_default_button_xref[TITLE_BUTTONS] =
    { C_TButton1, C_TButton3, C_TButton5, C_TButton7, C_TButton9,
      C_TButton0, C_TButton8, C_TButton6, C_TButton4, C_TButton2 };



/* TODO: fix MyLook loading to store everything in ARGB and thus be multiscreen
 * compatible */
char *
get_default_obj_name( unsigned long type_magic, Bool icon, Bool vertical )
{
	static char *normal_name		= NULL;
	static char *icon_name			= NULL;
	static char *vertical_name	    = NULL;
	static char *vertical_icon_name = NULL;
	char **res = NULL ;
	char hex[sizeof(unsigned long)*2+1] ;
	register char *ptr ;
	if( icon )
	{
		if( vertical )
			res = &vertical_icon_name;
		else
			res = &icon_name;
	}else if( vertical )
		res = &vertical_name;
	else
		res = &normal_name;
	if( *res == NULL )
	{
		if( icon )
		{
			if( vertical )
				*res = mystrdup( "0." DEFAULT_ICON_VERTICAL ".0000000");
			else
				*res = mystrdup( "0." DEFAULT_ICON_OBJ_NAME ".0000000");
		}else if( vertical )
			*res = mystrdup( "0." DEFAULT_VERTICAL ".0000000");
		else
			*res = mystrdup( "0." DEFAULT_OBJ_NAME ".0000000");
	}
	NUMBER2HEX(type_magic,hex);
	(*res)[0] = hex[sizeof(unsigned long)*2-1] ;
	for( ptr = &((*res)[2]) ; *ptr != '.' ; ++ptr );
	memcpy( ptr+1, &(hex[sizeof(unsigned long)*2-8]), 7 );
	return &((*res)[0]) ;
}

/***********************************************************************
 * Generic object destructor :
 **********************************************************************/
void
myobj_destroy (ASHashableValue value, void *data)
{
	if (data)
	{
		union
		{
			void *vptr;
			ASMagic *obj ;
			MyFrame *myframe ;
			MyBackground *myback ;
			MyDesktopConfig *mydesk ;
			MyLook *mylook ;
		} variant ;
		variant.vptr = data ;

		switch( variant.obj->magic )
		{
			case MAGIC_MYFRAME       :
                destroy_myframe( &variant.myframe );
				break ;
            case MAGIC_MYBACKGROUND :
				myback_delete ( &variant.myback, NULL );
			    break ;
			case MAGIC_MYDESKTOPCONFIG :
				mydeskconfig_delete ( &variant.mydesk );
			    break ;
			case MAGIC_MYLOOK :
				mylook_destroy ( &variant.mylook );
			    break ;
		}
	}
}

ARGB32
get_random_tint_color()
{	
	static ARGB32 tint_colors[6] = {0xFFFF0000,0xFFFFFF00,0xFFFF00FF,0xFF00FFFF,0xFF00FF00,0xFF0000FF};
	return tint_colors[time(NULL)%6] ;
}
	

/*************************************************************************/
/* MyLook :                                                              */
/*************************************************************************/
void
mylook_init (MyLook * look, Bool free_resources, unsigned long what_flags /*see LookLoadFlags in mylook.h */ )
{
	register int  i;

	if (look == NULL)
		return;
    if( look->magic != MAGIC_MYLOOK )
        return;

    if (get_flags (what_flags, LL_Balloons))
        balloon_init (free_resources);

    if (free_resources)
	{
		if (get_flags (what_flags, LL_MyStyles) && look->styles_list)
		{
			mystyle_list_destroy_all(&(look->styles_list));
		}

		if (get_flags (what_flags, LL_DeskBacks))
			destroy_ashash( &(look->backs_list) );

		if (get_flags (what_flags, LL_DeskConfigs))
			destroy_ashash( &(look->desk_configs) );

		if (get_flags (what_flags, LL_MenuIcons))
		{
            if (look->MenuArrow != NULL)
            {
                destroy_icon( &(look->MenuArrow) );
                look->MenuArrow = NULL;
            }
        }

		if (get_flags (what_flags, LL_Buttons))
		{
            for (i = 0; i < TITLE_BUTTONS; i++)
                free_button_resources( &(look->buttons[i]) );
        }
        if (get_flags (what_flags, LL_SupportedHints) && look->supported_hints)
            destroy_hints_list ( &(look->supported_hints) );

        if (get_flags (what_flags, LL_Layouts) )
        {
            if( look->DefaultFrame )
                destroy_myframe( &(look->DefaultFrame) );
            if( look->FramesList )
                destroy_ashash( &(look->FramesList));
        }
        if( look->configured_icon_areas && get_flags (what_flags, LL_Icons) )
		{
            free( look->configured_icon_areas );
			if( look->DefaultIcon )
				free( look->DefaultIcon );
		}

        if( look->balloon_look && get_flags (what_flags, LL_Balloons))
        {
            free( look->balloon_look );
        }

		if (get_flags (what_flags, LL_Misc))
		{
			if( look->CursorFore )
				free( look->CursorFore );
			if(	look->CursorBack )
				free( look->CursorBack );
    	}
	}/* free_resources */

	if (get_flags (what_flags, LL_MyStyles))
	{
		look->styles_list = NULL;

		if (get_flags (what_flags, LL_MSMenu))
		{									   /* menu MyStyles */
			for (i = 0; i < MENU_BACK_STYLES; i++)
				look->MSMenu[i] = NULL;
		}

		if (get_flags (what_flags, LL_MSWindow))
		{									   /* focussed, unfocussed, sticky window styles */
			for (i = 0; i < BACK_STYLES; i++)
				look->MSWindow[i] = NULL;
		}
	}
	if (get_flags (what_flags, LL_DeskBacks))
		look->backs_list = NULL ;
	if (get_flags (what_flags, LL_DeskConfigs))
		look->desk_configs = NULL ;
	if (get_flags (what_flags, LL_MenuIcons))
	{
		look->MenuArrow = NULL;
		look->minipixmap_width = 48 ;
		look->minipixmap_height = 48 ;
    }

	if (get_flags (what_flags, LL_Buttons))
	{										   /* TODO: Titlebuttons - we 'll move them into  MyStyles */
		/* we will translate button numbers from the user input */
        look->TitleButtonSpacing[0] = look->TitleButtonSpacing[1] = 2;
        look->TitleButtonStyle = 0;
        for (i = 0; i < TITLE_BUTTONS; i++)
        {
            memset(&(look->buttons[i]), 0x00, sizeof(MyButton));
            look->button_xref[i] = _as_default_button_xref[i] ;
            look->ordered_buttons[i] = NULL ;
        }
        look->button_first_right = DEFAULT_FIRST_RIGHT_BUTTON ;
    }

	if (get_flags (what_flags, LL_Layouts))
    {
        look->DefaultFrame = NULL ;
        look->FramesList = NULL ;
        look->TitleTextAlign = 0;
		look->TitleButtonStyle = 0;
		look->TitleButtonSpacing[0] = look->TitleButtonSpacing[1] = 0;
		look->TitleButtonXOffset[0] = look->TitleButtonXOffset[1] = 0;
		look->TitleButtonYOffset[0] = look->TitleButtonYOffset[1] = 0;
    }

	if (get_flags (what_flags, LL_SizeWindow))
	{
        memset( &(look->resize_move_geometry), 0x00, sizeof(ASGeometry));
	}

    if (get_flags (what_flags, LL_MenuParams))
	{
		look->DrawMenuBorders = 1;
		look->StartMenuSortMode = DEFAULTSTARTMENUSORT;
        look->menu_icm = TEXTURE_TRANSPIXMAP_ALPHA ;
        look->menu_hcm = TEXTURE_TRANSPIXMAP_ALPHA ;
        look->menu_scm = TEXTURE_TRANSPIXMAP_ALPHA ;
	}

	if (get_flags (what_flags, LL_Icons))
	{
        look->configured_icon_areas = NULL ;
        look->configured_icon_areas_num = 0;
		look->ButtonWidth = 64;
		look->ButtonHeight = 64;
		look->ButtonIconSpacing = 2;
		look->ButtonAlign  = ALIGN_CENTER|RESIZE_H_SCALE|RESIZE_V_SCALE;
		look->ButtonBevel  = DEFAULT_TBAR_HILITE;

        look->DefaultIcon = NULL;
    }

	if (get_flags (what_flags, LL_Misc))
	{
		CARD32 Base ;
		look->RubberBand = 0;
		look->CursorFore = NULL ;
		look->CursorBack = NULL ;
		if( !get_custom_color("Base", &Base) ) 
			look->desktop_animation_tint = get_random_tint_color() ;
		else
		{
			int Base_hue ;
			Base_hue = asxml_var_get("ascs.Base.hue");
			if( Base_hue < 30 || Base_hue >= 330 ) 	look->desktop_animation_tint = 0xFFFF0000;
			else if( Base_hue < 90  ) 	look->desktop_animation_tint = 0xFFFFFF00;
			else if( Base_hue < 150  ) 	look->desktop_animation_tint = 0xFF00FF00;
			else if( Base_hue < 210  ) 	look->desktop_animation_tint = 0xFF00FFFF;
 			else if( Base_hue < 270  ) 	look->desktop_animation_tint = 0xFF0000FF;
			else look->desktop_animation_tint = 0xFFFF00FF;
		}
    }
	LOCAL_DEBUG_OUT( "desk_anime_tint = %lX", look->desktop_animation_tint );
	if (get_flags (what_flags, LL_Flags))
		look->flags = SeparateButtonTitle | TxtrMenuItmInd;

    if (get_flags (what_flags, LL_SupportedHints))
        look->supported_hints = NULL ;

    if( look->balloon_look && get_flags (what_flags, LL_Balloons))
        look->balloon_look = NULL;
}

MyLook       *
mylook_create ()
{
	MyLook       *look;
	static unsigned long   next_look_id = 0 ;

	look = (MyLook *) safecalloc (1, sizeof (MyLook));
    look->magic = MAGIC_MYLOOK ;
	look->look_id = next_look_id++ ;
	mylook_init (look, False, LL_Everything);
	return look;
}

void
mylook_destroy (MyLook ** look)
{
	if (look)
		if (*look)
		{
			mylook_init (*look, True, LL_Everything);
			if( (*look)->name )
				free( (*look)->name );
            (*look)->magic = 0 ;                  /* invalidating object */
			free (*look);
			*look = NULL;
		}
}
/*************************************************************************/
/* MyFrame management :                                                  */
/*************************************************************************/
MyFrame *
create_myframe()
{
    MyFrame *frame = safecalloc( 1, sizeof(MyFrame));
    frame->magic = MAGIC_MYFRAME ;
	
	frame->left_layout = MYFRAME_DEFAULT_TITLE_LAYOUT ;
	frame->right_layout = MYFRAME_DEFAULT_TITLE_LAYOUT ;

	frame->title_fsat = frame->title_usat = frame->title_ssat = -1 ;
	frame->title_fhue = frame->title_uhue = frame->title_shue = -1 ;

    frame->title_h_spacing = DEFAULT_TBAR_HSPACING;
	frame->title_v_spacing = DEFAULT_TBAR_VSPACING;

    return frame;

}

MyFrame *
create_default_myframe(ASFlagType default_title_align)
{
    MyFrame *frame = create_myframe();
    frame->set_parts = 0xFFFFFFFF ;
    frame->parts_mask = C_SIDEBAR ;
    frame->set_part_size = C_SIDEBAR ;
    frame->part_width[FR_S] = BOUNDARY_WIDTH ;
    frame->part_width[FR_SW] = 0 ;
    frame->part_width[FR_SE] = 0 ;
    frame->part_length[FR_S] = 1;
    frame->part_length[FR_SW] = BOUNDARY_WIDTH ;
    frame->part_length[FR_SE] = BOUNDARY_WIDTH ;
    frame->set_part_align = C_SIDEBAR ;
    frame->part_align[FR_S]  = 0;
    frame->part_align[FR_SW] = 0 ;
    frame->part_align[FR_SE] = 0 ;
    SetPartFBevelMask(frame,C_SIDEBAR);
    SetPartUBevelMask(frame,C_SIDEBAR);
    SetPartSBevelMask(frame,C_SIDEBAR);
    frame->part_fbevel[FR_S]  = DEFAULT_TBAR_HILITE;
    frame->part_fbevel[FR_SW] = DEFAULT_TBAR_HILITE;
    frame->part_fbevel[FR_SE] = DEFAULT_TBAR_HILITE;
    frame->part_ubevel[FR_S]  = DEFAULT_TBAR_HILITE ;
    frame->part_ubevel[FR_SE] = DEFAULT_TBAR_HILITE ;
    frame->part_ubevel[FR_SW] = DEFAULT_TBAR_HILITE ;
    frame->part_sbevel[FR_S]  = DEFAULT_TBAR_HILITE ;
    frame->part_sbevel[FR_SW] = DEFAULT_TBAR_HILITE ;
    frame->part_sbevel[FR_SE] = DEFAULT_TBAR_HILITE ;
    set_flags( frame->set_title_attr, MYFRAME_TitleFBevelSet|
                                      MYFRAME_TitleUBevelSet|
                                      MYFRAME_TitleSBevelSet|
                                      MYFRAME_TitleAlignSet|
                                      MYFRAME_TitleFCMSet|
                                      MYFRAME_TitleUCMSet|
                                      MYFRAME_TitleSCMSet );

    frame->title_fbevel = DEFAULT_TBAR_HILITE;
    frame->title_ubevel = DEFAULT_TBAR_HILITE;
    frame->title_sbevel = DEFAULT_TBAR_HILITE;
    frame->title_align = default_title_align;
	frame->left_btn_align = ALIGN_VCENTER ;
	frame->right_btn_align = ALIGN_VCENTER ;
    frame->title_fcm = TEXTURE_TRANSPIXMAP_ALPHA;
    frame->title_ucm = TEXTURE_TRANSPIXMAP_ALPHA;
    frame->title_scm = TEXTURE_TRANSPIXMAP_ALPHA;
    frame->condense_titlebar = 0 ;
	frame->left_layout = MYFRAME_DEFAULT_TITLE_LAYOUT ;
	frame->right_layout = MYFRAME_DEFAULT_TITLE_LAYOUT ;
    frame->title_h_spacing = DEFAULT_TBAR_HSPACING;
	frame->title_v_spacing = DEFAULT_TBAR_VSPACING;
    return frame ;
}

void
inherit_myframe( MyFrame *frame, MyFrame *ancestor )
{
    if( frame && ancestor )
    {
        int i ;
        frame->parts_mask = (frame->parts_mask&(~ancestor->set_parts))|ancestor->parts_mask ;
        frame->set_parts |= ancestor->set_parts ;
        for( i = 0 ; i < FRAME_PARTS ; ++i )
        {
            if( ancestor->part_filenames[i] )
                set_string_value( &(frame->part_filenames[i]), mystrdup(ancestor->part_filenames[i]), NULL, 0 );
            if( get_flags(ancestor->set_part_size, 0x01<<i ) )
            {
                frame->part_width[i] = ancestor->part_width[i] ;
                frame->part_length[i] = ancestor->part_length[i] ;
            }
            if( IsPartFBevelSet(ancestor, i) )
                frame->part_fbevel[i] = ancestor->part_fbevel[i];
            if( IsPartUBevelSet(ancestor, i) )
                frame->part_ubevel[i] = ancestor->part_ubevel[i];
            if( IsPartSBevelSet(ancestor, i) )
                frame->part_sbevel[i] = ancestor->part_sbevel[i];
            if( get_flags(ancestor->set_part_align, 0x01<<i ) )
                frame->part_align[i] = ancestor->part_align[i];
        }
        frame->set_part_size |= ancestor->set_part_size ;
        frame->set_part_bevel |= ancestor->set_part_bevel ;
        frame->set_part_align |= ancestor->set_part_align ;
        for( i = 0 ; i < BACK_STYLES ; ++i )
        {
            if( ancestor->title_style_names )
                set_string_value( &(frame->title_style_names[i]), mystrdup(ancestor->title_style_names[i]), NULL, 0 );
            if( ancestor->frame_style_names )
                set_string_value( &(frame->frame_style_names[i]), mystrdup(ancestor->frame_style_names[i]), NULL, 0 );
        }
		for( i = 0 ; i < MYFRAME_TITLE_BACKS ; ++i )
		{
        	if( ancestor->title_back_filenames[i] )
            	set_string_value( &(frame->title_back_filenames[i]), mystrdup(ancestor->title_back_filenames[i]), NULL, 0 );
	        if( get_flags( ancestor->set_title_attr, MYFRAME_TitleBackAlignSet_Start<<i ) )
    	        frame->title_backs_align[i] = ancestor->title_backs_align[i];
		}

        if( get_flags( ancestor->set_title_attr, MYFRAME_TitleFBevelSet ) )
            frame->title_fbevel = ancestor->title_fbevel;
        if( get_flags( ancestor->set_title_attr, MYFRAME_TitleUBevelSet ) )
            frame->title_ubevel = ancestor->title_ubevel;
        if( get_flags( ancestor->set_title_attr, MYFRAME_TitleSBevelSet ) )
            frame->title_sbevel = ancestor->title_sbevel;

        if( get_flags( ancestor->set_title_attr, MYFRAME_TitleAlignSet ) )
            frame->title_align = ancestor->title_align;
        
		if( get_flags( ancestor->set_title_attr, MYFRAME_LeftBtnAlignSet ) )
            frame->left_btn_align = ancestor->left_btn_align;
		if( get_flags( ancestor->set_title_attr, MYFRAME_RightBtnAlignSet ) )
            frame->right_btn_align = ancestor->right_btn_align;

        if( get_flags( ancestor->set_title_attr, MYFRAME_CondenseTitlebarSet ) )
            frame->condense_titlebar = ancestor->condense_titlebar;
        
        if( get_flags( ancestor->set_title_attr, MYFRAME_TitleFCMSet ) )
            frame->title_fcm = ancestor->title_fcm;
        if( get_flags( ancestor->set_title_attr, MYFRAME_TitleUCMSet ) )
            frame->title_ucm = ancestor->title_ucm;
        if( get_flags( ancestor->set_title_attr, MYFRAME_TitleSCMSet ) )
            frame->title_scm = ancestor->title_scm;

        if( get_flags( ancestor->set_title_attr, MYFRAME_TitleFHueSet ) )
            frame->title_fhue = ancestor->title_fhue;
        if( get_flags( ancestor->set_title_attr, MYFRAME_TitleUHueSet ) )
            frame->title_uhue = ancestor->title_uhue;
        if( get_flags( ancestor->set_title_attr, MYFRAME_TitleSHueSet ) )
            frame->title_shue = ancestor->title_shue;
        if( get_flags( ancestor->set_title_attr, MYFRAME_TitleFSatSet ) )
            frame->title_fsat = ancestor->title_fsat;
        if( get_flags( ancestor->set_title_attr, MYFRAME_TitleUSatSet ) )
            frame->title_usat = ancestor->title_uhue;
        if( get_flags( ancestor->set_title_attr, MYFRAME_TitleSSatSet ) )
            frame->title_ssat = ancestor->title_ssat;

	    if( get_flags( ancestor->set_title_attr, MYFRAME_TitleHSpacingSet ) )
			frame->title_h_spacing = ancestor->title_h_spacing;
		if( get_flags( ancestor->set_title_attr, MYFRAME_TitleVSpacingSet ) )
			frame->title_v_spacing = ancestor->title_v_spacing;

        frame->set_title_attr |= ancestor->set_title_attr ;

		clear_flags( frame->flags, ~ancestor->set_flags );
		set_flags( frame->flags, ancestor->flags );
		frame->set_flags |= ancestor->set_flags ;
    }
}

MyFrame *
myframe_find( const char *name )
{
    ASHashData hdata;
	hdata.vptr = ASDefaultScr->Look.DefaultFrame ;
    if( name && ASDefaultScr->Look.FramesList )
        if( get_hash_item( ASDefaultScr->Look.FramesList, AS_HASHABLE(name), &hdata.vptr) != ASH_Success )
            return ASDefaultScr->Look.DefaultFrame ;
    return (MyFrame *)hdata.vptr ;
}

void
myframe_load ( MyFrame * frame, ASImageManager *imman )
{
	register int  i;

	if (frame == NULL)
		return;
	for (i = 0; i < FRAME_PARTS; i++)
        if( frame->part_filenames[i] )
        {
            frame->parts[i] = safecalloc( 1, sizeof(icon_t));
            if( !load_icon (frame->parts[i], frame->part_filenames[i], imman) )
            {
                free( frame->parts[i] );
                frame->parts[i] = NULL;
            }
#ifdef LOCAL_DEBUG
    LOCAL_DEBUG_OUT( "syncing %s","");
    ASSync(False);
#endif

        }
	for( i = 0 ; i < MYFRAME_TITLE_BACKS ; ++i )
	{
    	if( frame->title_back_filenames[i] )
    	{
        	frame->title_backs[i] = safecalloc( 1, sizeof(icon_t));
        	if( !load_icon (frame->title_backs[i], frame->title_back_filenames[i], imman) )
        	{
            	free( frame->title_backs[i] );
            	frame->title_backs[i] = NULL;
        	}
#ifdef LOCAL_DEBUG
	    	LOCAL_DEBUG_OUT( "syncing %s","");
    		ASSync(False);
#endif
		}
    }
}

Bool
filename2myframe_part (MyFrame *frame, int part, char *filename)
{
    char **dst = NULL ;
    if (filename && frame && part>= 0 && part < FRAME_PARTS)
        dst = &(frame->part_filenames[part]);
    if( dst )
    {
        if( *dst )
            free( *dst );
        *dst = mystrdup(filename);
        return True;
    }
    return False;
}

Bool
set_myframe_style (MyFrame *frame, unsigned int style, Bool title, char *stylename)
{
    char **dst = NULL ;
    if( style >= BACK_STYLES )
        style = BACK_DEFAULT ;
    if( title )
        dst = &(frame->title_style_names[style]);
    else
        dst = &(frame->frame_style_names[style]);
    if( dst )
    {
        if( *dst )
            free( *dst );
        *dst = mystrdup(stylename);
        return True;
    }
    return False;
}


Bool
myframe_has_parts(const MyFrame *frame, ASFlagType mask)
{
    if( frame )
    {
        register int i ;
        for( i = 0 ; i < FRAME_PARTS ; ++i )
		{	
            if( get_flags(mask,(0x01<<i)) )
			{	
				LOCAL_DEBUG_OUT( "i = %d", i );
                if( get_flags(frame->parts_mask,(0x01<<i)) )
				{	
					LOCAL_DEBUG_OUT( "parts[i] = %p", frame->parts[i] );
					if(frame->parts[i] || ((frame->part_width[i] || i >= FRAME_SIDES) && frame->part_length[i]))
					{	
	                    return True;
					}
				}
			}
		}
    }
    return False;
}

void
destroy_myframe( MyFrame **pframe )
{
    MyFrame *pf = *pframe ;
    if( pf )
    {
        register int i = FRAME_PARTS;
        while( --i >= 0 )
        {
            if( pf->parts[i] )
                destroy_icon( &(pf->parts[i]) );
            if( pf->part_filenames[i] )
                free( pf->part_filenames[i] );
        }
		for( i = 0 ; i < MYFRAME_TITLE_BACKS ; ++i )
		{
        	if( pf->title_backs[i] )
            	destroy_icon( &(pf->title_backs[i]) );
			if( pf->title_back_filenames[i] )
            	free( pf->title_back_filenames[i] );
		}

        for( i = 0 ; i < BACK_STYLES ; ++i )
        {
            if( pf->title_style_names[i] )
                free( pf->title_style_names[i] );
            if( pf->frame_style_names[i] )
                free( pf->frame_style_names[i] );
        }
        if( pf->name )
            free( pf->name );

        pf->magic = 0 ;
        free( pf );
        *pframe = NULL ;
    }
}

void check_myframes_list( MyLook *look )
{
    if( look->FramesList == NULL )
    {
        look->FramesList = create_ashash( 5, string_hash_value, string_compare, myobj_destroy );
    }
}


/*************************************************************************/

/**********************************************************************/
/******************** MyBackgrounds ***********************************/
MyBackground *create_myback( char *name )
{
	MyBackground *myback = safecalloc( 1, sizeof(MyBackground));
	myback->magic = MAGIC_MYBACKGROUND ;
    if( name )
        myback->name = mystrdup(name);
	return myback;
}

char *make_myback_image_name( MyLook *look, char *name )
{
	char *im_name = NULL ;
	/* creating unique fake filename so that it would not match any
	 * possible real filename : */
	
	if( name )
	{
		int len = strlen(name) ;
	 	im_name = safemalloc( len+1+4+2+1+4+2+1 );
		sprintf( im_name, "%s#%4.4dAB.%4.4dML", name, look->look_id%10000, look->deskback_id_base%10000 );
	}
	return im_name;
}

void myback_delete( MyBackground **myback, ASImageManager *imman )
{
	if( *myback )
	{
		if( (*myback)->name )
		{
            free( (*myback)->name );
			if( (*myback)->data )
				free( (*myback)->data );
		}
		if( (*myback)->loaded_im_name )
			free( (*myback)->loaded_im_name );
		if( (*myback)->loaded_pixmap )
		{	
			if( ASDefaultScr->RootBackground && ASDefaultScr->RootBackground->pmap == (*myback)->loaded_pixmap )
			{	
				ASDefaultScr->RootBackground->pmap = None;
			}
			destroy_visual_pixmap( ASDefaultVisual, &((*myback)->loaded_pixmap) );
		}
		(*myback)->magic = 0 ;
		free( *myback );
		*myback = NULL ;
	}
}

MyDesktopConfig *create_mydeskconfig( int desk, char *data )
{
    MyDesktopConfig *dc = safecalloc( 1, sizeof(MyDesktopConfig));
    dc->magic = MAGIC_MYDESKTOPCONFIG ;
    dc->desk = desk ;
    if( data )
        dc->back_name = mystrdup(data);
    return dc;
}


void mydeskconfig_delete( MyDesktopConfig **dc )
{
	if( *dc )
	{
		if( (*dc)->back_name )
			free( (*dc)->back_name );
		if( (*dc)->layout_name )
			free( (*dc)->layout_name );
		(*dc)->magic = 0 ;
		free( *dc );
		*dc = NULL ;
	}
}

void check_mybacks_list( MyLook *look )
{
    if( look->backs_list == NULL )
    {
        look->deskback_id_base++ ;
        look->backs_list = create_ashash( 5, string_hash_value, string_compare, myobj_destroy );
    }
}

inline MyBackground *
add_myback_to_list( ASHashTable *list, MyBackground *back, ASImageManager *imman )
{
	back->magic = MAGIC_MYBACKGROUND ;
	if( add_hash_item( list, (ASHashableValue)(back->name), back ) != ASH_Success )
		myback_delete( &back, imman );
	return back;
}

MyBackground *
add_myback( MyLook *look, MyBackground *back )
{
    check_mybacks_list( look );
    return add_myback_to_list( look->backs_list, back, ASDefaultScr->image_manager );
}


inline void init_deskconfigs_list( MyLook *look )
{
	look->desk_configs = create_ashash( 5, NULL, NULL, myobj_destroy );
}

inline MyDesktopConfig *
add_deskconfig_to_list( ASHashTable *list, MyDesktopConfig *dc )
{
	dc->magic = MAGIC_MYDESKTOPCONFIG ;
	if( add_hash_item( list, (ASHashableValue)(dc->desk), dc ) != ASH_Success )
		mydeskconfig_delete( &dc );
	return dc;
}

MyDesktopConfig *
add_deskconfig( MyLook *look, MyDesktopConfig *dc )
{
    if( look->desk_configs == NULL )
        init_deskconfigs_list( look );
    return add_deskconfig_to_list(look->desk_configs, dc );
}

/********************************************************************************/
/********************       MyLook      *****************************************/
/********************************************************************************/

MyStyle *
mylook_get_style(MyLook *look, const char *name)
{
    if( look )
        if( look->magic == MAGIC_MYLOOK )
            return mystyle_list_find_or_default (look->styles_list, name?name:"default");

    return NULL;
}

inline MyDesktopConfig *
mylook_get_desk_config(MyLook *look, long desk)
{
    if( look )
	{
		ASHashData hdata = {0} ;
		LOCAL_DEBUG_OUT( "looking for desk_config for dekstop %ld...", desk );
		if( get_hash_item( look->desk_configs, AS_HASHABLE(desk), &hdata.vptr) == ASH_Success )
		    return hdata.vptr ;
	}
	return NULL;
}


inline MyBackground *
mylook_get_desk_back(MyLook *look, long desk)
{
	MyBackground *myback = NULL ;
    if( look )
	{
		ASHashData hdata = {0} ;
LOCAL_DEBUG_OUT( "looking for desk_config for dekstop %ld...", desk );
		if( get_hash_item( look->desk_configs, AS_HASHABLE(desk), &hdata.vptr) == ASH_Success )
		{
		    MyDesktopConfig *dc = hdata.vptr ;
LOCAL_DEBUG_OUT( "found desk_config %p for dekstop %ld...", dc, desk );
			if( dc->back_name )
			{
#if defined(LOCAL_DEBUG) && !defined(NO_DEBUG_OUTPUT)
				print_ashash( look->backs_list, string_print );
#endif
				if( get_hash_item( look->backs_list, AS_HASHABLE(dc->back_name), &hdata.vptr) == ASH_Success )
					myback = hdata.vptr ;
				LOCAL_DEBUG_OUT( "found back %p for dekstop %ld with name \"%s\". data = \"%s\" ", myback, desk, dc->back_name, myback?(myback->data?myback->data:"<null>"):"<null>" );
			}
		}
	}
	return myback ;
}


inline MyBackground *
mylook_get_back(MyLook *look, char *name)
{
    ASHashData hdata = {0};
    if( look && name )
        if( get_hash_item( look->backs_list, AS_HASHABLE(name), &hdata.vptr) != ASH_Success )
			hdata.vptr = NULL ;
    return (MyBackground *)hdata.vptr ;
}

