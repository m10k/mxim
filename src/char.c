/*
 * char.c - This file is part of mxim
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

#include "char.h"
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

static const char *_charmap[] = {
	[CHAR_INVALID]    = "\0",

	[CHAR_EXCLAM]     = "!",
	[CHAR_DQUOTE]     = "\"",
	[CHAR_POUND]      = "#",
	[CHAR_DOLLAR]     = "$",
	[CHAR_PERCENT]    = "%",
	[CHAR_AMPERSAND]  = "&",
	[CHAR_QUOTE]      = "'",
	[CHAR_LPAREN]     = "(",
	[CHAR_RPAREN]     = ")",
	[CHAR_ASTERISK]   = "*",
	[CHAR_PLUS]       = "+",
	[CHAR_COMMA]      = ",",
	[CHAR_MINUS]      = "-",
	[CHAR_PERIOD]     = ".",
	[CHAR_SLASH]      = "/",
	[CHAR_COLON]      = ":",
	[CHAR_SEMICOLON]  = ";",
	[CHAR_LT]         = "<",
	[CHAR_EQ]         = "=",
	[CHAR_GT]         = ">",
	[CHAR_QMARK]      = "?",
	[CHAR_AT]         = "@",
	[CHAR_LBRACKET]   = "[",
	[CHAR_BACKSLASH]  = "\\",
	[CHAR_RBRACKET]   = "]",
	[CHAR_CIRCUMFLEX] = "^",
	[CHAR_UNDERSCORE] = "_",
	[CHAR_BACKTICK]   = "`",
	[CHAR_LBRACE]     = "{",
	[CHAR_PIPE]       = "|",
	[CHAR_RBRACE]     = "}",
	[CHAR_TILDA]      = "~",
	[CHAR_SPACE]      = " ",
	[CHAR_TAB]        = "\t",

	[CHAR_0] = "0",
	[CHAR_1] = "1",
	[CHAR_2] = "2",
	[CHAR_3] = "3",
	[CHAR_4] = "4",
	[CHAR_5] = "5",
	[CHAR_6] = "6",
	[CHAR_7] = "7",
	[CHAR_8] = "8",
	[CHAR_9] = "9",

	[CHAR_A] = "A",
	[CHAR_B] = "B",
	[CHAR_C] = "C",
	[CHAR_D] = "D",
	[CHAR_E] = "E",
	[CHAR_F] = "F",
	[CHAR_G] = "G",
	[CHAR_H] = "H",
	[CHAR_I] = "I",
	[CHAR_J] = "J",
	[CHAR_K] = "K",
	[CHAR_L] = "L",
	[CHAR_M] = "M",
	[CHAR_N] = "N",
	[CHAR_O] = "O",
	[CHAR_P] = "P",
	[CHAR_Q] = "Q",
	[CHAR_R] = "R",
	[CHAR_S] = "S",
	[CHAR_T] = "T",
	[CHAR_U] = "U",
	[CHAR_V] = "V",
	[CHAR_W] = "W",
	[CHAR_X] = "X",
	[CHAR_Y] = "Y",
	[CHAR_Z] = "Z",
	[CHAR_a] = "a",
	[CHAR_b] = "b",
	[CHAR_c] = "c",
	[CHAR_d] = "d",
	[CHAR_e] = "e",
	[CHAR_f] = "f",
	[CHAR_g] = "g",
	[CHAR_h] = "h",
	[CHAR_i] = "i",
	[CHAR_j] = "j",
	[CHAR_k] = "k",
	[CHAR_l] = "l",
	[CHAR_m] = "m",
	[CHAR_n] = "n",
	[CHAR_o] = "o",
	[CHAR_p] = "p",
	[CHAR_q] = "q",
	[CHAR_r] = "r",
	[CHAR_s] = "s",
	[CHAR_t] = "t",
	[CHAR_u] = "u",
	[CHAR_v] = "v",
	[CHAR_w] = "w",
	[CHAR_x] = "x",
	[CHAR_y] = "y",
	[CHAR_z] = "z",

	[CHAR_JA_A]      = "あ", /* あ */
	[CHAR_JA_I]      = "い",
	[CHAR_JA_U]      = "う",
	[CHAR_JA_E]      = "え",
	[CHAR_JA_O]      = "お",
	[CHAR_JA_a]      = "ぁ", /* ぁ */
	[CHAR_JA_i]      = "ぃ",
	[CHAR_JA_u]      = "ぅ",
	[CHAR_JA_e]      = "ぇ",
	[CHAR_JA_o]      = "ぉ",
	[CHAR_JA_KA]     = "か",
	[CHAR_JA_KI]     = "き",
	[CHAR_JA_KU]     = "く",
	[CHAR_JA_KE]     = "け",
	[CHAR_JA_KO]     = "こ",
	[CHAR_JA_ka]     = "ヵ", /* ヵ */
	[CHAR_JA_ke]     = "ヶ", /* ヶ */
	[CHAR_JA_GA]     = "が",
	[CHAR_JA_GI]     = "ぎ",
	[CHAR_JA_GU]     = "ぐ",
	[CHAR_JA_GE]     = "げ",
	[CHAR_JA_GO]     = "ご",
	[CHAR_JA_TA]     = "た",
	[CHAR_JA_TI]     = "ち",
	[CHAR_JA_TU]     = "つ",
	[CHAR_JA_TE]     = "て",
	[CHAR_JA_TO]     = "と",
	[CHAR_JA_tu]     = "っ",
	[CHAR_JA_DA]     = "だ",
	[CHAR_JA_DI]     = "ぢ",
	[CHAR_JA_DU]     = "づ",
	[CHAR_JA_DE]     = "で",
	[CHAR_JA_DO]     = "ど",
	[CHAR_JA_SA]     = "さ",
	[CHAR_JA_SI]     = "し",
	[CHAR_JA_SU]     = "す",
	[CHAR_JA_SE]     = "せ",
	[CHAR_JA_SO]     = "そ",
	[CHAR_JA_ZA]     = "ざ",
	[CHAR_JA_ZI]     = "じ",
	[CHAR_JA_ZU]     = "ず",
	[CHAR_JA_ZE]     = "ぜ",
	[CHAR_JA_ZO]     = "ぞ",
	[CHAR_JA_RA]     = "ら",
	[CHAR_JA_RI]     = "り",
	[CHAR_JA_RU]     = "る",
	[CHAR_JA_RE]     = "れ",
	[CHAR_JA_RO]     = "ろ",
	[CHAR_JA_YA]     = "や",
	[CHAR_JA_YU]     = "ゆ",
	[CHAR_JA_YE]     = "ゑ", /* ゑ/ヱ */
	[CHAR_JA_YO]     = "よ",
	[CHAR_JA_ya]     = "ゃ", /* ゃ */
	[CHAR_JA_yu]     = "ゅ", /* ゅ */
	[CHAR_JA_yo]     = "ょ", /* ょ */
	[CHAR_JA_HA]     = "は",
	[CHAR_JA_HI]     = "ひ",
	[CHAR_JA_HU]     = "ふ",
	[CHAR_JA_HE]     = "へ",
	[CHAR_JA_HO]     = "ほ",
	[CHAR_JA_BA]     = "ば",
	[CHAR_JA_BI]     = "び",
	[CHAR_JA_BU]     = "ぶ",
	[CHAR_JA_BE]     = "べ",
	[CHAR_JA_BO]     = "ぼ",
	[CHAR_JA_PA]     = "ぱ",
	[CHAR_JA_PI]     = "ぴ",
	[CHAR_JA_PU]     = "ぷ",
	[CHAR_JA_PE]     = "ぺ",
	[CHAR_JA_PO]     = "ぽ",
	[CHAR_JA_NA]     = "な",
	[CHAR_JA_NI]     = "に",
	[CHAR_JA_NU]     = "ぬ",
	[CHAR_JA_NE]     = "ね",
	[CHAR_JA_NO]     = "の",
	[CHAR_JA_MA]     = "ま",
	[CHAR_JA_MI]     = "み",
	[CHAR_JA_MU]     = "む",
	[CHAR_JA_ME]     = "め",
	[CHAR_JA_MO]     = "も",
	[CHAR_JA_WA]     = "わ",
	[CHAR_JA_WI]     = "ゐ", /* ゐ/ヰ */
	[CHAR_JA_WO]     = "を",
	[CHAR_JA_wa]     = "ゎ", /* ゎ */
	[CHAR_JA_N]      = "ん",

        [CHAR_JA_CHOUON]     = "ー",
        [CHAR_JA_DAKUTEN]    = "゛",
        [CHAR_JA_HANDAKUTEN] = "゜",
        [CHAR_JA_LQUOTE]     = "「",
        [CHAR_JA_RQUOTE]     = "」",
        [CHAR_JA_CDOT]       = "・",
        [CHAR_JA_PERIOD]     = "。",
        [CHAR_JA_COMMA]      = "、",

	[CHAR_KR_BB]  = "ㅃ",
	[CHAR_KR_B]   = "ㅂ",
	[CHAR_KR_JJ]  = "ㅉ",
	[CHAR_KR_J]   = "ㅈ",
	[CHAR_KR_DD]  = "ㄸ",
	[CHAR_KR_D]   = "ㄷ",
	[CHAR_KR_GG]  = "ㄲ",
	[CHAR_KR_G]   = "ㄱ",
	[CHAR_KR_SS]  = "ㅆ",
	[CHAR_KR_S]   = "ㅅ",
	[CHAR_KR_M]   = "ㅁ",
	[CHAR_KR_N]   = "ㄴ",
	[CHAR_KR_NG]  = "ㅇ",
	[CHAR_KR_R]   = "ㄹ",
	[CHAR_KR_H]   = "ㅎ",
	[CHAR_KR_K]   = "ㅋ",
	[CHAR_KR_T]   = "ㅌ",
	[CHAR_KR_Z]   = "ㅊ",
	[CHAR_KR_P]   = "ㅍ",
	[CHAR_KR_YO]  = "ㅛ",
	[CHAR_KR_YEO] = "ㅕ",
	[CHAR_KR_YA]  = "ㅑ",
	[CHAR_KR_YAE] = "ㅒ",
	[CHAR_KR_AE]  = "ㅐ",
	[CHAR_KR_E]   = "ㅔ",
	[CHAR_KR_YE]  = "ㅖ",
	[CHAR_KR_O]   = "ㅗ",
	[CHAR_KR_EO]  = "ㅓ",
	[CHAR_KR_A]   = "ㅏ",
	[CHAR_KR_I]   = "ㅣ",
	[CHAR_KR_YU]  = "ㅠ",
	[CHAR_KR_U]   = "ㅜ",
	[CHAR_KR_EU]  = "ㅡ",

	[CHAR_LAST] = ""
};

int char_to_utf8(const char_t *src, const size_t src_len, char *dst, const size_t dst_size)
{
	size_t src_idx;
	size_t dst_offset;

	if(dst_size > 0) {
		dst[0] = 0;
	}

	for (src_idx = dst_offset = 0; src_idx < src_len; src_idx++) {
		const char *utf8;

		if (!(utf8 = _charmap[src[src_idx]])) {
			break;
		}

		dst_offset += snprintf(dst + dst_offset, dst_size - dst_offset, utf8);
	}

	return (int)dst_offset;
}

int _estimate_size(const char_t *src, const size_t src_len)
{
	int size;
	int i;

	if (!src) {
		return -EINVAL;
	}

	for (size = i = 0; i < (int)src_len; i++) {
		const char *utf8;

		if (!(utf8 = _charmap[src[i]])) {
			break;
		}

		size += snprintf(NULL, 0, utf8);
	}

	return size;
}

int char_to_utf8_dyn(const char_t *src, const size_t src_len, char **dst)
{
	char *buffer;
	int buffer_size;

	if (!dst || (buffer_size = _estimate_size(src, src_len)) < 0) {
		return -EINVAL;
	}

	if (!(buffer = malloc(buffer_size + 1))) {
		return -ENOMEM;
	}

	*dst = buffer;
	return char_to_utf8(src, src_len, buffer, buffer_size + 1);
}
