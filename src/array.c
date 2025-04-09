/*
 * array.c - This file is part of mxim
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

#include "array.h"
#include <errno.h>
#include <stdlib.h>

int array_add(void ***array, void *item)
{
        void **arr;
        int len;

        if (!array) {
                return -EINVAL;
        }

        len = 0;

        if (*array) {
                while ((*array)[len]) {
                        len++;
                }
        }

        if (!(arr = realloc(*array, (len + 1 + 1) * sizeof(*arr)))) {
                return -ENOMEM;
        }

        arr[len] = item;
        arr[len + 1] = NULL;

        *array = arr;
        return len;
}

int array_free(void ***array)
{
        int i;

        if (!array) {
                return -EINVAL;
        }

        if (*array) {
                for (i = 0; (*array)[i]; i++) {
                        free((*array)[i]);
                        (*array)[i] = NULL;
                }

                free(*array);
                *array = NULL;
        }

        return 0;
}

int array_foreach(void ***array, int (*func)(void*, void*), void *data)
{
	int err;
	int i;

	if (!array || !func) {
		return -EINVAL;
	}

	if (!*array) {
		return -ENOENT;
	}

	for (i = 0, err = 0; (*array)[i]; i++) {
		if ((err = func((*array)[i], data)) < 0) {
			break;
		}
	}

	return err;
}

int array_len(const void ***array)
{
	int len;

	if (!array) {
		return -EINVAL;
	}

	for (len = 0; *array && (*array)[len]; len++);

	return len;
}
