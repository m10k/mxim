/*
 * char.h - This file is part of mxim
 * Copyright (C) 2024-2025 Matthias Kruk
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

#ifndef MXIM_CHAR_H
#define MXIM_CHAR_H

#include <stddef.h>

typedef enum {
	CHAR_INVALID = 0,

	CHAR_EXCLAM,
	CHAR_DQUOTE,
	CHAR_POUND,
	CHAR_DOLLAR,
	CHAR_PERCENT,
	CHAR_AMPERSAND,
	CHAR_QUOTE,
	CHAR_LPAREN,
	CHAR_RPAREN,
	CHAR_ASTERISK,
	CHAR_PLUS,
	CHAR_COMMA,
	CHAR_MINUS,
	CHAR_PERIOD,
	CHAR_SLASH,
	CHAR_COLON,
	CHAR_SEMICOLON,
	CHAR_LT,
	CHAR_EQ,
	CHAR_GT,
	CHAR_QMARK,
	CHAR_AT,
	CHAR_LBRACKET,
	CHAR_BACKSLASH,
	CHAR_RBRACKET,
	CHAR_CIRCUMFLEX,
	CHAR_UNDERSCORE,
	CHAR_BACKTICK,
	CHAR_LBRACE,
	CHAR_PIPE,
	CHAR_RBRACE,
	CHAR_TILDA,
	CHAR_SPACE,
	CHAR_TAB,

	CHAR_0,
	CHAR_1,
	CHAR_2,
	CHAR_3,
	CHAR_4,
	CHAR_5,
	CHAR_6,
	CHAR_7,
	CHAR_8,
	CHAR_9,

	CHAR_A,
	CHAR_B,
	CHAR_C,
	CHAR_D,
	CHAR_E,
	CHAR_F,
	CHAR_G,
	CHAR_H,
	CHAR_I,
	CHAR_J,
	CHAR_K,
	CHAR_L,
	CHAR_M,
	CHAR_N,
	CHAR_O,
	CHAR_P,
	CHAR_Q,
	CHAR_R,
	CHAR_S,
	CHAR_T,
	CHAR_U,
	CHAR_V,
	CHAR_W,
	CHAR_X,
	CHAR_Y,
	CHAR_Z,
	CHAR_a,
	CHAR_b,
	CHAR_c,
	CHAR_d,
	CHAR_e,
	CHAR_f,
	CHAR_g,
	CHAR_h,
	CHAR_i,
	CHAR_j,
	CHAR_k,
	CHAR_l,
	CHAR_m,
	CHAR_n,
	CHAR_o,
	CHAR_p,
	CHAR_q,
	CHAR_r,
	CHAR_s,
	CHAR_t,
	CHAR_u,
	CHAR_v,
	CHAR_w,
	CHAR_x,
	CHAR_y,
	CHAR_z,

	CHAR_JA_A, /* あ */
	CHAR_JA_I,
	CHAR_JA_U,
	CHAR_JA_E,
	CHAR_JA_O,
	CHAR_JA_a, /* ぁ */
	CHAR_JA_i,
	CHAR_JA_u,
	CHAR_JA_e,
	CHAR_JA_o,
	CHAR_JA_KA,
	CHAR_JA_KI,
	CHAR_JA_KU,
	CHAR_JA_KE,
	CHAR_JA_KO,
	CHAR_JA_ka, /* ヵ */
	CHAR_JA_ke, /* ヶ */
	CHAR_JA_GA,
	CHAR_JA_GI,
	CHAR_JA_GU,
	CHAR_JA_GE,
	CHAR_JA_GO,
	CHAR_JA_TA,
	CHAR_JA_TI,
	CHAR_JA_TU,
	CHAR_JA_TE,
	CHAR_JA_TO,
	CHAR_JA_tu,
	CHAR_JA_DA,
	CHAR_JA_DI,
	CHAR_JA_DU,
	CHAR_JA_DE,
	CHAR_JA_DO,
	CHAR_JA_SA,
	CHAR_JA_SI,
	CHAR_JA_SU,
	CHAR_JA_SE,
	CHAR_JA_SO,
	CHAR_JA_ZA,
	CHAR_JA_ZI,
	CHAR_JA_ZU,
	CHAR_JA_ZE,
	CHAR_JA_ZO,
	CHAR_JA_RA,
	CHAR_JA_RI,
	CHAR_JA_RU,
	CHAR_JA_RE,
	CHAR_JA_RO,
	CHAR_JA_YA,
	CHAR_JA_YU,
	CHAR_JA_YO,
	CHAR_JA_ya, /* ゃ */
	CHAR_JA_yu, /* ゅ */
	CHAR_JA_yo, /* ょ */
	CHAR_JA_HA,
	CHAR_JA_HI,
	CHAR_JA_HU,
	CHAR_JA_HE,
	CHAR_JA_HO,
	CHAR_JA_BA,
	CHAR_JA_BI,
	CHAR_JA_BU,
	CHAR_JA_BE,
	CHAR_JA_BO,
	CHAR_JA_PA,
	CHAR_JA_PI,
	CHAR_JA_PU,
	CHAR_JA_PE,
	CHAR_JA_PO,
	CHAR_JA_NA,
	CHAR_JA_NI,
	CHAR_JA_NU,
	CHAR_JA_NE,
	CHAR_JA_NO,
	CHAR_JA_MA,
	CHAR_JA_MI,
	CHAR_JA_MU,
	CHAR_JA_ME,
	CHAR_JA_MO,
	CHAR_JA_WA,
	CHAR_JA_WI, /* ゐ/ヰ */
	CHAR_JA_WE, /* ゑ/ヱ */
	CHAR_JA_WO,
	CHAR_JA_wa, /* ゎ */
	CHAR_JA_N,
	CHAR_JA_VA,
	CHAR_JA_VI,
	CHAR_JA_VU,
	CHAR_JA_VE,
	CHAR_JA_VO,

        CHAR_JA_CHOUON, /* ー（長音棒）*/
        CHAR_JA_DAKUTEN,
        CHAR_JA_HANDAKUTEN,
        CHAR_JA_LQUOTE,
        CHAR_JA_RQUOTE,
        CHAR_JA_CDOT,
        CHAR_JA_PERIOD,
        CHAR_JA_COMMA,

	CHAR_KR_BB,
	CHAR_KR_B,
	CHAR_KR_JJ,
	CHAR_KR_J,
	CHAR_KR_DD,
	CHAR_KR_D,
	CHAR_KR_GG,
	CHAR_KR_G,
	CHAR_KR_SS,
	CHAR_KR_S,
	CHAR_KR_M,
	CHAR_KR_N,
	CHAR_KR_NG,
	CHAR_KR_R,
	CHAR_KR_H,
	CHAR_KR_K,
	CHAR_KR_T,
	CHAR_KR_Z,
	CHAR_KR_P,
	CHAR_KR_YO,
	CHAR_KR_YEO,
	CHAR_KR_YA,
	CHAR_KR_YAE,
	CHAR_KR_AE,
	CHAR_KR_E,
	CHAR_KR_YE,
	CHAR_KR_O,
	CHAR_KR_EO,
	CHAR_KR_A,
	CHAR_KR_I,
	CHAR_KR_YU,
	CHAR_KR_U,
	CHAR_KR_EU,

	CHAR_LAST
} __attribute__((packed)) char_t;

typedef enum {
	LANG_EN,
	LANG_JA,
	LANG_KR,
	LANG_MAX
} lang_t;

int char_to_utf8(const char_t *src, const size_t src_len, char *dst, const size_t dst_size);
int char_to_utf8_dyn(const char_t *src, const size_t src_len, char **dst);
int char_from_utf8(const char *src, const size_t src_len, char_t **dst);
char_t char_combine(const char_t left, const char_t right);
int char_same_set(const char_t left, const char_t right);

#endif /* MXIM_CHAR_H */
