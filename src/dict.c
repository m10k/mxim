/*
 * dict.c - This file is part of mxim
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
#include "dict.h"
#include "trie.h"
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct dict {
	trie_t *trie;
};

int dict_candidate_new(dict_candidate_t **candidate)
{
	dict_candidate_t *c;

	if (!(c = calloc(1, sizeof(*c)))) {
		return -ENOMEM;
	}

	*candidate = c;
	return 0;
}

int dict_candidate_free(dict_candidate_t **candidate)
{
	if (!candidate || !*candidate) {
		return -EINVAL;
	}

	free((*candidate)->value);
	(*candidate)->value = NULL;

	free(*candidate);
	*candidate = NULL;

	return 0;
}

int dict_entry_new(dict_entry_t **entry)
{
	dict_entry_t *ent;

	if (!(ent = calloc(1, sizeof(*ent)))) {
		return -ENOMEM;
	}

	*entry = ent;

	return 0;
}

int dict_entry_free(dict_entry_t **entry)
{
	size_t i;

	if (!entry || !*entry) {
		return -EINVAL;
	}

	free((*entry)->key);
	(*entry)->key = NULL;

	for (i = 0; i < (*entry)->num_candidates; i++) {
		dict_candidate_free(&(*entry)->candidates[i]);
	}
	free((*entry)->candidates);
	(*entry)->candidates = NULL;

	free(*entry);
	*entry = NULL;

	return 0;
}

int dict_entry_add(dict_entry_t *entry,
                   const dict_candidate_t **candidates,
                   const size_t num_candidates)
{
	dict_candidate_t **new_candidates;
	size_t new_num_candidates;

	if (!entry || !candidates) {
		return -EINVAL;
	}

	if (SIZE_MAX - entry->num_candidates <= num_candidates) {
		return -EOVERFLOW;
	}

	new_num_candidates = entry->num_candidates + num_candidates;
	if (!(new_candidates = realloc(entry->candidates,
	                               (new_num_candidates + 1) * sizeof(*new_candidates)))) {
		return -ENOMEM;
	}

	memcpy(new_candidates + entry->num_candidates,
	       candidates, sizeof(*candidates) * num_candidates);
	new_candidates[new_num_candidates] = NULL;

	entry->candidates = new_candidates;
	entry->num_candidates = new_num_candidates;

	return 0;
}

int dict_new(dict_t **dict)
{
	dict_t *d;
	int err;

	if (!(d = calloc(1, sizeof(*d)))) {
		return -ENOMEM;
	}

	err = trie_new(&d->trie);

	if (err) {
		dict_free(&d);
	} else {
		*dict = d;
	}

	return err;
}

int dict_free(dict_t **dict)
{
	if (!dict || !*dict) {
		return -EINVAL;
	}

	trie_free(&(*dict)->trie);

	free(*dict);
	*dict = NULL;

	return 0;
}

int dict_add(dict_t *dict, dict_entry_t **entries, const size_t num_entries)
{
	size_t i;
	int err;

	if (!dict || !entries) {
		return -EINVAL;
	}

	err = 0;

	for (i = 0; i < num_entries; i++) {
		if ((err = trie_insert(dict->trie, entries[i]->key,
		                       (const void**)&entries[i], 1)) < 0) {
			break;
		}
	}

	return err;
}

int dict_lookup(const dict_t *dict, const char_t *key, dict_entry_t ***output)
{
	if (!dict || !key || !output) {
		return -EINVAL;
	}

	return trie_get_values(dict->trie, key, (void***)output);
}
