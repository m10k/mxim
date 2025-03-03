/*
 * hangul.c - This file is part of mxim
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

#include <stdint.h>
#include <errno.h>
#include "char.h"

/*
 * The codepoint for a Hangul can be determined with the following formula.
 *
 *   codepoint = 0xAC00 + 588 * x + 28 * y + z
 *
 * Where x, y, and z have the following meaning.
 *
 *  x      | 0  | 1  | 2  | 3  | 4  | 5  | 6  | 7  | 8  | 9  | 10 | 11 | 12
 *  Hangul | ㄱ | ㄲ | ㄴ | ㄷ | ㄸ | ㄹ | ㅁ | ㅂ | ㅃ | ㅅ | ㅆ | ㅇ | ㅈ
 *  x      | 13 | 14 | 15 | 16 | 17 | 18
 *  Hangul | ㅉ | ㅊ | ㅋ | ㅌ | ㅍ | ㅎ
 *
 *  y      | 0  | 1  | 2  | 3  | 4  | 5  | 6  | 7  | 8  | 9  | 10 | 11 | 12
 *  Hangul | ㅏ | ㅐ | ㅑ | ㅒ | ㅓ | ㅔ | ㅕ | ㅖ | ㅗ | ㅘ | ㅙ | ㅚ | ㅛ
 *  y      | 13 | 14 | 15 | 16 | 17 | 18 | 19 | 20
 *  Hangul | ㅜ | ㅝ | ㅞ | ㅟ | ㅠ | ㅡ | ㅢ | ㅣ
 *
 *  z      | 0  | 1  | 2  | 3  | 4  | 5  | 6  | 7  | 8  | 9  | 10 | 11 | 12
 *  Hangul |    | ㄱ | ㄲ | ㄳ | ㄴ | ㄵ | ㄶ | ㄷ | ㄹ | ㄺ | ㄻ | ㄼ | ㄽ
 *  z      | 13 | 14 | 15 | 16 | 17 | 18 | 19 | 20 | 21 | 22 | 23 | 24 | 25
 *  Hangul | ㄾ | ㄿ | ㅀ | ㅁ | ㅂ | ㅄ | ㅅ | ㅆ | ㅇ | ㅈ | ㅊ | ㅋ | ㅌ
 *  z      | 26 | 27
 *  Hangul | ㅍ | ㅎ
 *
 * The conversion methods read a string of char_t elements and try to convert
 * them according to the tables above. When converting the value for z (the
 * patchim), the conversion first checks if the character is the start of a
 * new hangul, as an input such as "ㄱㅏㄷ" could be "갇" or "가ㄷ...". This
 * cannot be decided without looking at the next char_t.
 */

enum {
	MODE_X,
	MODE_Y,
	MODE_Y_O,
	MODE_Y_U,
	MODE_Y_EU,
	MODE_Z,
	MODE_Z_G,
	MODE_Z_N,
	MODE_Z_R,
	MODE_Z_B,
	MODE_DONE
};

#define FIRST_HANGUL_CODEPOINT 0xAC00
#define LAST_HANGUL_CODEPOINT  0xD7A3

#define ASIZE(a)     (sizeof(a) / sizeof(a[0]))
#define IS_HANGUL(c) ((c) >= CHAR_KR_BB && (c) <= CHAR_KR_EU)

static const int xmap[] = {
	[CHAR_KR_G]  = 1,
	[CHAR_KR_GG] = 2,
	[CHAR_KR_N]  = 3,
	[CHAR_KR_D]  = 4,
	[CHAR_KR_DD] = 5,
	[CHAR_KR_R]  = 6,
	[CHAR_KR_M]  = 7,
	[CHAR_KR_B]  = 8,
	[CHAR_KR_BB] = 9,
	[CHAR_KR_S]  = 10,
	[CHAR_KR_SS] = 11,
	[CHAR_KR_NG] = 12,
	[CHAR_KR_J]  = 13,
	[CHAR_KR_JJ] = 14,
	[CHAR_KR_Z]  = 15,
	[CHAR_KR_K]  = 16,
	[CHAR_KR_T]  = 17,
	[CHAR_KR_P]  = 18,
	[CHAR_KR_H]  = 19
};

static const int ymap[] = {
	[CHAR_KR_A]   = 1,
	[CHAR_KR_AE]  = 2,
	[CHAR_KR_YA]  = 3,
	[CHAR_KR_YAE] = 4,
	[CHAR_KR_EO]  = 5,
	[CHAR_KR_E]   = 6,
	[CHAR_KR_YEO] = 7,
	[CHAR_KR_YE]  = 8,
	[CHAR_KR_O]   = 9,
	[CHAR_KR_YO]  = 13,
	[CHAR_KR_U]   = 14,
	[CHAR_KR_YU]  = 18,
	[CHAR_KR_EU]  = 19,
	[CHAR_KR_I]   = 21
};

static const int ymap_o[] = {
	[CHAR_KR_A]  = 1,
	[CHAR_KR_AE] = 2,
	[CHAR_KR_I]  = 3
};

static const int ymap_u[] = {
	[CHAR_KR_EO] = 1,
	[CHAR_KR_E]  = 2,
	[CHAR_KR_I]  = 3

};
static const int ymap_eu[] = {
	[CHAR_KR_I] = 1
};

static const int zmap[] = {
	[CHAR_KR_G]  = 1,
	[CHAR_KR_GG] = 2,
	[CHAR_KR_N]  = 4,
	[CHAR_KR_D]  = 7,
	[CHAR_KR_R]  = 8,
	[CHAR_KR_M]  = 16,
	[CHAR_KR_B]  = 17,
	[CHAR_KR_S]  = 19,
	[CHAR_KR_SS] = 20,
	[CHAR_KR_NG] = 21,
	[CHAR_KR_J]  = 22,
	[CHAR_KR_Z]  = 23,
	[CHAR_KR_K]  = 24,
	[CHAR_KR_T]  = 25,
	[CHAR_KR_P]  = 26,
	[CHAR_KR_H]  = 27
};

static const int zmap_g[] = {
	[CHAR_KR_S] = 2
};

static const int zmap_n[] = {
	[CHAR_KR_J] = 1,
	[CHAR_KR_H] = 2
};

static const int zmap_r[] = {
	[CHAR_KR_G] = 1,
	[CHAR_KR_M] = 2,
	[CHAR_KR_B] = 3,
	[CHAR_KR_S] = 4,
	[CHAR_KR_T] = 5,
	[CHAR_KR_P] = 6,
	[CHAR_KR_H] = 7
};

static const int zmap_b[] = {
	[CHAR_KR_S] = 1
};

static int codepoint_to_utf8(uint32_t codepoint, char *dst, const size_t dst_size)
{
	if (!dst) {
		return -EINVAL;
	}

	if (codepoint < FIRST_HANGUL_CODEPOINT || codepoint > LAST_HANGUL_CODEPOINT) {
		return -EDOM;
	}

	if (dst_size < 3) {
		return -EMSGSIZE;
	}

	/*
	 * Codepoints between U+0800 and U+FFFF are converted to UTF-8 like this:
	 *   U+wxyz -> 1110wwww 10xxxxyy 10yyzzzz
	 */

	dst[0] = 0xE0 | ((codepoint >> 12) & 0x0f);
	dst[1] = 0x80 | ((codepoint >>  6) & 0x3f);
	dst[2] = 0x80 | (codepoint         & 0x3f);

	return 3;
}

int hangul_to_utf8(const char_t *src, const size_t src_len,
                   char *dst, const size_t dst_size)
{
	int i;
	int x;
	int y;
	int z;
	int mode;

	x = y = -1;
	z = 0;

	for (mode = MODE_X, i = 0; i < src_len && IS_HANGUL(src[i]) && mode != MODE_DONE; i++) {
		switch (mode) {
		case MODE_X:
			if (src[i] >= ASIZE(xmap) ||
			    (x = xmap[src[i]] - 1) < 0) {
				mode = MODE_DONE;
				break;
			}

			mode = MODE_Y;
			break;

		case MODE_Y:
			if (src[i] >= ASIZE(ymap) ||
			    (y = ymap[src[i]] - 1) < 0) {
				mode = MODE_DONE;
				break;
			}

			switch (src[i]) {
			case CHAR_KR_O:
				mode = MODE_Y_O;
				break;

			case CHAR_KR_U:
				mode = MODE_Y_U;
				break;

			case CHAR_KR_EU:
				mode = MODE_Y_EU;
				break;

			default:
				mode = MODE_Z;
				break;
			}

			break;

		case MODE_Y_O:
			if (src[i] >= ASIZE(ymap_o) || !ymap_o[src[i]]) {
				/* use same character again for z */
				i--;
			} else {
				y += ymap_o[src[i]];
			}
			mode = MODE_Z;
			break;

		case MODE_Y_U:
			if (src[i] >= ASIZE(ymap_u) || !ymap_u[src[i]]) {
				/* use same character again for z */
				i--;
			} else {
				y += ymap_u[src[i]];
			}
			mode = MODE_Z;
			break;

		case MODE_Y_EU:
			if (src[i] >= ASIZE(ymap_eu) || !ymap_eu[src[i]]) {
				/* use same character again for z */
				i--;
			} else {
				y += ymap_eu[src[i]];
			}
			mode = MODE_Z;
			break;

		case MODE_Z:
			if (src[i] >= ASIZE(zmap) || !zmap[src[i]]) {
				/* character was not consumed */
				i--;
				mode = MODE_DONE;
				break;
			}

			if (hangul_to_utf8(src + i, src_len - i, NULL, 0) > 0) {
				/* Byte is not a patchim but start of a new hangul */
				i--;
				mode = MODE_DONE;
			}

			z = zmap[src[i]];

			switch (src[i]) {
			case CHAR_KR_G:
				mode = MODE_Z_G;
				break;

			case CHAR_KR_N:
				mode = MODE_Z_N;
				break;

			case CHAR_KR_R:
				mode = MODE_Z_R;
				break;

			case CHAR_KR_B:
				mode = MODE_Z_B;
				break;

			default:
				mode = MODE_DONE;
				break;
			}
			break;

		case MODE_Z_G:
			if (src[i] >= ASIZE(zmap_g) || !zmap_g[src[i]]) {
				i--;
			} else {
				z += zmap_g[src[i]];
			}
			mode = MODE_DONE;
			break;

		case MODE_Z_N:
			if (src[i] >= ASIZE(zmap_n) || !zmap_n[src[i]]) {
				i--;
			} else {
				z += zmap_n[src[i]];
			}
			mode = MODE_DONE;
			break;

		case MODE_Z_R:
			if (src[i] >= ASIZE(zmap_r) || !zmap_r[src[i]]) {
				i--;
			} else {
				z += zmap_r[src[i]];
			}
			mode = MODE_DONE;
			break;

		case MODE_Z_B:
			if (src[i] >= ASIZE(zmap_b) || !zmap_b[src[i]]) {
				i--;
			} else {
				z += zmap_b[src[i]];
			}
			mode = MODE_DONE;
			break;

		case MODE_DONE:
			break;
		}
	}

	if (x >= 0 && y >= 0) {
		uint32_t codepoint;

		codepoint = 0xAC00 + 588 * x + 28 * y + z;
		/*
		 * Write codepoint only if `dst' was specified. This way, this
		 * function can be used to check if we have a valid Hangul.
		 */
		if (dst && codepoint_to_utf8(codepoint, dst, dst_size) < 0) {
			i = -EMSGSIZE;
		}
	} else {
		i = -ERANGE;
	}

	return i;
}
