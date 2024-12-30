/*
 * string.c - This file is part of mxim
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

#define _GNU_SOURCE
#include "char.h"
#include "string.h"
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>

struct string {
	char *str;
	size_t len;
	size_t size;
};

int string_new(string_t **str)
{
	string_t *s;

	if (!str) {
		return -EINVAL;
	}

	if (!(s = calloc(1, sizeof(*s)))) {
		return -ENOMEM;
	}

	*str = s;
	return 0;
}

int string_free(string_t **str)
{
	if (!str) {
		return -EINVAL;
	}

	if (*str) {
		free((*str)->str);
		(*str)->str = NULL;
		free(*str);
		*str = NULL;
	}

	return 0;
}

static int _string_grow(string_t *str, const size_t size_request)
{
	char *new_str;
	size_t new_size;

	if (!str) {
		return -EINVAL;
	}

	if (size_request == SIZE_MAX ||
	    SIZE_MAX - str->size < (size_request + 1)) {
		return -EMSGSIZE;
	}

	new_size = str->size + size_request;
	new_str = realloc(str->str, new_size + 1);

	if (!new_str) {
		return -ENOMEM;
	}

	str->str = new_str;
	str->size = new_size;

	return 0;
}

int string_append(string_t *dst, const string_t *src)
{
	size_t available;
	int err;

	if (!dst || !src) {
		return -EINVAL;
	}

	if (src->len == SIZE_MAX) {
		return -EMSGSIZE;
	}

	available = dst->size - dst->len;

	if (available < src->len + 1) {
		if ((err = _string_grow(dst, src->len - available + 1)) < 0) {
			return err;
		}
	}

	if ((err = snprintf(dst->str + dst->len, dst->size - dst->len, "%s", src->str)) > 0) {
		dst->len += err;
	}

	return err;
}

int string_append_char(string_t *dst, const char_t *src, const size_t src_len)
{
	char *utf8;
	size_t utf8_len;
	int err;

	if (!dst || !src) {
		return -EINVAL;
	}

	if ((err = char_to_utf8_dyn(src, src_len, &utf8)) < 0) {
		return err;
	}

	utf8_len = strlen(utf8);
	err = string_append_utf8(dst, utf8, utf8_len);

	free(utf8);
	return err;
}

int string_append_utf8(string_t *dst, const char *src, const size_t src_len)
{
	size_t available;
	int err;
	size_t len;

	if (!dst || !src) {
		return -EINVAL;
	}

	if (src_len == SIZE_MAX) {
		return -EOVERFLOW;
	}

	available = dst->size - dst->len;

	if (available < src_len + 1) {
		err = _string_grow(dst, src_len - available + 1);
		if (err < 0) {
			return err;
		}
	}

	len = dst->size - dst->len;
	if (src_len < len) {
		len = src_len;
	}

	if (len > INT_MAX) {
		return -EOVERFLOW;
	}

	if ((err = snprintf(dst->str + dst->len, dst->size - dst->len,
	                    "%.*s", (int)len, src)) > 0) {
		dst->len += err;
	}

	return err;
}

int string_append_fmt(string_t *dst, const char *fmt, ...)
{
	va_list args;
	char *formatted;
	int len;
	int err;

	va_start(args, fmt);
	len = vasprintf(&formatted, fmt, args);
	va_end(args);

	if (len < 0) {
		return -ENOMEM;
	}

	err = string_append_utf8(dst, formatted, len);
	free(formatted);

	return err;
}

int string_replace(string_t *str, const char *search, const char *replace)
{
	string_t *new;
	char *occurrence;
	char *cur;
	int err;
	int search_len;
	int replace_len;

	search_len = strlen(search);
	replace_len = strlen(replace);

	if ((err = string_new(&new)) < 0) {
		return err;
	}

	cur = str->str;

	while ((occurrence = strstr(cur, search))) {
		int len;

		len = occurrence - cur;
		if ((err = string_append_utf8(new, cur, len)) < 0) {
			goto cleanup;
		}

		if ((err = string_append_utf8(new, replace, replace_len)) < 0) {
			goto cleanup;
		}

		cur = occurrence + search_len;
	}

	if ((err = string_append_utf8(new, cur, strlen(cur))) < 0) {
		goto cleanup;
	}

cleanup:
	if (err >= 0) {
		string_t swap;

		memcpy(&swap, str, sizeof(swap));
		memcpy(str, new, sizeof(*str));
		memcpy(new, &swap, sizeof(*new));
	}

	string_free(&new);
	return err;
}

int string_get_utf8(string_t *str, char **dst)
{
	char *utf8;

	if (!str || !dst) {
		return -EINVAL;
	}

	if (!(utf8 = strdup(str->str))) {
		return -ENOMEM;
	}

	*dst = utf8;
	return strlen(utf8);
}
