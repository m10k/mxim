/*
 * dict.h - This file is part of mxim
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

#ifndef DICT_H
#define DICT_H

#include "char.h"

typedef struct dict_candidate dict_candidate_t;

struct dict_candidate {
	char *value;
	int priority;
};

typedef struct dict_entry dict_entry_t;

struct dict_entry {
	int priority;
	char_t *key;
	char *key_utf8;
	dict_candidate_t **candidates;
	size_t num_candidates;
};

typedef struct dict dict_t;

int dict_candidate_new(dict_candidate_t **candidate);
int dict_candidate_free(dict_candidate_t **candidate);

int dict_entry_new(dict_entry_t **entry);
int dict_entry_free(dict_entry_t **entry);
int dict_entry_add(dict_entry_t *entry,
                   const dict_candidate_t **candidates,
                   const size_t num_candidates);

int dict_new(dict_t **dict);
int dict_free(dict_t **dict);
int dict_add(dict_t *dict,
             dict_entry_t **entries,
             const size_t num_entries);

int dict_lookup(const dict_t *dict, const char_t *key, dict_entry_t ***output);

#endif /* DICT_H */
