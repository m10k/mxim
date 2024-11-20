/*
 * inputcontext.c - This file is part of mxim
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

#include "inputcontext.h"
#include <errno.h>
#include <stdlib.h>

struct input_context {

};

int input_context_new(input_context_t **ic)
{
	input_context_t *context;

	if (!ic) {
		return -EINVAL;
	}

	if (!(context = calloc(1, sizeof(*context)))) {
		return -ENOMEM;
	}

	*ic = context;
	return 0;
}

int input_context_free(input_context_t **ic)
{
	if (!ic || !*ic) {
		return -EINVAL;
	}

	free(*ic);
	*ic = NULL;
	return 0;
}
