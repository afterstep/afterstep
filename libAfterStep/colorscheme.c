#define LOCAL_DEBUG
#undef DO_CLOCKING

#include "../configure.h"
#include "asapp.h"
#include "afterstep.h"
#include "../libAfterImage/afterimage.h"
#include "colorscheme.h"

char *ASMainColorNames[ASMC_MainColors] = {
	"Base",
	"Inactive1",
	"Inactive2",
	"Active",
	"InactiveText1",
	"InactiveText2",

	"ActiveText",
	"HighInactive",
	"HighActive",
	"HighInactiveBack",
	"HighActiveBack",
	"HighInactiveText",
	"HighActiveText",
	"DisabledText",

	"BaseDark",
	"BaseLight",
	"Inactive1Dark",
	"Inactive1Light",
	"Inactive2Dark",
	"Inactive2Light",
	"ActiveDark",
	"ActiveLight",
	"HighInactiveDark",
	"HighInactiveLight",
	"HighActiveDark",
	"HighActiveLight",
	"HighInactiveBackDark",
	"HighInactiveBackLight",
	"HighActiveBackDark",
	"HighActiveBackLight",
	"Cursor"
};

ARGB32
make_color_scheme_argb (ASColorScheme * cs, int id, CARD32 base_alpha16,
												CARD32 hue360, CARD32 sat100, CARD32 val100)
{
	CARD32 red16, green16, blue16;
	ARGB32 argb;

	if (val100 > 100)
		val100 = 100;
	if (sat100 > 100)
		sat100 = 100;


	hsv2rgb (degrees2hue16 (hue360), percent2val16 (sat100),
					 percent2val16 (val100), &red16, &green16, &blue16);
	argb =
			MAKE_ARGB32_CHAN16 (base_alpha16,
													ARGB32_ALPHA_CHAN) | MAKE_ARGB32_CHAN16 (red16,
																																	 ARGB32_RED_CHAN)
			| MAKE_ARGB32_CHAN16 (green16,
														ARGB32_GREEN_CHAN) |
			MAKE_ARGB32_CHAN16 (blue16, ARGB32_BLUE_CHAN);

	if (cs) {
		cs->main_hues[id] = hue360;
		cs->main_values[id] = val100;
		cs->main_saturations[id] = sat100;
		cs->main_colors[id] = argb;
	}
	return argb;
}

void make_color_scheme_hsv (ARGB32 argb, int *phue, int *psat, int *pval)
{
	CARD32 hue16, sat16, val16;

	hue16 =
			rgb2hsv (ARGB32_RED16 (argb), ARGB32_GREEN16 (argb),
							 ARGB32_BLUE16 (argb), &sat16, &val16);
	if (ARGB32_RED16 (argb) == ARGB32_GREEN16 (argb)
			&& ARGB32_GREEN16 (argb) == ARGB32_BLUE16 (argb)) {
		if (phue)
			*phue = -1;
		if (psat)
			*psat = -1;
	} else {
		if (phue)
			*phue = hue162degrees (hue16);
		if (psat)
			*psat = val162percent (sat16);
	}
	if (pval)
		*pval = val162percent (val16);
}

Bool is_light_hsv (int hue360, int sat100, int val100)
{
	CARD32 red16, green16, blue16;

	hsv2rgb (degrees2hue16 (hue360), percent2val16 (sat100),
					 percent2val16 (val100), &red16, &green16, &blue16);
	show_progress ("hsv(%d,%d,%d)->rgb16(%d,%d,%d)->crit%d", hue360, sat100,
								 val100, red16, green16, blue16,
								 ASCS_BLACK_O_WHITE_CRITERIA16_VAL (red16, green16,
																										blue16));
	return ASCS_BLACK_O_WHITE_CRITERIA16 (red16, green16, blue16);
}

int compare_color_lightness (ARGB32 c1, ARGB32 c2)
{
	CARD32 red1, green1, blue1;
	CARD32 red2, green2, blue2;

	red1 = ARGB32_RED16 (c1);
	green1 = ARGB32_GREEN16 (c1);
	blue1 = ARGB32_BLUE16 (c1);

	red2 = ARGB32_RED16 (c2);
	green2 = ARGB32_GREEN16 (c2);
	blue2 = ARGB32_BLUE16 (c2);

	return ASCS_BLACK_O_WHITE_CRITERIA16_VAL (red1, green1,
																						blue1) -
			ASCS_BLACK_O_WHITE_CRITERIA16_VAL (red2, green2, blue2);
}

inline void
make_grad_argb (ASColorScheme * cs, int id, ARGB32 base_alpha16,
								int hue360, int sat100, int val100, Bool base)
{
	int dark_val = val100 - ASCS_GRADIENT_BRIGHTNESS_OFFSET;
	int light_val = val100 + ASCS_GRADIENT_BRIGHTNESS_OFFSET;

	if (base) {
		dark_val -= ASCS_GRADIENT_BRIGHTNESS_OFFSET;
		light_val += ASCS_GRADIENT_BRIGHTNESS_OFFSET;
	}
	if (light_val > 100)
		light_val = 100;
	else if (light_val < 0)
		light_val = 0;
	if (dark_val > 100)
		dark_val = 100;
	else if (dark_val < 0)
		dark_val = 0;

	make_color_scheme_argb (cs, id, base_alpha16, hue360, sat100, dark_val);
	make_color_scheme_argb (cs, id + 1, base_alpha16, hue360, sat100,
													light_val);
}

static inline ARGB32 MAKE_ARGB32_SHADE100 (CARD32 a, int s100)
{
	CARD32 s8 = percent2val16 (s100) >> 8;

	LOCAL_DEBUG_OUT ("s100=%d, s8 = %4.4lX", s100, s8);
	return MAKE_ARGB32_GREY8 (a, s8);
}

inline void
make_mono_grad_argb (ARGB32 * grad, ARGB32 base_alpha16, int shade100,
										 int *val_ret)
{
	int dark_val = shade100 - ASCS_MONO_GRADIENT_OFFSET;
	int light_val = shade100 + ASCS_MONO_GRADIENT_OFFSET;

	if (light_val > 100)
		light_val = 100;
	else if (light_val < 0)
		light_val = 0;
	if (dark_val > 100)
		dark_val = 100;
	else if (dark_val < 0)
		dark_val = 0;

	grad[0] = MAKE_ARGB32_SHADE100 (base_alpha16, dark_val);
	grad[1] = MAKE_ARGB32_SHADE100 (base_alpha16, light_val);
	if (val_ret) {
		val_ret[0] = dark_val;
		val_ret[1] = light_val;
	}
}

inline int offset_shade (int shade, int offset, Bool rollover)
{
	shade += offset;
	if (shade > 100) {
		if (rollover)
			while (shade > 100)
				shade -= 100;
		else
			shade = 100;
	} else if (shade < 0) {
		if (rollover)
			while (shade < 0)
				shade += 100;
		else
			shade = 0;
	}
	return shade;
}

int make_text_shade (int background_shade)
{
	if (background_shade <= 100 - ASCS_MONO_TEXT_CONTRAST)
		return background_shade + ASCS_MONO_TEXT_CONTRAST;
	else if (background_shade >= ASCS_MONO_TEXT_CONTRAST)
		return background_shade - ASCS_MONO_TEXT_CONTRAST;
	else if (background_shade < 50)
		return 98;
	return 2;
}

ASColorScheme *make_mono_ascolor_scheme (ARGB32 base)
{
	ASColorScheme *cs = safecalloc (1, sizeof (ASColorScheme));
	int base_shade = val162percent (ARGB32_GREEN16 (base));
	int shade;
	CARD32 base_alpha16;
	int active_val, inactive1_val;

#ifndef DONT_CLAMP_BASE_COLOR
	if (base_shade < ASCS_MONO_MIN_BASE_SHADE)
		base_shade = ASCS_MONO_MIN_BASE_SHADE;
	else if (base_shade > ASCS_MONO_MAX_BASE_SHADE)
		base_shade = ASCS_MONO_MAX_BASE_SHADE;
#endif
	/* handling base color */
	base_alpha16 = ARGB32_ALPHA16 (base);

	cs->main_colors[ASMC_Base] =
			MAKE_ARGB32_SHADE100 (base_alpha16, base_shade);;
	cs->main_values[ASMC_Base] = base_shade;
	cs->angle = 0;

	make_mono_grad_argb (&(cs->main_colors[ASMC_BaseDark]), base_alpha16,
											 base_shade, &(cs->main_values[ASMC_BaseDark]));

	shade = offset_shade (base_shade, -ASCS_MONO_SIMILAR_OFFSET, False);
	LOCAL_DEBUG_OUT ("base_shade = %d, Inactive1 shade = %d", base_shade,
									 shade);
	cs->main_colors[ASMC_Inactive1] =
			MAKE_ARGB32_SHADE100 (base_alpha16, shade);
	cs->main_values[ASMC_Inactive1] = shade;
	inactive1_val = shade;
	LOCAL_DEBUG_OUT ("Inactive1 color = #%8.8lX",
									 cs->main_colors[ASMC_Inactive1]);
	make_mono_grad_argb (&(cs->main_colors[ASMC_Inactive1Dark]),
											 base_alpha16, shade,
											 &(cs->main_values[ASMC_Inactive1Dark]));
	shade = make_text_shade (shade);
	cs->main_colors[ASMC_InactiveText1] =
			MAKE_ARGB32_SHADE100 (base_alpha16, shade);
	cs->main_values[ASMC_InactiveText1] = shade;

	shade = offset_shade (base_shade, ASCS_MONO_SIMILAR_OFFSET, False);
	cs->main_colors[ASMC_Inactive2] =
			MAKE_ARGB32_SHADE100 (base_alpha16, shade);
	cs->main_values[ASMC_Inactive2] = shade;
	make_mono_grad_argb (&(cs->main_colors[ASMC_Inactive2Dark]),
											 base_alpha16, shade,
											 &(cs->main_values[ASMC_Inactive2Dark]));
	shade = make_text_shade (shade);
	cs->main_colors[ASMC_InactiveText2] =
			MAKE_ARGB32_SHADE100 (base_alpha16, shade);
	cs->main_values[ASMC_InactiveText2] = shade;


	shade = offset_shade (base_shade, ASCS_MONO_CONTRAST_OFFSET, True);
	cs->main_colors[ASMC_Active] =
			MAKE_ARGB32_SHADE100 (base_alpha16, shade);
	cs->main_values[ASMC_Active] = shade;
	cs->main_colors[ASMC_Cursor] = cs->main_colors[ASMC_Active];
	cs->main_values[ASMC_Cursor] = shade;
	make_mono_grad_argb (&(cs->main_colors[ASMC_ActiveDark]), base_alpha16,
											 shade, &(cs->main_values[ASMC_ActiveDark]));
	active_val = shade;
	shade = make_text_shade (shade);
	cs->main_colors[ASMC_ActiveText] =
			MAKE_ARGB32_SHADE100 (base_alpha16, shade);
	cs->main_values[ASMC_ActiveText] = shade;

	shade = offset_shade (inactive1_val, ASCS_MONO_HIGH_OFFSET, False);
	cs->main_colors[ASMC_HighInactive] =
			MAKE_ARGB32_SHADE100 (base_alpha16, shade);
	cs->main_values[ASMC_HighInactive] = shade;
	make_mono_grad_argb (&(cs->main_colors[ASMC_HighInactiveDark]),
											 base_alpha16, shade,
											 &(cs->main_values[ASMC_HighInactiveDark]));

	shade = offset_shade (active_val, ASCS_MONO_HIGH_OFFSET, False);
	cs->main_colors[ASMC_HighActive] =
			MAKE_ARGB32_SHADE100 (base_alpha16, shade);
	cs->main_values[ASMC_HighActive] = shade;
	make_mono_grad_argb (&(cs->main_colors[ASMC_HighActiveDark]),
											 base_alpha16, shade,
											 &(cs->main_values[ASMC_HighActiveDark]));

	shade = offset_shade (base_shade, ASCS_MONO_HIGH_OFFSET, False);
	cs->main_colors[ASMC_HighInactiveBack] =
			MAKE_ARGB32_SHADE100 (base_alpha16, shade);
	cs->main_values[ASMC_HighInactiveBack] = shade;
	make_mono_grad_argb (&(cs->main_colors[ASMC_HighInactiveBackDark]),
											 base_alpha16, shade,
											 &(cs->main_values[ASMC_HighInactiveBackDark]));

	shade = offset_shade (shade, ASCS_MONO_SIMILAR_OFFSET, False);
	cs->main_colors[ASMC_DisabledText] =
			MAKE_ARGB32_SHADE100 (base_alpha16, shade);
	cs->main_values[ASMC_DisabledText] = shade;

	shade = make_text_shade (shade);
	cs->main_colors[ASMC_HighInactiveText] =
			MAKE_ARGB32_SHADE100 (base_alpha16, shade);
	cs->main_values[ASMC_HighInactiveText] = shade;

	shade =
			offset_shade (base_shade,
										ASCS_MONO_CONTRAST_OFFSET - ASCS_MONO_HIGH_OFFSET,
										False);
	cs->main_colors[ASMC_HighActiveBack] =
			MAKE_ARGB32_SHADE100 (base_alpha16, shade);
	cs->main_values[ASMC_HighActiveBack] = shade;
	make_mono_grad_argb (&(cs->main_colors[ASMC_HighActiveBackDark]),
											 base_alpha16, shade,
											 &(cs->main_values[ASMC_HighActiveBackDark]));
	shade = make_text_shade (shade);
	cs->main_colors[ASMC_HighActiveText] =
			MAKE_ARGB32_SHADE100 (base_alpha16, shade);
	cs->main_values[ASMC_HighActiveText] = shade;

	/* all of the colors are computed by now */
	cs->set_main_colors = 0;
	return cs;
}

ASColorScheme *make_NeXTish_ascolor_scheme ()
{
	ASColorScheme *cs = safecalloc (1, sizeof (ASColorScheme));
	int i;

	cs->main_colors[ASMC_Base] = 0xFF555577;
	cs->main_colors[ASMC_BaseDark] = 0xFF444466;
	cs->main_colors[ASMC_BaseLight] = 0xFF666688;

	cs->main_colors[ASMC_Inactive1] = 0xFFBDBDBD;
	cs->main_colors[ASMC_Inactive1Dark] = 0xFFADADAD;
	cs->main_colors[ASMC_Inactive1Light] = 0xFFCDCDCD;
	cs->main_colors[ASMC_InactiveText1] = 0xFF101010;

	cs->main_colors[ASMC_Inactive2] = 0xFF8A8A99;
	cs->main_colors[ASMC_Inactive2Dark] = 0xFF7A7A89;
	cs->main_colors[ASMC_Inactive2Light] = 0xFF9A9AA9;
	cs->main_colors[ASMC_InactiveText2] = 0xFF101000;

	cs->main_colors[ASMC_Active] = 0xFF000033;
	cs->main_colors[ASMC_ActiveDark] = 0xFF000000;
	cs->main_colors[ASMC_Cursor] = cs->main_colors[ASMC_Active];
	cs->main_colors[ASMC_ActiveLight] = 0xFF000066;
	cs->main_colors[ASMC_ActiveText] = 0xFFF0F0E0;

	cs->main_colors[ASMC_HighInactive] = 0xFFCDCDCD;
	cs->main_colors[ASMC_HighInactiveDark] = 0xFFbDbDbD;
	cs->main_colors[ASMC_HighInactiveLight] = 0xFFdDdDdD;
	cs->main_colors[ASMC_HighInactiveBack] = 0xFFC0C0C0;
	cs->main_colors[ASMC_HighInactiveBackDark] = 0xFFb0b0b0;
	cs->main_colors[ASMC_HighInactiveBackLight] = 0xFFd0d0d0;

	cs->main_colors[ASMC_DisabledText] = 0xFFA0A0A0;
	cs->main_colors[ASMC_HighInactiveText] = 0xFF000000;

	cs->main_colors[ASMC_HighActive] = 0xFF101010;
	cs->main_colors[ASMC_HighActiveDark] = 0xFF000000;
	cs->main_colors[ASMC_HighActiveLight] = 0xFF202020;
	cs->main_colors[ASMC_HighActiveBack] = 0xFFEEEEEE;
	cs->main_colors[ASMC_HighActiveBackDark] = 0xFFdEdEdE;
	cs->main_colors[ASMC_HighActiveBackLight] = 0xFFfEfEfE;
	cs->main_colors[ASMC_HighActiveText] = 0xFF000000;

	for (i = 0; i < ASMC_MainColors; ++i)
		make_color_scheme_hsv (cs->main_colors[i], &(cs->main_hues[i]),
													 &(cs->main_saturations[i]),
													 &(cs->main_values[i]));

	/* all of the colors are set manually by now */
	cs->set_main_colors = 0xFFFFFFFF;
	return cs;
}


ASColorScheme *make_ascolor_scheme (ARGB32 base, int angle)
{
	ASColorScheme *cs;
	CARD32 hue16, sat16, val16;
	CARD32 base_alpha16;
	int sat, val;
	int base_val, base_sat, base_hue;
	int inactive1_hue, inactive1_sat, inactive1_val;
	int inactive2_hue, inactive2_sat, inactive2_val;
	int active_hue, active_sat, active_val;
	int inactive_text1_hue, inactive_text1_sat, inactive_text1_val;
	int inactive_text2_hue, inactive_text2_sat, inactive_text2_val;
	int active_text_sat, active_text_val;
	int high_inactive_text_sat, high_inactive_text_val;
	int high_active_text_sat, high_active_text_val;
	int pointer_hue, pointer_val;
	Bool active_light;

	/* handling base color */
	base_alpha16 = ARGB32_ALPHA16 (base);
	hue16 =
			rgb2hsv (ARGB32_RED16 (base), ARGB32_GREEN16 (base),
							 ARGB32_BLUE16 (base), &sat16, &val16);

	if (hue16 == -1 || (hue16 == 0 && sat16 == 0))
		return make_mono_ascolor_scheme (base);

	cs = safecalloc (1, sizeof (ASColorScheme));

	angle = FIT_IN_RANGE (ASCS_MIN_ANGLE, angle, ASCS_MAX_ANGLE);
	cs->angle = angle;

	sat = val162percent (sat16);
	LOCAL_DEBUG_OUT ("sat16 = %ld(0x%lX), sat = %d", sat16, sat16, sat);
	val = val162percent (val16);
	base_hue = hue162degrees (hue16);
	base_sat = sat;
	base_val = val;
#ifndef DONT_CLAMP_BASE_COLOR
	if (base_sat < ASCS_MIN_PRIMARY_SATURATION)
		base_sat = ASCS_MIN_PRIMARY_SATURATION;
	if (base_val < ASCS_MIN_PRIMARY_BRIGHTNESS)
		base_val = ASCS_MIN_PRIMARY_BRIGHTNESS;
	else if (base_val > ASCS_MAX_PRIMARY_BRIGHTNESS)
		base_val = ASCS_MAX_PRIMARY_BRIGHTNESS;
#endif
	make_color_scheme_argb (cs, ASMC_Base, base_alpha16, base_hue, base_sat,
													base_val);

	inactive1_hue = normalize_degrees_val (base_hue + angle);
	if (inactive1_hue > ASCS_MIN_COLD_HUE
			&& inactive1_hue < ASCS_MAX_COLD_HUE
			&& base_sat >
			ASCS_MIN_PRIMARY_SATURATION + ASCS_COLD_SATURATION_OFFSET)
		inactive1_sat = base_sat - ASCS_COLD_SATURATION_OFFSET;
	else
		inactive1_sat = base_sat;

	inactive1_val = base_val + ASCS_NORMAL_BRIGHTNESS_OFFSET;
	make_color_scheme_argb (cs, ASMC_Inactive1, base_alpha16, inactive1_hue,
													inactive1_sat, inactive1_val);

	inactive2_hue = normalize_degrees_val (base_hue - angle);
	if (inactive2_hue > ASCS_MIN_COLD_HUE
			&& inactive2_hue < ASCS_MAX_COLD_HUE
			&& base_sat >
			ASCS_MIN_PRIMARY_SATURATION + ASCS_COLD_SATURATION_OFFSET)
		inactive2_sat = base_sat - ASCS_COLD_SATURATION_OFFSET;
	else
		inactive2_sat = base_sat;
	inactive2_val = base_val + ASCS_NORMAL_BRIGHTNESS_OFFSET;
	make_color_scheme_argb (cs, ASMC_Inactive2, base_alpha16, inactive2_hue,
													inactive2_sat, inactive2_val);

	/* we want to make sure that Inactive2 is whiter then Inactive1 at all times */
	if (compare_color_lightness
			(cs->main_colors[ASMC_Inactive1],
			 cs->main_colors[ASMC_Inactive2]) > 0) {
		ARGB32 argb_tmp = cs->main_colors[ASMC_Inactive1];
		int itmp = inactive1_hue;

		inactive1_hue = inactive2_hue;
		inactive2_hue = itmp;
		itmp = inactive1_sat;
		inactive1_sat = inactive2_sat;
		inactive2_sat = itmp;
		itmp = inactive1_val;
		inactive1_val = inactive2_val;
		inactive2_val = itmp;
		cs->main_colors[ASMC_Inactive1] = cs->main_colors[ASMC_Inactive2];
		cs->main_colors[ASMC_Inactive2] = argb_tmp;
	}
	cs->main_hues[ASMC_Inactive1] = inactive1_hue;
	cs->main_saturations[ASMC_Inactive1] = inactive1_sat;
	cs->main_values[ASMC_Inactive1] = inactive1_val;
	cs->main_hues[ASMC_Inactive2] = inactive2_hue;
	cs->main_saturations[ASMC_Inactive2] = inactive2_sat;
	cs->main_values[ASMC_Inactive2] = inactive2_val;

	active_hue = normalize_degrees_val (base_hue - 180);
	active_sat = base_sat;
	active_val = base_val + ASCS_NORMAL_BRIGHTNESS_OFFSET;
	make_color_scheme_argb (cs, ASMC_Active, base_alpha16, active_hue,
													active_sat, active_val);

	inactive_text1_hue = normalize_degrees_val (inactive1_hue - 180);
	inactive_text1_sat = base_sat;
	inactive_text1_val = base_val;
	if (is_light_hsv (inactive1_hue, inactive1_sat, inactive1_val))
		inactive_text1_val = ASCS_BLACKING_BRIGHTNESS_LEVEL;
	else {
		if (inactive_text1_hue > ASCS_MIN_COLD_HUE
				&& inactive_text1_hue < ASCS_MAX_COLD_HUE)
			inactive_text1_sat = ASCS_WHITING_SATURATION_LEVEL;
		else
			inactive_text1_sat = 0;
		if (inactive_text1_val < ASCS_WHITING_MIN_BRIGHTNESS_LEVEL)
			inactive_text1_val = ASCS_WHITING_MIN_BRIGHTNESS_LEVEL;
	}
	make_color_scheme_argb (cs, ASMC_InactiveText1, base_alpha16,
													inactive_text1_hue, inactive_text1_sat,
													inactive_text1_val);

	inactive_text2_hue = normalize_degrees_val (inactive2_hue - 180);
	inactive_text2_sat = base_sat;
	inactive_text2_val = base_val;
	if (is_light_hsv (inactive2_hue, inactive2_sat, inactive2_val))
		inactive_text2_val = ASCS_BLACKING_BRIGHTNESS_LEVEL;
	else {
		if (inactive_text2_hue > ASCS_MIN_COLD_HUE
				&& inactive_text2_hue < ASCS_MAX_COLD_HUE)
			inactive_text2_sat = ASCS_WHITING_SATURATION_LEVEL;
		else
			inactive_text2_sat = 0;
		if (inactive_text2_val < ASCS_WHITING_MIN_BRIGHTNESS_LEVEL)
			inactive_text2_val = ASCS_WHITING_MIN_BRIGHTNESS_LEVEL;
	}
	make_color_scheme_argb (cs, ASMC_InactiveText2, base_alpha16,
													inactive_text2_hue, inactive_text2_sat,
													inactive_text2_val);

	active_text_sat = base_sat;
	active_text_val = base_val;
	if (is_light_hsv (active_hue, active_sat, active_val))
		active_text_val = ASCS_BLACKING_BRIGHTNESS_LEVEL;
	else {
		if (base_hue > ASCS_MIN_COLD_HUE && base_hue < ASCS_MAX_COLD_HUE)
			active_text_sat = ASCS_WHITING_SATURATION_LEVEL;
		else
			active_text_sat = 0;
		if (active_text_val < ASCS_WHITING_ACTV_MIN_BRIGHT_LEVEL)
			active_text_val = ASCS_WHITING_ACTV_MIN_BRIGHT_LEVEL;
	}
	make_color_scheme_argb (cs, ASMC_ActiveText, base_alpha16, base_hue,
													active_text_sat, active_text_val);

	pointer_hue =
			normalize_degrees_val ((base_hue > 120
															&& base_hue <
															340) ? base_hue - 90 : base_hue + 90);
	active_light = (active_text_val <= ASCS_BLACKING_BRIGHTNESS_LEVEL);
	if (is_light_hsv (base_hue, base_sat, base_val) && !active_light)
		pointer_val = 20;
	else if (active_light)
		pointer_val = 95;
	else
		pointer_val = 50;
	make_color_scheme_argb (cs, ASMC_Cursor, base_alpha16, pointer_hue, 90,
													pointer_val);


	make_color_scheme_argb (cs, ASMC_HighInactive, base_alpha16,
													inactive1_hue, inactive1_sat,
													inactive1_val + ASCS_HIGH_BRIGHTNESS_OFFSET);
	make_color_scheme_argb (cs, ASMC_HighActive, base_alpha16, active_hue,
													active_sat,
													active_val + ASCS_HIGH_BRIGHTNESS_OFFSET);
	make_color_scheme_argb (cs, ASMC_HighInactiveBack, base_alpha16,
													base_hue, base_sat,
													base_val + ASCS_HIGH_BRIGHTNESS_OFFSET);
	make_color_scheme_argb (cs, ASMC_HighActiveBack, base_alpha16,
													active_hue, active_sat,
													active_val - ASCS_HIGH_BRIGHTNESS_OFFSET);
	/* active hue */
	high_inactive_text_sat = base_sat;
	high_inactive_text_val = base_val + ASCS_HIGH_BRIGHTNESS_OFFSET;
	if (is_light_hsv
			(base_hue, base_sat, base_val + ASCS_HIGH_BRIGHTNESS_OFFSET))
		high_inactive_text_val = ASCS_BLACKING_BRIGHTNESS_LEVEL;
	else {
		if (active_hue > ASCS_MIN_COLD_HUE && active_hue < ASCS_MAX_COLD_HUE)
			high_inactive_text_sat = ASCS_WHITING_SATURATION_LEVEL;
		else
			high_inactive_text_sat = 0;
		if (high_inactive_text_val < ASCS_WHITING_MIN_BRIGHTNESS_LEVEL)
			high_inactive_text_val = ASCS_WHITING_MIN_BRIGHTNESS_LEVEL;
	}
	make_color_scheme_argb (cs, ASMC_HighInactiveText, base_alpha16,
													active_hue, high_inactive_text_sat,
													high_inactive_text_val);
	/* base hue */
	high_active_text_sat = active_sat;
	high_active_text_val = active_val + ASCS_HIGH_BRIGHTNESS_OFFSET;
	if (is_light_hsv
			(active_hue, active_sat, active_val - ASCS_HIGH_BRIGHTNESS_OFFSET))
		high_active_text_val = ASCS_BLACKING_BRIGHTNESS_LEVEL;
	else {
		if (base_hue > ASCS_MIN_COLD_HUE && base_hue < ASCS_MAX_COLD_HUE)
			high_active_text_sat = ASCS_WHITING_SATURATION_LEVEL;
		else
			high_active_text_sat = 0;
		if (high_active_text_val < ASCS_WHITING_ACTV_MIN_BRIGHT_LEVEL)
			high_active_text_val = ASCS_WHITING_ACTV_MIN_BRIGHT_LEVEL;
	}
	make_color_scheme_argb (cs, ASMC_HighActiveText, base_alpha16, base_hue,
													high_active_text_sat, high_active_text_val);

	if (base_val > 50)
		make_color_scheme_argb (cs, ASMC_DisabledText, base_alpha16, base_hue,
														(base_sat * 80) / 100,
														base_val -
														ASCS_GRADIENT_BRIGHTNESS_OFFSET / 2);
	else
		make_color_scheme_argb (cs, ASMC_DisabledText, base_alpha16, base_hue,
														(base_sat * 80) / 100,
														base_val -
														ASCS_GRADIENT_BRIGHTNESS_OFFSET / 2);

	make_grad_argb (cs, ASMC_BaseDark, base_alpha16, base_hue, base_sat,
									base_val, True);
	make_grad_argb (cs, ASMC_Inactive1Dark, base_alpha16, inactive1_hue,
									inactive1_sat, inactive1_val, False);
	make_grad_argb (cs, ASMC_Inactive2Dark, base_alpha16, inactive2_hue,
									inactive2_sat, inactive2_val, False);
	make_grad_argb (cs, ASMC_ActiveDark, base_alpha16, active_hue,
									active_sat, active_val, False);
	make_grad_argb (cs, ASMC_HighInactiveDark, base_alpha16, inactive1_hue,
									inactive1_sat,
									inactive1_val + ASCS_HIGH_BRIGHTNESS_OFFSET, False);
	make_grad_argb (cs, ASMC_HighActiveDark, base_alpha16, active_hue,
									active_sat, active_val + ASCS_HIGH_BRIGHTNESS_OFFSET,
									False);
	make_grad_argb (cs, ASMC_HighInactiveBackDark, base_alpha16, base_hue,
									base_sat, base_val + ASCS_HIGH_BRIGHTNESS_OFFSET, False);
	make_grad_argb (cs, ASMC_HighActiveBackDark, base_alpha16, active_hue,
									active_sat, active_val - ASCS_HIGH_BRIGHTNESS_OFFSET,
									False);


	/* all of the colors are computed by now */
	cs->set_main_colors = 0;

	return cs;
}

void populate_ascs_colors_rgb (ASColorScheme * cs)
{
	register int i;

	for (i = 0; i < ASMC_MainColors; ++i)
		register_custom_color (ASMainColorNames[i], cs->main_colors[i]);
}

void populate_ascs_colors_xml (ASColorScheme * cs)
{
	int i;
	static char tmp[256];

	for (i = 0; i < ASMC_MainColors; ++i) {
		sprintf (tmp, "ascs.%s.alpha", ASMainColorNames[i]);
		asxml_var_insert (tmp, ARGB32_ALPHA8 (cs->main_colors[i]));
		sprintf (tmp, "ascs.%s.red", ASMainColorNames[i]);
		asxml_var_insert (tmp, ARGB32_RED8 (cs->main_colors[i]));
		sprintf (tmp, "ascs.%s.green", ASMainColorNames[i]);
		asxml_var_insert (tmp, ARGB32_GREEN8 (cs->main_colors[i]));
		sprintf (tmp, "ascs.%s.blue", ASMainColorNames[i]);
		asxml_var_insert (tmp, ARGB32_BLUE8 (cs->main_colors[i]));

		sprintf (tmp, "ascs.%s.hue", ASMainColorNames[i]);
		asxml_var_insert (tmp, cs->main_hues[i]);
		sprintf (tmp, "ascs.%s.saturation", ASMainColorNames[i]);
		asxml_var_insert (tmp, cs->main_saturations[i]);
		sprintf (tmp, "ascs.%s.value", ASMainColorNames[i]);
		asxml_var_insert (tmp, cs->main_values[i]);
	}
}
