#ifndef COLORSCHEME_H_HEADER_DEFINED
#define COLORSCHEME_H_HEADER_DEFINED

#define LOCAL_DEBUG
#undef DO_CLOCKING

#include "../configure.h"

#include "asapp.h"
#include "afterstep.h"
#include "../libAfterImage/afterimage.h"

ASColorScheme *
make_ascolor_scheme( ARGB32 base, unsigned int angle )
{
	ASColorScheme *cs = safecalloc( 1, sizeof(ASColorScheme));
	CARD32 hue16, sat16, val16 ;
	unsigned int hue, sat, val;

	angle = FIT_IN_RANGE( ASCS_MIN_ANGLE, angle, ASCS_MAX_ANGLE );
	cs->angle = angle ;
	/* handling base color */
	cs->base_argb = base ;
	hue16 = rgb2hsv( ARGB32_RED16(base), ARGB32_GREEN16(base), ARGB32_BLUE16(base), &sat16, &val16 );
	cs->base_hue = hue162degrees(hue16);
	sat = val162percent(sat16);
	val = val162percent(val16);
	cs->base_sat = max(sat,ASCS_MIN_PRIMARY_SATURATION);
	cs->base_val = FIT_IN_RANGE(ASCS_MIN_PRIMARY_BRIGHTNESS, val, ASCS_MAX_PRIMARY_BRIGHTNESS);

	cs->
	inactive1_hue, inactive1_sat, inactive1_val ;
	inactive1_argb ;
	inactive2_hue, inactive2_sat, inactive2_val ;
	inactive2_argb ;
	active_hue, active_sat, active_val ;
	active_argb ;
	inactive_text1_hue, inactive_text1_sat, inactive_text1_val ;
	inactive_text1_argb ;
	inactive_text2_hue, inactive_text2_sat, inactive_text2_val ;
	inactive_text2_argb ;

	active_text_argb ;
	high_inactive_argb ;
	high_active_argb ;
	disabled_text_argb ;

	base_grad[2] ;
	inactive1_grad[2] ;
	inactive2_gradb[2] ;
	active_grad[2] ;
	high_inactive_grad[2] ;
	high_active_grad[2] ;



}




#endif COLORSCHEME_H_HEADER_DEFINED

