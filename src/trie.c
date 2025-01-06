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
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct trie {
	trie_t *children[256];

	char **candidates;
	int num_candidates;
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

int trie_insert(trie_t *trie, const char_t *key, const char **candidates, const size_t num_candidates)
{
	int err;

	if (!trie || !key || !candidates || !num_candidates) {
		return -EINVAL;
	}

	if (*key != CHAR_INVALID) {
		if (!trie->children[*key] &&
		    (err = trie_new(&trie->children[*key])) < 0) {
			return err;
		}

		return trie_insert(trie->children[*key], key + 1, candidates, num_candidates);
	}

	return trie_add_candidates(trie, candidates, num_candidates);
}

int trie_add_candidates(trie_t *trie, const char **candidates, const size_t num_candidates)
{
	size_t new_num_candidates;
	char **new_candidates;

	if (SIZE_MAX - trie->num_candidates <= num_candidates) {
		return -EOVERFLOW;
	}

	new_num_candidates = trie->num_candidates + num_candidates;
	new_candidates = realloc(trie->candidates, (new_num_candidates + 1) * sizeof(*new_candidates));

	if (!new_candidates) {
		return -ENOMEM;
	}

	memmove(new_candidates + trie->num_candidates, candidates, num_candidates * sizeof(*candidates));
	trie->candidates = new_candidates;
	trie->num_candidates = new_num_candidates;

	return 0;
}

int candidate_new(candidate_t **candidate, const char *str)
{
	candidate_t *c;

	if (!candidate || !str) {
		return -EINVAL;
	}

	if (!(c = calloc(1, sizeof(*c)))) {
		return -ENOMEM;
	}

	c->candidate = str;
	*candidate = c;
	return 0;
}

int trie_append_to_list(trie_t *trie, candidate_t **list)
{
	int i;

	if (!trie || !list) {
		return -EINVAL;
	}

	while (*list) {
		list = &(*list)->next;
	}

	for (i = 0; i < trie->num_candidates; i++) {
		if (candidate_new(list, trie->candidates[i]) < 0) {
			break;
		}

		list = &(*list)->next;
	}

	return 0;
}

int trie_get_candidates(trie_t *trie, const char_t *key, candidate_t **candidates)
{
	int i;

	if (!trie || !key || !candidates) {
		return -EINVAL;
	}

	if (*key != CHAR_INVALID) {
		return trie_get_candidates(trie->children[*key], key + 1, candidates);
	} else {
		trie_append_to_list(trie, candidates);

		for (i = 0; i < (sizeof(trie->children) / sizeof(trie->children[0])); i++) {
			trie_get_candidates(trie->children[i], key, candidates);
		}
	}

	return 0;
}
