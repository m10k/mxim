/*
 * trie.c - This file is part of mxim
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

#include "char.h"
#include "trie.h"
#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct trie {
	trie_t *children[256];

	void **values;
	int num_values;
};

int trie_new(trie_t **trie)
{
	trie_t *t;

	if (!(t = calloc(1, sizeof(*t)))) {
		return -ENOMEM;
	}

	*trie = t;
	return 0;
}

int trie_free(trie_t **trie)
{
	if (!trie || !*trie) {
		return -EINVAL;
	}

	free(*trie);
	*trie = NULL;

	return 0;
}

int trie_insert(trie_t *trie, const char_t *key, const void **values, const size_t num_values)
{
	int err;

	if (!trie || !key || !values || !num_values) {
		return -EINVAL;
	}

	if (*key != CHAR_INVALID) {
		if (!trie->children[*key] &&
		    (err = trie_new(&trie->children[*key])) < 0) {
			return err;
		}

		return trie_insert(trie->children[*key], key + 1, values, num_values);
	}

	return trie_add_values(trie, values, num_values);
}

int trie_insert_reverse(trie_t *trie, const char_t *key, const int key_len,
                        const void **values, const size_t num_values)
{
	char_t idx;

	if (!trie || !key || key_len < 0 || !values || !num_values) {
		return -EINVAL;
	}

	if (key_len == 0) {
		return trie_add_values(trie, values, num_values);
	}

	idx = key[key_len - 1];

	if (!trie->children[idx]) {
		int err;

		if ((err = trie_new(&trie->children[idx])) < 0) {
			return err;
		}
	}

	return trie_insert_reverse(trie->children[idx], key, key_len - 1,
	                           values, num_values);
}

int trie_add_values(trie_t *trie, const void **values, const size_t num_values)
{
	size_t new_num_values;
	void **new_values;

	if (SIZE_MAX - trie->num_values <= num_values) {
		return -EOVERFLOW;
	}

	new_num_values = trie->num_values + num_values;
	new_values = realloc(trie->values, (new_num_values + 1) * sizeof(*new_values));

	if (!new_values) {
		return -ENOMEM;
	}

	memmove(new_values + trie->num_values, values, num_values * sizeof(*values));
	trie->values = new_values;
	trie->num_values = new_num_values;

	return 0;
}

static int _trie_append_to_array(trie_t *trie, void ***array)
{
	void **new_array;
	int new_len;
	int len;

	len = 0;

	if (!trie || !array) {
		return -EINVAL;
	}

	if (*array) {
		while ((*array)[len]) {
			len++;
		}
	}

	if (INT_MAX - len <= trie->num_values) {
		return -EOVERFLOW;
	}

	new_len = len + trie->num_values;
	new_array = realloc(*array, (new_len + 1) * sizeof(**array));

	if (!new_array) {
		return -ENOMEM;
	}

	memmove(new_array + len, trie->values, sizeof(*trie->values) * trie->num_values);
	new_array[new_len] = NULL;
	*array = new_array;

	return new_len;
}

int trie_get_values(trie_t *trie, const char_t *key, const int mode, void ***values)
{
	int i;

	if (!trie || !key || !values) {
		return -EINVAL;
	}

	if (*key != CHAR_INVALID) {
		return trie_get_values(trie->children[*key], key + 1, mode, values);
	} else {
		_trie_append_to_array(trie, values);

		if (mode & TRIE_LOOKUP_PREDICT) {
			for (i = 0; i < (sizeof(trie->children) / sizeof(trie->children[0])); i++) {
				trie_get_values(trie->children[i], key, mode, values);
			}
		}
	}

	return 0;
}

int trie_get_values_reverse(trie_t *trie, const char_t *key, const int key_len,
                            const int mode, void ***values)
{
	char_t idx;
	int num_values;

	if (!trie || !key || !values) {
		return -EINVAL;
	}

	num_values = 0;

	if (key_len == 0 || mode == TRIE_LOOKUP_COLLECT) {
		/*
		 * Pick up values in the current node if the entire search key
		 * was matched, or the caller wants us to pick up everything
		 * along the way.
		 */
		num_values = _trie_append_to_array(trie, values);

		if (num_values < 0 || key_len == 0) {
			return num_values;
		}
	}

	idx = key[key_len - 1];

	if (!trie->children[idx]) {
		return num_values;
	}

	return trie_get_values_reverse(trie->children[idx], key, key_len - 1,
	                               mode, values);
}
