/*
 * array.h - This file is part of mxim
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

#ifndef MXIM_ARRAY_H
#define MXIM_ARRAY_H

#define ARRAY_DONT_FREE    ((int(*)(void**))0)
#define ARRAY_GENERIC_FREE ((int(*)(void**))1)

int array_add(void ***array, void **items, const int num_items);
int array_free(void ***array, int (*dealloc)(void**));
int array_foreach(void ***array, int (*func)(void*, void*), void *data);
int array_len(const void ***array);

#endif /* MXIM_ARRAY_H */
