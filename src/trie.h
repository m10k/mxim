/*
 * trie.h - This file is part of mxim
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

#ifndef TRIE_H
#define TRIE_H

#include "char.h"
#include <stddef.h>

typedef struct trie trie_t;

int trie_new(trie_t **trie);
int trie_free(trie_t **trie);

int trie_insert(trie_t *trie, const char_t *key, const void **values, const size_t num_values);
int trie_add_values(trie_t *trie, const void **values, const size_t num_values);
int trie_get_values(trie_t *trie, const char_t *key, void ***values);

#endif /* TRIE_H */
