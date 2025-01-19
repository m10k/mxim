/*
 * parray.c - This file is part of mxim
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

#include "parray.h"
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct parray {
	int (*cmp)(const void*, const void*);
	const void **items;
	int num_items;
};

int parray_new(parray_t **parray, int (*cmp)(const void*, const void*))
{
	parray_t *pa;

	if (!(pa = calloc(1, sizeof(*pa)))) {
		return -ENOMEM;
	}

	pa->cmp = cmp;

	*parray = pa;
	return 0;
}

int parray_free(parray_t **parray)
{
	if (!parray || !*parray) {
		return -EINVAL;
	}

	free(*parray);
	*parray = NULL;

	return 0;
}

static int _find_slot(const void **items, const int num_items,
                      const void *data, int(*cmp)(const void*, const void*))
{
	int i;

	for (i = 0; i < num_items; i++) {
		if (cmp(items[i], data) > 0) {
			break;
		}
	}

	return i;
}

int parray_insert(parray_t *parray, const void **data, const int n)
{
	const void **new_items;
	int new_num_items;
	int slot;
	int i;

	if (!parray || !data || n < 0) {
		return -EINVAL;
	}

	if (INT_MAX - parray->num_items <= n) {
		return -ENOBUFS;
	}

	new_num_items = parray->num_items + n;
	if (!(new_items = realloc(parray->items, (new_num_items + 1) * sizeof(*parray->items)))) {
		return -ENOMEM;
	}

	for (i = 0; i < n; i++) {
		slot = _find_slot(new_items, parray->num_items, data[i], parray->cmp);
		memmove(&new_items[slot + 1],
		        &new_items[slot],
		        (parray->num_items - slot) * sizeof(*new_items));
		new_items[slot] = data[i];
		parray->num_items++;
	}
	parray->items = new_items;

	return 0;
}

int parray_get_items(parray_t *parray, const void ***items)
{
        void **arr;

	if (!(arr = malloc((parray->num_items + 1) * sizeof(*arr)))) {
		return -ENOMEM;
	}

	memcpy(arr, parray->items, parray->num_items * sizeof(*arr));
	arr[parray->num_items] = NULL;

	*items = (const void**)arr;
	return 0;
}
