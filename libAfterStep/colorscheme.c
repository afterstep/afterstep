#define LOCAL_DEBUG
#undef DO_CLOCKING

#include "../configure.h"

#include "asapp.h"
#include "afterstep.h"
#include "../libAfterImage/afterimage.h"
#include "colorscheme.h"

char *ASMainColorNames[ASMC_MainColors] = {
	"Base"					,
	"Inactive1"				,
	"Inactive2"				,
	"Active"				,
	"InactiveText1"		    ,
	"InactiveText2"			,

	"ActiveText"			,
	"HighInactive"			,
	"HighActive"			,
	"HighInactiveBack"		,
	"HighActiveBack"	    ,
	"HighInactiveText"		,
	"HighActiveText"		,
	"DisabledText"			,

	"BaseDark"				,
	"BaseLight"				,
	"Inactive1Dark"			,
	"Inactive1Light"		,
	"Inactive2Dark"			,
	"Inactive2Light"		,
	"ActiveDark"			,
	"ActiveLight"			,
	"HighInactiveDark"		,
	"HighInactiveLight"		,
	"HighActiveDark"		,
	"HighActiveLight"		,
	"HighInactiveBackDark"	,
	"HighInactiveBackLight"	,
	"HighActiveBackDark"	,
	"HighActiveBackLight"
};


ARGB32
make_color_scheme_argb( CARD32 base_alpha16, CARD32 hue360, CARD32 sat100, CARD32 val100 )
{
	CARD32 red16, green16, blue16 ;
	ARGB32 argb ;

	if( val100 > 100 )
		val100 = 100 ;
	if( sat100 > 100 )
		sat100 = 100 ;

	hsv2rgb(degrees2hue16(hue360), percent2val16(sat100), percent2val16(val100), &red16, &green16, &blue16);
	argb = 	MAKE_ARGB32_CHAN16(base_alpha16,ARGB32_ALPHA_CHAN)|
			MAKE_ARGB32_CHAN16(red16,ARGB32_RED_CHAN)|
			MAKE_ARGB32_CHAN16(green16,ARGB32_GREEN_CHAN)|
			MAKE_ARGB32_CHAN16(blue16,ARGB32_BLUE_CHAN);
	return argb;
}

void
make_color_scheme_hsv( ARGB32 argb, int *phue, int *psat, int *pval )
{
	CARD32 hue16, sat16, val16 ;

	hue16 = rgb2hsv( ARGB32_RED16(argb), ARGB32_GREEN16(argb), ARGB32_BLUE16(argb), &sat16, &val16 );

	if( phue )
		*phue = hue162degrees( hue16 );
	if( psat )
		*psat = val162percent( sat16 );
	if( pval )
		*pval = val162percent( val16 );
}

Bool
is_light_hsv( int hue360, int sat100, int val100 )
{
	CARD32 red16, green16, blue16 ;

	hsv2rgb(degrees2hue16(hue360), percent2val16(sat100), percent2val16(val100), &red16, &green16, &blue16);
	show_progress( "hsv(%d,%d,%d)->rgb16(%d,%d,%d)->crit%d", hue360, sat100, val100, red16, green16, blue16, ASCS_BLACK_O_WHITE_CRITERIA16_VAL( red16, green16, blue16 ));
	return ASCS_BLACK_O_WHITE_CRITERIA16( red16, green16, blue16 );
}

int
compare_color_lightness( ARGB32 c1, ARGB32 c2 )
{
	CARD32 red1, green1, blue1 ;
	CARD32 red2, green2, blue2 ;

	red1   = ARGB32_RED16(c1);
	green1 = ARGB32_GREEN16(c1);
	blue1  = ARGB32_BLUE16(c1);

	red2   = ARGB32_RED16(c2);
	green2 = ARGB32_GREEN16(c2);
	blue2  = ARGB32_BLUE16(c2);

	return ASCS_BLACK_O_WHITE_CRITERIA16_VAL( red1, green1, blue1 )-ASCS_BLACK_O_WHITE_CRITERIA16_VAL( red2, green2, blue2 );
}

inline void
make_grad_argb( ARGB32 *grad, ARGB32 base_alpha16, int hue360, int sat100, int val100 )
{
	int dark_val = val100-ASCS_GRADIENT_BRIGHTNESS_OFFSET ;
	int light_val = val100+ASCS_GRADIENT_BRIGHTNESS_OFFSET ;
	if( light_val > 100 )
		light_val = 100 ;
	else if( light_val < 0 )
		light_val = 0 ;
	if( dark_val > 100 )
		dark_val = 100 ;
	else if( dark_val < 0 )
		dark_val = 0 ;

	grad[0] = make_color_scheme_argb( base_alpha16, hue360, sat100, dark_val );
	grad[1] = make_color_scheme_argb( base_alpha16, hue360, sat100, light_val );
}

inline void
make_mono_grad_argb( ARGB32 *grad, ARGB32 base_alpha16, int shade100 )
{
	int dark_val = shade100 - ASCS_MONO_GRADIENT_OFFSET ;
	int light_val = shade100 + ASCS_MONO_GRADIENT_OFFSET ;
	if( light_val > 100 )
		light_val = 100 ;
	else if( light_val < 0 )
		light_val = 0 ;
	if( dark_val > 100 )
		dark_val = 100 ;
	else if( dark_val < 0 )
		dark_val = 0 ;

	grad[0] = MAKE_ARGB32_GREY( base_alpha16, dark_val );
	grad[1] = MAKE_ARGB32_GREY( base_alpha16, light_val );
}

ASColorScheme *
make_mono_ascolor_scheme( ARGB32 base )
{
	ASColorScheme *cs = safecalloc( 1, sizeof(ASColorScheme));
	int base_shade = val162percent(ARGB32_GREEN16(base));
	int shade ;
	CARD32 base_alpha16 ;

	/* handling base color */
	base_alpha16 = ARGB32_ALPHA16(base);

	cs->main_colors[ASMC_Base] = base ;
	cs->angle = 0 ;

	make_mono_grad_argb( &(cs->main_colors[ASMC_BaseDark]), base_alpha16, base_shade );

	shade = base_shade - ASCS_MONO_SIMILAR_OFFSET ;
	cs->main_colors[ASMC_Inactive1] 	= MAKE_ARGB32_GREY(base_alpha16,shade);
	make_mono_grad_argb( &(cs->main_colors[ASMC_Inactive1Dark]), base_alpha16, shade );
	cs->inactive1_val = shade ;

	shade = base_shade + ASCS_MONO_SIMILAR_OFFSET ;
	cs->main_colors[ASMC_Inactive2] 	= MAKE_ARGB32_GREY(base_alpha16,shade);
	make_mono_grad_argb( &(cs->main_colors[ASMC_Inactive2Dark]), base_alpha16, shade );
	cs->inactive2_val = shade ;

	shade = base_shade + ASCS_MONO_CONTRAST_OFFSET ;
	cs->main_colors[ASMC_Active] 		= MAKE_ARGB32_GREY(base_alpha16,shade);
	make_mono_grad_argb( &(cs->main_colors[ASMC_ActiveDark]), base_alpha16, shade );
	cs->active_val = shade;

	shade = base_shade - ASCS_MONO_SIMILAR_OFFSET + ASCS_MONO_CONTRAST_OFFSET;
	cs->main_colors[ASMC_InactiveText1] = MAKE_ARGB32_GREY(base_alpha16,shade);
	cs->inactive_text1_val = shade ;

	shade = base_shade + ASCS_MONO_SIMILAR_OFFSET + ASCS_MONO_CONTRAST_OFFSET;
	cs->main_colors[ASMC_InactiveText2] = MAKE_ARGB32_GREY(base_alpha16,shade);
	cs->inactive_text2_val = shade ;

	shade = base_shade ;
	cs->main_colors[ASMC_ActiveText] 	= MAKE_ARGB32_GREY(base_alpha16,shade);
	cs->active_text_val = shade ;

	shade = base_shade - ASCS_MONO_SIMILAR_OFFSET + ASCS_MONO_HIGH_OFFSET;
	cs->main_colors[ASMC_HighInactive] 	= MAKE_ARGB32_GREY(base_alpha16,shade);
	make_mono_grad_argb( &(cs->main_colors[ASMC_HighInactiveDark]), base_alpha16, shade );

	shade = base_shade + ASCS_MONO_CONTRAST_OFFSET + ASCS_MONO_HIGH_OFFSET;
	cs->main_colors[ASMC_HighActive]   	= MAKE_ARGB32_GREY(base_alpha16,shade);
	make_mono_grad_argb( &(cs->main_colors[ASMC_ActiveDark]), base_alpha16, shade );

	shade = base_shade - ASCS_MONO_SIMILAR_OFFSET - ASCS_MONO_HIGH_OFFSET;
	cs->main_colors[ASMC_HighInactiveBack] 	= MAKE_ARGB32_GREY(base_alpha16,shade);
	make_mono_grad_argb( &(cs->main_colors[ASMC_HighInactiveBackDark]), base_alpha16, shade );

	shade = base_shade  + ASCS_MONO_CONTRAST_OFFSET - ASCS_MONO_HIGH_OFFSET;
	cs->main_colors[ASMC_HighActiveBack] 	= MAKE_ARGB32_GREY(base_alpha16,shade);
	make_mono_grad_argb( &(cs->main_colors[ASMC_HighActiveBackDark]), base_alpha16, shade );

	shade = base_shade - ASCS_MONO_SIMILAR_OFFSET - ASCS_MONO_HIGH_OFFSET + ASCS_MONO_CONTRAST_OFFSET ;
	cs->main_colors[ASMC_HighInactiveText] 	= MAKE_ARGB32_GREY(base_alpha16,shade);
	cs->high_inactive_text_val = shade ;

	shade = base_shade + ASCS_MONO_HIGH_OFFSET ;
	cs->main_colors[ASMC_HighActiveText] 	= MAKE_ARGB32_GREY(base_alpha16,shade);
	cs->high_active_text_val = shade ;

	shade = base_shade - ASCS_MONO_SIMILAR_OFFSET + ASCS_MONO_HIGH_OFFSET;
	cs->main_colors[ASMC_DisabledText] 	= MAKE_ARGB32_GREY(base_alpha16,shade);

	/* all of the colors are computed by now */
	cs->set_main_colors = 0 ;
	return cs;
}


ASColorScheme *
make_ascolor_scheme( ARGB32 base, int angle )
{
	ASColorScheme *cs;
	CARD32 hue16, sat16, val16 ;
	CARD32 base_alpha16 ;
	int sat, val;

	/* handling base color */
	base_alpha16 = ARGB32_ALPHA16(base);
	hue16 = rgb2hsv( ARGB32_RED16(base), ARGB32_GREEN16(base), ARGB32_BLUE16(base), &sat16, &val16 );

	if( hue16 == -1 || ( hue16 == 0 && sat16 == 0 )  )
		return make_mono_ascolor_scheme( base );

	cs = safecalloc( 1, sizeof(ASColorScheme));

	angle = FIT_IN_RANGE( ASCS_MIN_ANGLE, angle, ASCS_MAX_ANGLE );
	cs->angle = angle ;

	cs->base_hue = hue162degrees(hue16);
	sat = val162percent(sat16);
	val = val162percent(val16);
	cs->base_sat = max(sat,ASCS_MIN_PRIMARY_SATURATION);
	cs->base_val = FIT_IN_RANGE(ASCS_MIN_PRIMARY_BRIGHTNESS, val, ASCS_MAX_PRIMARY_BRIGHTNESS);
	cs->main_colors[ASMC_Base] = make_color_scheme_argb( base_alpha16, cs->base_hue, cs->base_sat, cs->base_val ) ;

	cs->inactive1_hue = normalize_degrees_val(cs->base_hue + angle) ;
	if( cs->inactive1_hue > ASCS_MIN_COLD_HUE && cs->inactive1_hue < ASCS_MAX_COLD_HUE &&
		cs->base_sat > ASCS_MIN_PRIMARY_SATURATION + ASCS_COLD_SATURATION_OFFSET )
		cs->inactive1_sat = cs->base_sat - ASCS_COLD_SATURATION_OFFSET ;
	else
		cs->inactive1_sat = cs->base_sat ;
	cs->inactive1_val = cs->base_val + ASCS_NORMAL_BRIGHTNESS_OFFSET  ;
	cs->main_colors[ASMC_Inactive1] = make_color_scheme_argb( base_alpha16, cs->inactive1_hue, cs->inactive1_sat, cs->inactive1_val );

	cs->inactive2_hue = normalize_degrees_val(cs->base_hue - angle);
	if( cs->inactive2_hue > ASCS_MIN_COLD_HUE && cs->inactive2_hue < ASCS_MAX_COLD_HUE &&
		cs->base_sat > ASCS_MIN_PRIMARY_SATURATION + ASCS_COLD_SATURATION_OFFSET )
		cs->inactive2_sat = cs->base_sat - ASCS_COLD_SATURATION_OFFSET ;
	else
		cs->inactive2_sat = cs->base_sat ;
	cs->inactive2_val = cs->base_val + ASCS_NORMAL_BRIGHTNESS_OFFSET ;
	cs->main_colors[ASMC_Inactive2] = make_color_scheme_argb( base_alpha16, cs->inactive2_hue, cs->inactive2_sat, cs->inactive2_val );

	/* we want to make sure that Inactive2 is whiter then Inactive1 at all times */
	if( compare_color_lightness( cs->main_colors[ASMC_Inactive1], cs->main_colors[ASMC_Inactive2] ) > 0 )
	{
		ARGB32 argb_tmp = cs->main_colors[ASMC_Inactive1] ;
		int itmp  = cs->inactive1_hue ;
		cs->inactive1_hue = cs->inactive2_hue ;
		cs->inactive2_hue = itmp ;
		itmp  = cs->inactive1_sat ;
		cs->inactive1_sat = cs->inactive2_sat ;
		cs->inactive2_sat = itmp ;
		itmp  = cs->inactive1_val ;
		cs->inactive1_val = cs->inactive2_val ;
		cs->inactive2_val = itmp ;
		cs->main_colors[ASMC_Inactive1] = cs->main_colors[ASMC_Inactive2] ;
		cs->main_colors[ASMC_Inactive2] = argb_tmp ;
	}

	cs->active_hue = normalize_degrees_val(cs->base_hue - 180);
	cs->active_sat = cs->base_sat ;
	cs->active_val = cs->base_val + ASCS_NORMAL_BRIGHTNESS_OFFSET  ;
	cs->main_colors[ASMC_Active] = make_color_scheme_argb( base_alpha16, cs->active_hue, cs->active_sat, cs->active_val );

	cs->inactive_text1_hue = normalize_degrees_val(cs->inactive1_hue - 180);
	cs->inactive_text1_sat = cs->base_sat ;
	cs->inactive_text1_val = cs->base_val ;
	if( is_light_hsv(cs->inactive1_hue, cs->inactive1_sat, cs->inactive1_val) )
		cs->inactive_text1_val = ASCS_BLACKING_BRIGHTNESS_LEVEL ;
	else
	{
		if( cs->inactive_text1_hue > ASCS_MIN_COLD_HUE && cs->inactive_text1_hue < ASCS_MAX_COLD_HUE )
			cs->inactive_text1_sat = ASCS_WHITING_SATURATION_LEVEL ;
		else
			cs->inactive_text1_sat = 0 ;
		if( cs->inactive_text1_val < ASCS_WHITING_MIN_BRIGHTNESS_LEVEL )
			cs->inactive_text1_val = ASCS_WHITING_MIN_BRIGHTNESS_LEVEL ;
	}
	cs->main_colors[ASMC_InactiveText1] = make_color_scheme_argb( base_alpha16, cs->inactive_text1_hue, cs->inactive_text1_sat, cs->inactive_text1_val );

	cs->inactive_text2_hue = normalize_degrees_val(cs->inactive2_hue - 180);
	cs->inactive_text2_sat = cs->base_sat ;
	cs->inactive_text2_val = cs->base_val ;
	if( is_light_hsv(cs->inactive2_hue, cs->inactive2_sat, cs->inactive2_val) )
		cs->inactive_text2_val = ASCS_BLACKING_BRIGHTNESS_LEVEL ;
	else
	{
		if( cs->inactive_text2_hue > ASCS_MIN_COLD_HUE && cs->inactive_text2_hue < ASCS_MAX_COLD_HUE )
			cs->inactive_text2_sat = ASCS_WHITING_SATURATION_LEVEL ;
		else
			cs->inactive_text2_sat = 0 ;
		if( cs->inactive_text2_val < ASCS_WHITING_MIN_BRIGHTNESS_LEVEL )
			cs->inactive_text2_val = ASCS_WHITING_MIN_BRIGHTNESS_LEVEL ;
	}
	cs->main_colors[ASMC_InactiveText2] = make_color_scheme_argb( base_alpha16, cs->inactive_text2_hue, cs->inactive_text2_sat, cs->inactive_text2_val );

	cs->active_text_sat = cs->base_sat ;
	cs->active_text_val = cs->base_val ;
	if( is_light_hsv(cs->active_hue, cs->active_sat, cs->active_val) )
		cs->active_text_val = ASCS_BLACKING_BRIGHTNESS_LEVEL ;
	else
	{
		if( cs->base_hue > ASCS_MIN_COLD_HUE && cs->base_hue < ASCS_MAX_COLD_HUE )
			cs->active_text_sat = ASCS_WHITING_SATURATION_LEVEL ;
		else
			cs->active_text_sat = 0 ;
		if( cs->active_text_val < ASCS_WHITING_ACTV_MIN_BRIGHT_LEVEL )
			cs->active_text_val = ASCS_WHITING_ACTV_MIN_BRIGHT_LEVEL ;
	}
	cs->main_colors[ASMC_ActiveText] = make_color_scheme_argb( base_alpha16, cs->base_hue, cs->active_text_sat, cs->active_text_val );

	cs->main_colors[ASMC_HighInactive] = make_color_scheme_argb( base_alpha16, cs->inactive1_hue, cs->inactive1_sat, cs->inactive1_val + ASCS_HIGH_BRIGHTNESS_OFFSET);
	cs->main_colors[ASMC_HighActive]   = make_color_scheme_argb( base_alpha16, cs->active_hue, cs->active_sat, cs->active_val + ASCS_HIGH_BRIGHTNESS_OFFSET);
	cs->main_colors[ASMC_HighInactiveBack] = make_color_scheme_argb( base_alpha16, cs->base_hue, cs->base_sat, cs->base_val + ASCS_HIGH_BRIGHTNESS_OFFSET); ;
	cs->main_colors[ASMC_HighActiveBack] = make_color_scheme_argb( base_alpha16, cs->active_hue, cs->active_sat, cs->active_val - ASCS_HIGH_BRIGHTNESS_OFFSET);
	/* active hue */
	cs->high_inactive_text_sat = cs->base_sat ;
	cs->high_inactive_text_val = cs->base_val + ASCS_HIGH_BRIGHTNESS_OFFSET ;
	if( is_light_hsv(cs->base_hue, cs->base_sat, cs->base_val+ASCS_HIGH_BRIGHTNESS_OFFSET) )
		cs->high_inactive_text_val = ASCS_BLACKING_BRIGHTNESS_LEVEL ;
	else
	{
		if( cs->active_hue > ASCS_MIN_COLD_HUE && cs->active_hue < ASCS_MAX_COLD_HUE )
			cs->high_inactive_text_sat = ASCS_WHITING_SATURATION_LEVEL ;
		else
			cs->high_inactive_text_sat = 0 ;
		if( cs->high_inactive_text_val < ASCS_WHITING_MIN_BRIGHTNESS_LEVEL )
			cs->high_inactive_text_val = ASCS_WHITING_MIN_BRIGHTNESS_LEVEL ;
	}
	cs->main_colors[ASMC_HighInactiveText] = make_color_scheme_argb( base_alpha16, cs->active_hue, cs->high_inactive_text_sat, cs->high_inactive_text_val);
	/* base hue */
	cs->high_active_text_sat = cs->active_sat ;
	cs->high_active_text_val = cs->active_val + ASCS_HIGH_BRIGHTNESS_OFFSET ;
	if( is_light_hsv(cs->active_hue, cs->active_sat, cs->active_val-ASCS_HIGH_BRIGHTNESS_OFFSET) )
		cs->high_active_text_val = ASCS_BLACKING_BRIGHTNESS_LEVEL ;
	else
	{
		if( cs->base_hue > ASCS_MIN_COLD_HUE && cs->base_hue < ASCS_MAX_COLD_HUE )
			cs->high_active_text_sat = ASCS_WHITING_SATURATION_LEVEL ;
		else
			cs->high_active_text_sat = 0 ;
		if( cs->high_active_text_val < ASCS_WHITING_ACTV_MIN_BRIGHT_LEVEL )
			cs->high_active_text_val = ASCS_WHITING_ACTV_MIN_BRIGHT_LEVEL ;
	}
	cs->main_colors[ASMC_HighActiveText] = make_color_scheme_argb( base_alpha16, cs->base_hue, cs->high_active_text_sat, cs->high_active_text_val);

	if( cs->base_sat  >  ASCS_DISABLED_SATURATION_LEVEL )
		cs->main_colors[ASMC_DisabledText] = make_color_scheme_argb( base_alpha16, cs->base_hue, cs->base_sat - ASCS_DISABLED_SATURATION_LEVEL, cs->high_inactive_text_val);
	else
		cs->main_colors[ASMC_DisabledText] = make_color_scheme_argb( base_alpha16, cs->base_hue, cs->base_sat + ASCS_DISABLED_SATURATION_LEVEL, cs->high_inactive_text_val);

	make_grad_argb( &(cs->main_colors[ASMC_BaseDark]), base_alpha16, cs->base_hue, cs->base_sat, cs->base_val );
	make_grad_argb( &(cs->main_colors[ASMC_Inactive1Dark]), base_alpha16, cs->inactive1_hue, cs->inactive1_sat, cs->inactive1_val );
	make_grad_argb( &(cs->main_colors[ASMC_Inactive2Dark]), base_alpha16, cs->inactive2_hue, cs->inactive2_sat, cs->inactive2_val );
	make_grad_argb( &(cs->main_colors[ASMC_ActiveDark]), base_alpha16, cs->active_hue, cs->active_sat, cs->active_val );
	make_grad_argb( &(cs->main_colors[ASMC_HighInactiveDark]), base_alpha16, cs->inactive1_hue, cs->inactive1_sat, cs->inactive1_val + ASCS_HIGH_BRIGHTNESS_OFFSET );
	make_grad_argb( &(cs->main_colors[ASMC_ActiveDark]), base_alpha16, cs->active_hue, cs->active_sat, cs->active_val + ASCS_HIGH_BRIGHTNESS_OFFSET );
	make_grad_argb( &(cs->main_colors[ASMC_HighInactiveBackDark]), base_alpha16, cs->base_hue, cs->base_sat, cs->base_val + ASCS_HIGH_BRIGHTNESS_OFFSET );
	make_grad_argb( &(cs->main_colors[ASMC_HighActiveBackDark]), base_alpha16, cs->active_hue, cs->active_sat, cs->active_val - ASCS_HIGH_BRIGHTNESS_OFFSET );

	/* all of the colors are computed by now */
	cs->set_main_colors = 0 ;

	return cs;
}

void
populate_ascs_colors_rgb( ASColorScheme *cs )
{
	register int i ;
	for( i = 0 ; i < ASMC_MainColors ; ++i )
		register_custom_color( ASMainColorNames[i], cs->main_colors[i] );
}

void
populate_ascs_colors_xml( ASColorScheme *cs )
{
	asxml_var_insert( "ascs.angle", cs->angle );
	asxml_var_insert( "ascs.base_hue", cs->base_hue );
	asxml_var_insert( "ascs.base_saturation", cs->base_sat );
	asxml_var_insert( "ascs.base_value", cs->base_val );
	asxml_var_insert( "ascs.inactive1_hue", cs->inactive1_hue );
	asxml_var_insert( "ascs.inactive1_saturation", cs->inactive1_sat );
	asxml_var_insert( "ascs.inactive1_value", cs->inactive1_val );
	asxml_var_insert( "ascs.inactive2_hue", cs->inactive2_hue );
	asxml_var_insert( "ascs.inactive2_saturation", cs->inactive2_sat );
	asxml_var_insert( "ascs.inactive2_value", cs->inactive2_val );
	asxml_var_insert( "ascs.active_hue", cs->active_hue );
	asxml_var_insert( "ascs.active_saturation", cs->active_sat );
	asxml_var_insert( "ascs.active_value", cs->active_val );
	asxml_var_insert( "ascs.inactive_text1_hue", cs->inactive_text1_hue );
	asxml_var_insert( "ascs.inactive_text1_saturation", cs->inactive_text1_sat );
	asxml_var_insert( "ascs.inactive_text1_value", cs->inactive_text1_val );
	asxml_var_insert( "ascs.inactive_text2_hue", cs->inactive_text2_hue );
	asxml_var_insert( "ascs.inactive_text2_saturation", cs->inactive_text2_sat );
	asxml_var_insert( "ascs.inactive_text2_value", cs->inactive_text2_val );
	asxml_var_insert( "ascs.active_text_saturation", cs->active_text_sat );
	asxml_var_insert( "ascs.active_text_value", cs->active_text_val );
	asxml_var_insert( "ascs.high_inactive_text_saturation", cs->high_inactive_text_sat );
	asxml_var_insert( "ascs.high_inactive_text_value", cs->high_inactive_text_val );
	asxml_var_insert( "ascs.high_active_text_saturation", cs->high_active_text_sat );
	asxml_var_insert( "ascs.high_active_text_value", cs->high_active_text_val );

}


