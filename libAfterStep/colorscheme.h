#ifndef COLORSCHEME_H_HEADER_DEFINED
#define COLORSCHEME_H_HEADER_DEFINED

#include "../libAfterImage/afterimage.h"

#ifdef __cplusplus
extern "C" {
#endif

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

#define ASMC_Base 					0
#define ASMC_Inactive1 				1
#define ASMC_Inactive2 				2
#define ASMC_Active 				3
#define ASMC_InactiveText1 		    4
#define ASMC_InactiveText2 			5

#define ASMC_ActiveText 			6
#define ASMC_HighInactive 			7
#define ASMC_HighActive 			8
#define ASMC_HighInactiveBack 		9
#define ASMC_HighActiveBack 	    10
#define ASMC_HighInactiveText 		11
#define ASMC_HighActiveText			12
#define ASMC_DisabledText			13

#define ASMC_BaseDark				14
#define ASMC_BaseLight				15
#define ASMC_Inactive1Dark			16
#define ASMC_Inactive1Light			17
#define ASMC_Inactive2Dark			18
#define ASMC_Inactive2Light			19
#define ASMC_ActiveDark				20
#define ASMC_ActiveLight			21
#define ASMC_HighInactiveDark		22
#define ASMC_HighInactiveLight		23
#define ASMC_HighActiveDark			24
#define ASMC_HighActiveLight		25
#define ASMC_HighInactiveBackDark	26
#define ASMC_HighInactiveBackLight	27
#define ASMC_HighActiveBackDark		28
#define ASMC_HighActiveBackLight   	29
#define ASMC_MainColors			   	30


extern char *ASMainColorNames[ASMC_MainColors];

typedef struct ASColorScheme
{
	int angle ;
	int base_hue, base_sat, base_val ;
	int inactive1_hue, inactive1_sat, inactive1_val ;
	int inactive2_hue, inactive2_sat, inactive2_val ;
	int active_hue, active_sat, active_val ;
	int inactive_text1_hue, inactive_text1_sat, inactive_text1_val ;
	int inactive_text2_hue, inactive_text2_sat, inactive_text2_val ;

	int 		 active_text_sat, active_text_val ;
	int 		 high_inactive_text_sat, high_inactive_text_val ;
	int 		 high_active_text_sat, high_active_text_val ;

	ASFlagType   set_main_colors ;             /* set bits represent colors
												* set manually, instead of
												* being computed from the
												* base color */
	ARGB32       main_colors[ASMC_MainColors] ;

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

#define ASCS_MAX_PRIMARY_BRIGHTNESS 70
#define ASCS_MIN_PRIMARY_BRIGHTNESS 50
#define ASCS_MIN_PRIMARY_SATURATION 10

#define ASCS_WHITING_SATURATION_LEVEL	5
#define ASCS_WHITING_MIN_BRIGHTNESS_LEVEL	75
#define ASCS_WHITING_ACTV_MIN_BRIGHT_LEVEL	90
#define ASCS_BLACKING_BRIGHTNESS_LEVEL	30

#define ASCS_BLACK_O_WHITE_CRITERIA16_VAL(r16,g16,b16)  (((r16)>>9)+((g16)>>8)+(((b16)*3)/2560))
#define ASCS_BLACK_O_WHITE_CRITERIA16(r16,g16,b16)  ((((r16)>>9)+((g16)>>8)+(((b16)*3)/2560))>220)

#define ASCS_NORMAL_BRIGHTNESS_OFFSET	 	10
#define ASCS_HIGH_BRIGHTNESS_OFFSET	 		20
#define ASCS_DISABLED_SATURATION_LEVEL		40
#define ASCS_GRADIENT_BRIGHTNESS_OFFSET 	20

#define ASCS_MONO_MIN_SHADE					10
#define ASCS_MONO_MAX_SHADE					90
#define ASCS_MONO_MIN_BASE_SHADE   			20
#define ASCS_MONO_MAX_BASE_SHADE   			80
#define ASCS_MONO_CONTRAST_OFFSET			45
#define ASCS_MONO_TEXT_CONTRAST				65
#define ASCS_MONO_SIMILAR_OFFSET			15
#define ASCS_MONO_GRADIENT_OFFSET			 5
#define ASCS_MONO_HIGH_OFFSET			     5



ARGB32 make_color_scheme_argb( CARD32 base_alpha16, CARD32 hue360, CARD32 sat100, CARD32 val100 );
void make_color_scheme_hsv( ARGB32 argb, int *phue, int *psat, int *pval );

ASColorScheme *make_ascolor_scheme( ARGB32 base, int angle );
void populate_ascs_colors_rgb( ASColorScheme *cs );
void populate_ascs_colors_xml( ASColorScheme *cs );


#ifdef __cplusplus
}
#endif



#endif COLORSCHEME_H_HEADER_DEFINED

