/*
 * char.c - This file is part of mxim
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

#define _POSIX_C_SOURCE 200809L
#include "char.h"
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Conversion from UTF-8 to char_t is implemented
 * with a lookup tree where the node at the n-th
 * level contains the n-th byte of the UTF-8 sequence.
 *
 * The `data` field in a node refers to either an
 * array of char_t (the conversion result) or another
 * node. In the former case, the corresponding `len`
 * is the length of the char_t array (without the
 * terminating 0-byte); in the case that the data
 * field points to another node, the length is 0.
 * This reduces the size of the tree from ~400K to
 * ~18K.
 */
struct lut_node {
	unsigned char len[256];
	void *data[256];
};

struct lut_node *_lut_root = NULL;

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
	[CHAR_JA_WE]     = "ゑ", /* ゑ/ヱ */
	[CHAR_JA_WO]     = "を",
	[CHAR_JA_wa]     = "ゎ", /* ゎ */
	[CHAR_JA_N]      = "ん",
	[CHAR_JA_VA]     = "ヷ",
	[CHAR_JA_VI]     = "ヸ",
	[CHAR_JA_VU]     = "ヴ",
	[CHAR_JA_VE]     = "ヹ",
	[CHAR_JA_VO]     = "ヺ",

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

static char_t* _chardup(char_t chr)
{
	char_t *dup;

	if ((dup = malloc(2 * sizeof(*dup)))) {
		dup[0] = chr;
		dup[1] = CHAR_INVALID;
	}

	return dup;
}

static int _lut_insert_node(struct lut_node *node, int idx)
{
	struct lut_node *dir;

	if (!(dir = calloc(1, sizeof(*dir)))) {
		return -ENOMEM;
	}

	dir->data[0] = node->data[idx];
	dir->len[0] = node->len[idx];
	node->data[idx] = dir;
	node->len[idx] = 0;

	return 0;
}

static int _lut_insert(struct lut_node *node, const char *key, const char_t *value, const size_t value_len)
{
	unsigned char idx;

	if (!node || !key || !value) {
		return -EINVAL;
	}

	if (value_len > UCHAR_MAX) {
		return -EMSGSIZE;
	}

	idx = (unsigned char)*key;

	if (idx == 0 || (*(key + 1) == 0 && !node->data[idx])) {
		/* insert here */
		node->data[idx] = strdup((char*)value);
		node->len[idx] = (unsigned char)value_len;

		return node->data[idx] ? 0 : -ENOMEM;
	}

	/* make sure we have a node at data[idx] */
	if ((!node->data[idx] || node->len[idx] > 0) &&
	    _lut_insert_node(node, idx) < 0) {
		return -ENOMEM;
	}

	return _lut_insert((struct lut_node*)node->data[idx],
	                   key + 1, value, value_len);
}

static int _lut_find(struct lut_node *node, const char *key, const char_t **value)
{
	unsigned int idx;
	int len;

	if (!node || !key || !value) {
		return -EINVAL;
	}

	idx = (unsigned char)*key;

	if (idx == 0 || node->len[idx] > 0) {
		*value = (const char_t*)node->data[idx];
		len = idx == 0 ? 0 : 1;
	} else {
		if ((len = _lut_find((struct lut_node*)node->data[idx], key + 1, value)) >= 0) {
			len++;
		}
	}

	return len;
}

static int _lut_free(struct lut_node **node)
{
	int i;

	if (!node || !*node) {
		return -EINVAL;
	}

	for (i = 0; i < (sizeof((*node)->data) / sizeof((*node)->data[0])); i++) {
		if ((*node)->data[i]) {
			if ((*node)->len[i] == 0) {
				_lut_free((struct lut_node**)&(*node)->data[i]);
			} else {
				free((*node)->data[i]);
				(*node)->data[i] = NULL;
			}
		}
	}

	return 0;
}

static int _lut_init(void)
{
	int err;
	int i;

	if (_lut_root) {
		/* already initialized */
		return 0;
	}

	if (!(_lut_root = calloc(1, sizeof(*_lut_root)))) {
		return -ENOMEM;
	}

	err = 0;

	for (i = 0; i < CHAR_LAST; i++) {
		char_t *value;
		size_t value_len;

		if (!_charmap[i]) {
			continue;
		}

		value = _chardup((char_t)i);
		value_len = strlen((char*)value) + 1;

		if ((err = _lut_insert(_lut_root, _charmap[i], value, value_len)) < 0) {
			break;
		}
	}

	if (err) {
		_lut_free(&_lut_root);
	}

	return err;
}

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

static int _estimate_size(const char_t *src, const size_t src_len)
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

int char_from_utf8(const char *src, const size_t src_len, char_t **dst)
{
	char_t *result;
	size_t result_size;
	size_t src_offset;
	size_t dst_offset;
	int err;

	if (!src || !dst) {
		return -EINVAL;
	}

	if (src_len == SIZE_MAX) {
		return -EOVERFLOW;
	}

	if (!_lut_root) {
		if ((err = _lut_init()) < 0) {
			return err;
		}
	}

	err = 0;

	/*
	 * Since each Japanese and Korean character uses at least three bytes in UTF-8,
	 * the input length is a safe upper boundary for the output length.
	 */
	result_size = src_len + 1;
	if (!(result = malloc(result_size))) {
		return -ENOMEM;
	}

	for (dst_offset = src_offset = 0; src_offset < src_len; ) {
		const char_t *char_data;
		int char_len;
		int j;

		char_data = NULL;
		char_len = _lut_find(_lut_root, src + src_offset, &char_data);

		if (char_len < 0 || !char_data) {
			err = -EBADMSG;
			break;
		}

		for (j = 0; char_data[j] != CHAR_INVALID; j++) {
			result[dst_offset++] = char_data[j];
		}

		src_offset += char_len;
	}

	if (err) {
		free(result);
	} else {
		result[dst_offset] = CHAR_INVALID;
		*dst = result;
		err = (int)dst_offset;
	}

	return err;
}
