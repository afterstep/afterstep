#ifndef COLORSCHEME_H_HEADER_DEFINED
#define COLORSCHEME_H_HEADER_DEFINED

/* Color scheme principles :
 *	Primary colors :
 *		We use 4 primary colors : Base(background), Inactive1, Inactive2 and Active
 *		Then we use 3 contrasting colors for InactiveText1, InactiveText2 and ActiveText
 *      -----------------
 * 		7 colors total.
 * 		It makes sense to use analogous color scheme with 4 complimetary color :
 * 		here is the color hues relationship :
 *
 * 		Inactive1	= Base + Angle
 * 		Inactive2	= Base - Angle
 * 		Active		= Base - 180    ( complimentary to Background )
 *
 * 		where Angle is preset constant in range of 10-60 ( default is 30 ).
 *
 * 		Brightness of each of these colors should not exceed 80%.
 *
 * 		For the text we use either witish or blackish color, which we calculate by
 *      either reducing saturation of the contrasting color to  5% ( to get witish color )
 *          or reducing brightness of the contrasting color to 30% ( to get blackish color )
 *
 * 		To determine if we need blackish or witish color we use this formula :
 * 		if( (0.5*Red+Green+0.3*Blue)>220 )
 * 			blackish
 * 		else
 * 			whitish
 *
 * 		We use the folowing formula to determine contrasting hues :
 * 		ActiveText	  = Base  ( same as background )
 * 		InactiveText1 = Inactive1 - 180
 * 		InactiveText2 = Inactive2 - 180
 *
 *
 * 		P.S. AS the matter of fact we only use 6 primary colors as ActiveText uses
 * 		same hue as the background.
 *
 * 	Secondary colors :
 *		Menu colors : HighInactive, HighActive
 * 			These should have same hue as Inactive1 and Active, but brightness
 *          at 10% higher.
 *		Menu text colors should be the same as InactiveText1 and ActiveText
 * 		Menu disabled item's text color: DisabledText
 *			This one should have same hue as Inactive1, but saturation reduced
 *          to 5% level.
 *
 * 	Derived gradient colors
 * 		Colors Background, Inactive1, Inactive2, Active, HighInactive, HighActive
 *      will have a 2 color gradient associated with them. To calculate these colors
 *      we add 10% and substract 10% to/from brightness of the respectable color.
 *  ____________________
 *  10 colors total: 6 primary colors and 4 derived colors
 */

typedef struct ASColorScheme
{
	int angle ;
	int base_hue, base_sat, base_val ;
	ARGB32       base_argb ;
	int inactive1_hue, inactive1_sat, inactive1_val ;
	ARGB32       inactive1_argb ;
	int inactive2_hue, inactive2_sat, inactive2_val ;
	ARGB32       inactive2_argb ;
	int active_hue, active_sat, active_val ;
	ARGB32       active_argb ;
	int inactive_text1_hue, inactive_text1_sat, inactive_text1_val ;
	ARGB32       inactive_text1_argb ;
	int inactive_text2_hue, inactive_text2_sat, inactive_text2_val ;
	ARGB32       inactive_text2_argb ;

	int 		 active_text_sat, active_text_val ;
	ARGB32       active_text_argb ;
	ARGB32       high_inactive_argb ;
	ARGB32       high_active_argb ;
	ARGB32       disabled_text_argb ;

	ARGB32       base_grad[2] ;
	ARGB32       inactive1_grad[2] ;
	ARGB32       inactive2_gradb[2] ;
	ARGB32       active_grad[2] ;
	ARGB32       high_inactive_grad[2] ;
	ARGB32       high_active_grad[2] ;
}ASColorScheme;

#define ASCS_MIN_ANGLE	0
#define ASCS_MAX_ANGLE	60
#define ASCS_DEFAULT_ANGLE	30

#define ASCS_MAX_BRIGHTNESS 100
#define ASCS_MAX_SATURATION 100
#define ASCS_MAX_HUE 		360
#define ASCS_MAX_COLD_HUE	300
#define ASCS_MIN_COLD_HUE	120

#define ASCS_COLD_SATURATION_OFFSET	 20

#define ASCS_MAX_PRIMARY_BRIGHTNESS 80
#define ASCS_MIN_PRIMARY_BRIGHTNESS 50
#define ASCS_MIN_PRIMARY_SATURATION 10

#define ASCS_WHITING_SATURATION_LEVEL	5
#define ASCS_BLACKING_BRIGHTNESS_LEVEL	30

#define ASCS_BLACK_O_WHITE_CRITERIA16(r16,g16,b16)  ((((r16)>>17)+((g16)>>16)+((b16)>>18))>214)

#define ASCS_HIGH_BRIGHTNESS_OFFSET	 		10
#define ASCS_DISABLED_SATURATION_LEVEL		5
#define ASCS_GRADIENT_BRIGHTNESS_OFFSET 	10

ASColorScheme *make_ascolor_scheme( ARGB32 base, int angle );

#endif COLORSCHEME_H_HEADER_DEFINED

