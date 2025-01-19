/*
 * parray.h - This file is part of mxim
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

#ifndef PARRAY_H
#define PARRAY_H

typedef struct parray parray_t;

int parray_new(parray_t **parray, int (*cmp)(const void*, const void*));
int parray_free(parray_t **parray);

int parray_insert(parray_t *parray, const void **data, const int n);
int parray_get_items(parray_t *parray, const void ***items);

#endif /* PARRAY_H */
