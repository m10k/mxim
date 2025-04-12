/*
 * conjugation.c - This file is part of mxim
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
#include "conjugation.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>

int conjugation_new(conjugation_t **dst,
                    const char_t *dict_form,
                    const int suffix_len,
                    const char_t *conjugation,
                    const int conjugation_len)
{
	conjugation_t *con;
	int err;

	if (!dst) {
		return -EINVAL;
	}

	if (!(con = calloc(1, sizeof(*con)))) {
		return -ENOMEM;
	}

	if ((err = char_dup(&con->dict_form, dict_form, 0)) < 0) {
		goto cleanup;
	}

	if (conjugation && (err = char_dup(&con->conjugation, conjugation, conjugation_len)) < 0) {
		goto cleanup;
	}

	con->suffix_len = suffix_len;
	con->conjugation_len = conjugation_len;
	*dst = con;

cleanup:
	if (err < 0) {
		conjugation_free(&con);
	} else {
		*dst = con;
		err = 0;
	}

	return err;
}

int conjugation_free(conjugation_t **conj)
{
	if (!conj) {
		return -EINVAL;
	}

	if (*conj) {
		free((*conj)->dict_form);
		free((*conj)->conjugation);
		free(*conj);
		*conj = NULL;
	}

	return 0;
}

int conjugation_conjugate(char **dst, const char *src, conjugation_t *conjugation)
{
	size_t src_bytes;
	size_t drop_bytes;
	size_t conjugation_bytes;
	size_t dst_bytes;
	char *dst_str;

	/* FIXME: Add support for non-CJK languages */

        src_bytes = strlen(src);
        /* CJK characters are 3 bytes in UTF-8 */
        drop_bytes = conjugation->suffix_len * 3;
	conjugation_bytes = conjugation->conjugation_len * 3;

	if (drop_bytes > src_bytes) {
		return -EOVERFLOW;
	}

	src_bytes -= drop_bytes;

	if ((SIZE_MAX - conjugation_bytes) < src_bytes) {
		return -EOVERFLOW;
	}

	dst_bytes = src_bytes + conjugation_bytes;

	if (dst_bytes == SIZE_MAX) {
		return -EOVERFLOW;
	}

	if (!(dst_str = malloc(dst_bytes + 1))) {
		return -ENOMEM;
	}

	memcpy(dst_str, src, src_bytes);
	char_to_utf8(conjugation->conjugation, conjugation->conjugation_len,
	             dst_str + src_bytes, dst_bytes - src_bytes + 1);

	dst_str[dst_bytes] = 0;
	*dst = dst_str;
	return 0;
}
