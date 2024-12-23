/*
 * config.c - This file is part of mxim
 * Copyright (C) 2024 Matthias Kruk
 *
 * Mxim is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 3, or (at your
 * option) any later version.
 *
 * Mxim is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Mxim; see the file COPYING.  If not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#define CONFIG_C
#include "config.h"
#include "cmd.h"
#include "keysym.h"
#include "char.h"
#include <errno.h>

struct keymap_layer {
	char_t keys[255];
};

struct keymap {
	struct keymap_layer layers[2];
};

cmd_def_t config_keybindings[255][16] = {
	[KEY_F1] = {
		[MOD_ALT]  =  { .cmd = CMD_LANG_SWITCH, .arg = { .u = LANG_JA } },
	},
	[KEY_F2] = {
		[MOD_ALT]  =  { .cmd = CMD_LANG_SWITCH, .arg = { .u = LANG_KR } },
	},
	[KEY_F3] = {
		[MOD_ALT]  =  { .cmd = CMD_LANG_SWITCH, .arg = { .u = LANG_EN } },
	},
	[KEY_RETURN] = {
		[MOD_ALT]   = { .cmd = CMD_COMMIT },
		[MOD_SHIFT] = { .cmd = CMD_COMMIT },
	},
};

static const struct keymap _config_keymap_en = {
	.layers = {
		[MOD_NONE] = {
			.keys = {
				[KEY_1]          = CHAR_1,
				[KEY_2]          = CHAR_2,
				[KEY_3]          = CHAR_3,
				[KEY_4]          = CHAR_4,
				[KEY_5]          = CHAR_5,
				[KEY_6]          = CHAR_6,
				[KEY_7]          = CHAR_7,
				[KEY_8]          = CHAR_8,
				[KEY_9]          = CHAR_9,
				[KEY_0]          = CHAR_0,
				[KEY_MINUS]      = CHAR_MINUS,
				[KEY_CIRCUMFLEX] = CHAR_CIRCUMFLEX,
				[KEY_YEN]        = CHAR_BACKSLASH,
				[KEY_Q]          = CHAR_q,
				[KEY_W]          = CHAR_w,
				[KEY_E]          = CHAR_e,
				[KEY_R]          = CHAR_r,
				[KEY_T]          = CHAR_t,
				[KEY_Y]          = CHAR_y,
				[KEY_U]          = CHAR_u,
				[KEY_I]          = CHAR_i,
				[KEY_O]          = CHAR_o,
				[KEY_P]          = CHAR_p,
				[KEY_AT]         = CHAR_AT,
				[KEY_LBRACKET]   = CHAR_LBRACKET,
				[KEY_A]          = CHAR_a,
				[KEY_S]          = CHAR_s,
				[KEY_D]          = CHAR_d,
				[KEY_F]          = CHAR_f,
				[KEY_G]          = CHAR_g,
				[KEY_H]          = CHAR_h,
				[KEY_J]          = CHAR_j,
				[KEY_K]          = CHAR_k,
				[KEY_L]          = CHAR_l,
				[KEY_SEMICOLON]  = CHAR_SEMICOLON,
				[KEY_COLON]      = CHAR_COLON,
				[KEY_RBRACKET]   = CHAR_RBRACKET,
				[KEY_Z]          = CHAR_z,
				[KEY_X]          = CHAR_x,
				[KEY_C]          = CHAR_c,
				[KEY_V]          = CHAR_v,
				[KEY_B]          = CHAR_b,
				[KEY_N]          = CHAR_n,
				[KEY_M]          = CHAR_m,
				[KEY_COMMA]      = CHAR_COMMA,
				[KEY_PERIOD]     = CHAR_PERIOD,
				[KEY_SLASH]      = CHAR_SLASH,
				[KEY_BACKSLASH]  = CHAR_BACKSLASH
			}
		},
		[MOD_SHIFT] = {
			.keys = {
				[KEY_1]          = CHAR_EXCLAM,
				[KEY_2]          = CHAR_DQUOTE,
				[KEY_3]          = CHAR_POUND,
				[KEY_4]          = CHAR_DOLLAR,
				[KEY_5]          = CHAR_PERCENT,
				[KEY_6]          = CHAR_AMPERSAND,
				[KEY_7]          = CHAR_QUOTE,
				[KEY_8]          = CHAR_LPAREN,
				[KEY_9]          = CHAR_RPAREN,
				[KEY_0]          = CHAR_TILDA,
				[KEY_MINUS]      = CHAR_EQ,
				[KEY_CIRCUMFLEX] = CHAR_TILDA,
				[KEY_YEN]        = CHAR_PIPE,
				[KEY_Q]          = CHAR_Q,
				[KEY_W]          = CHAR_W,
				[KEY_E]          = CHAR_E,
				[KEY_R]          = CHAR_R,
				[KEY_T]          = CHAR_T,
				[KEY_Y]          = CHAR_Y,
				[KEY_U]          = CHAR_U,
				[KEY_I]          = CHAR_I,
				[KEY_O]          = CHAR_O,
				[KEY_P]          = CHAR_P,
				[KEY_AT]         = CHAR_BACKTICK,
				[KEY_LBRACKET]   = CHAR_LBRACE,
				[KEY_A]          = CHAR_A,
				[KEY_S]          = CHAR_S,
				[KEY_D]          = CHAR_D,
				[KEY_F]          = CHAR_F,
				[KEY_G]          = CHAR_G,
				[KEY_H]          = CHAR_H,
				[KEY_J]          = CHAR_J,
				[KEY_K]          = CHAR_K,
				[KEY_L]          = CHAR_L,
				[KEY_SEMICOLON]  = CHAR_PLUS,
				[KEY_COLON]      = CHAR_ASTERISK,
				[KEY_RBRACKET]   = CHAR_RBRACE,
				[KEY_Z]          = CHAR_Z,
				[KEY_X]          = CHAR_X,
				[KEY_C]          = CHAR_C,
				[KEY_V]          = CHAR_V,
				[KEY_B]          = CHAR_B,
				[KEY_N]          = CHAR_N,
				[KEY_M]          = CHAR_M,
				[KEY_COMMA]      = CHAR_LT,
				[KEY_PERIOD]     = CHAR_GT,
				[KEY_SLASH]      = CHAR_QMARK,
				[KEY_BACKSLASH]  = CHAR_UNDERSCORE
			}
		}
	}
};

static const struct keymap _config_keymap_ja = {
	.layers = {
		[MOD_NONE] = {
			.keys = {
				[KEY_1]          = CHAR_JA_NU,
				[KEY_2]          = CHAR_JA_HU,
				[KEY_3]          = CHAR_JA_A,
				[KEY_4]          = CHAR_JA_U,
				[KEY_5]          = CHAR_JA_E,
				[KEY_6]          = CHAR_JA_O,
				[KEY_7]          = CHAR_JA_YA,
				[KEY_8]          = CHAR_JA_YU,
				[KEY_9]          = CHAR_JA_YO,
				[KEY_0]          = CHAR_JA_WO,
				[KEY_MINUS]      = CHAR_JA_HO,
				[KEY_CIRCUMFLEX] = CHAR_JA_HE,
				[KEY_YEN]        = CHAR_JA_CHOUON,
				[KEY_Q]          = CHAR_JA_TA,
				[KEY_W]          = CHAR_JA_TE,
				[KEY_E]          = CHAR_JA_I,
				[KEY_R]          = CHAR_JA_SU,
				[KEY_T]          = CHAR_JA_KA,
				[KEY_Y]          = CHAR_JA_N,
				[KEY_U]          = CHAR_JA_NA,
				[KEY_I]          = CHAR_JA_NI,
				[KEY_O]          = CHAR_JA_RA,
				[KEY_P]          = CHAR_JA_SE,
				[KEY_AT]         = CHAR_JA_DAKUTEN,
				[KEY_LBRACKET]   = CHAR_JA_HANDAKUTEN,
				[KEY_A]          = CHAR_JA_TI,
				[KEY_S]          = CHAR_JA_TO,
				[KEY_D]          = CHAR_JA_SI,
				[KEY_F]          = CHAR_JA_HA,
				[KEY_G]          = CHAR_JA_KI,
				[KEY_H]          = CHAR_JA_KU,
				[KEY_J]          = CHAR_JA_MA,
				[KEY_K]          = CHAR_JA_NO,
				[KEY_L]          = CHAR_JA_RI,
				[KEY_SEMICOLON]  = CHAR_JA_RE,
				[KEY_COLON]      = CHAR_JA_KE,
				[KEY_RBRACKET]   = CHAR_JA_MU,
				[KEY_Z]          = CHAR_JA_TU,
				[KEY_X]          = CHAR_JA_SA,
				[KEY_C]          = CHAR_JA_SO,
				[KEY_V]          = CHAR_JA_HI,
				[KEY_B]          = CHAR_JA_KO,
				[KEY_N]          = CHAR_JA_MI,
				[KEY_M]          = CHAR_JA_MO,
				[KEY_COMMA]      = CHAR_JA_NE,
				[KEY_PERIOD]     = CHAR_JA_RU,
				[KEY_SLASH]      = CHAR_JA_ME,
				[KEY_BACKSLASH]  = CHAR_JA_RO
			}
		},
		[MOD_SHIFT] = {
			.keys = {
				[KEY_1]          = CHAR_JA_NU,
				[KEY_2]          = CHAR_JA_HU,
				[KEY_3]          = CHAR_JA_a,
				[KEY_4]          = CHAR_JA_u,
				[KEY_5]          = CHAR_JA_e,
				[KEY_6]          = CHAR_JA_o,
				[KEY_7]          = CHAR_JA_ya,
				[KEY_8]          = CHAR_JA_yu,
				[KEY_9]          = CHAR_JA_yo,
				[KEY_0]          = CHAR_JA_WO,
				[KEY_MINUS]      = CHAR_JA_YE,
				[KEY_CIRCUMFLEX] = CHAR_JA_WO,
				[KEY_YEN]        = CHAR_JA_CHOUON,
				[KEY_Q]          = CHAR_JA_TA,
				[KEY_W]          = CHAR_JA_TE,
				[KEY_E]          = CHAR_JA_i,
				[KEY_R]          = CHAR_JA_SU,
				[KEY_T]          = CHAR_JA_ka,
				[KEY_Y]          = CHAR_JA_N,
				[KEY_U]          = CHAR_JA_NA,
				[KEY_I]          = CHAR_JA_NI,
				[KEY_O]          = CHAR_JA_RA,
				[KEY_P]          = CHAR_JA_SE,
				[KEY_AT]         = CHAR_JA_DAKUTEN,
				[KEY_LBRACKET]   = CHAR_JA_LQUOTE,
				[KEY_A]          = CHAR_JA_TI,
				[KEY_S]          = CHAR_JA_TO,
				[KEY_D]          = CHAR_JA_SI,
				[KEY_F]          = CHAR_JA_wa,
				[KEY_G]          = CHAR_JA_KI,
				[KEY_H]          = CHAR_JA_KU,
				[KEY_J]          = CHAR_JA_MA,
				[KEY_K]          = CHAR_JA_NO,
				[KEY_L]          = CHAR_JA_RI,
				[KEY_SEMICOLON]  = CHAR_JA_RE,
				[KEY_COLON]      = CHAR_JA_KE,
				[KEY_RBRACKET]   = CHAR_JA_RQUOTE,
				[KEY_Z]          = CHAR_JA_tu,
				[KEY_X]          = CHAR_JA_SA,
				[KEY_C]          = CHAR_JA_SO,
				[KEY_V]          = CHAR_JA_WI,
				[KEY_B]          = CHAR_JA_KO,
				[KEY_N]          = CHAR_JA_MI,
				[KEY_M]          = CHAR_JA_MO,
				[KEY_COMMA]      = CHAR_JA_COMMA,
				[KEY_PERIOD]     = CHAR_JA_PERIOD,
				[KEY_SLASH]      = CHAR_JA_CDOT,
				[KEY_BACKSLASH]  = CHAR_JA_RO
			}
		}
	}
};

static const struct keymap _config_keymap_kr = {
	.layers = {
		[MOD_NONE] = {
			.keys = {
				[KEY_1]          = CHAR_1,
				[KEY_2]          = CHAR_2,
				[KEY_3]          = CHAR_3,
				[KEY_4]          = CHAR_4,
				[KEY_5]          = CHAR_5,
				[KEY_6]          = CHAR_6,
				[KEY_7]          = CHAR_7,
				[KEY_8]          = CHAR_8,
				[KEY_9]          = CHAR_9,
				[KEY_0]          = CHAR_0,
				[KEY_MINUS]      = CHAR_MINUS,
				[KEY_CIRCUMFLEX] = CHAR_CIRCUMFLEX,
				[KEY_YEN]        = CHAR_BACKSLASH,
				[KEY_Q]          = CHAR_KR_B,
				[KEY_W]          = CHAR_KR_J,
				[KEY_E]          = CHAR_KR_D,
				[KEY_R]          = CHAR_KR_G,
				[KEY_T]          = CHAR_KR_S,
				[KEY_Y]          = CHAR_KR_YO,
				[KEY_U]          = CHAR_KR_YEO,
				[KEY_I]          = CHAR_KR_YA,
				[KEY_O]          = CHAR_KR_AE,
				[KEY_P]          = CHAR_KR_E,
				[KEY_AT]         = CHAR_AT,
				[KEY_LBRACKET]   = CHAR_LBRACKET,
				[KEY_A]          = CHAR_KR_M,
				[KEY_S]          = CHAR_KR_N,
				[KEY_D]          = CHAR_KR_NG,
				[KEY_F]          = CHAR_KR_R,
				[KEY_G]          = CHAR_KR_H,
				[KEY_H]          = CHAR_KR_O,
				[KEY_J]          = CHAR_KR_EO,
				[KEY_K]          = CHAR_KR_A,
				[KEY_L]          = CHAR_KR_I,
				[KEY_SEMICOLON]  = CHAR_SEMICOLON,
				[KEY_COLON]      = CHAR_COLON,
				[KEY_RBRACKET]   = CHAR_RBRACKET,
				[KEY_Z]          = CHAR_KR_K,
				[KEY_X]          = CHAR_KR_T,
				[KEY_C]          = CHAR_KR_Z,
				[KEY_V]          = CHAR_KR_P,
				[KEY_B]          = CHAR_KR_YU,
				[KEY_N]          = CHAR_KR_U,
				[KEY_M]          = CHAR_KR_EU,
				[KEY_COMMA]      = CHAR_COMMA,
				[KEY_PERIOD]     = CHAR_PERIOD,
				[KEY_SLASH]      = CHAR_SLASH,
				[KEY_BACKSLASH]  = CHAR_BACKSLASH
			}
		},
		[MOD_SHIFT] = {
			.keys = {
				[KEY_1]          = CHAR_EXCLAM,
				[KEY_2]          = CHAR_DQUOTE,
				[KEY_3]          = CHAR_POUND,
				[KEY_4]          = CHAR_DOLLAR,
				[KEY_5]          = CHAR_PERCENT,
				[KEY_6]          = CHAR_AMPERSAND,
				[KEY_7]          = CHAR_QUOTE,
				[KEY_8]          = CHAR_LPAREN,
				[KEY_9]          = CHAR_RPAREN,
				[KEY_0]          = CHAR_TILDA,
				[KEY_MINUS]      = CHAR_EQ,
				[KEY_CIRCUMFLEX] = CHAR_TILDA,
				[KEY_YEN]        = CHAR_PIPE,
				[KEY_Q]          = CHAR_KR_BB,
				[KEY_W]          = CHAR_KR_JJ,
				[KEY_E]          = CHAR_KR_DD,
				[KEY_R]          = CHAR_KR_GG,
				[KEY_T]          = CHAR_KR_SS,
				[KEY_Y]          = CHAR_KR_YO,
				[KEY_U]          = CHAR_KR_YEO,
				[KEY_I]          = CHAR_KR_YA,
				[KEY_O]          = CHAR_KR_YAE,
				[KEY_P]          = CHAR_KR_YE,
				[KEY_AT]         = CHAR_BACKTICK,
				[KEY_LBRACKET]   = CHAR_LBRACE,
				[KEY_A]          = CHAR_KR_M,
				[KEY_S]          = CHAR_KR_N,
				[KEY_D]          = CHAR_KR_NG,
				[KEY_F]          = CHAR_KR_R,
				[KEY_G]          = CHAR_KR_H,
				[KEY_H]          = CHAR_KR_O,
				[KEY_J]          = CHAR_KR_EO,
				[KEY_K]          = CHAR_KR_A,
				[KEY_L]          = CHAR_KR_I,
				[KEY_SEMICOLON]  = CHAR_PLUS,
				[KEY_COLON]      = CHAR_ASTERISK,
				[KEY_RBRACKET]   = CHAR_RBRACE,
				[KEY_Z]          = CHAR_KR_K,
				[KEY_X]          = CHAR_KR_T,
				[KEY_C]          = CHAR_KR_Z,
				[KEY_V]          = CHAR_KR_P,
				[KEY_B]          = CHAR_KR_YU,
				[KEY_N]          = CHAR_KR_U,
				[KEY_M]          = CHAR_KR_EU,
				[KEY_COMMA]      = CHAR_LT,
				[KEY_PERIOD]     = CHAR_GT,
				[KEY_SLASH]      = CHAR_QMARK,
				[KEY_BACKSLASH]  = CHAR_UNDERSCORE
			}
		}
	}
};

static const struct keymap *_config_keymap[LANG_MAX] = {
	[LANG_EN]  = &_config_keymap_en,
	[LANG_JA]  = &_config_keymap_ja,
	[LANG_KR]  = &_config_keymap_kr
};

int config_keysym_to_char(char_t *dst, const keysym_t *src, const lang_t lang)
{
	const struct keymap *map;
	char_t chr;

	if (!dst || !src || lang < 0 || lang >= LANG_MAX) {
		return -EINVAL;
	}

	/* keymaps have only MOD_SHIFT and MOD_NONE for now */
	if (src->mod & ~MOD_SHIFT) {
		return -ENOENT;
	}

	map = _config_keymap[lang];
	chr = map->layers[src->mod].keys[src->key];

	if (!chr) {
		return -ENOENT;
	}

	*dst = chr;
	return 0;
}
