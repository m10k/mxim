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
#include "inputmethod.h"
#include "ximtypes.h"
#include <errno.h>
#include <stdlib.h>

struct input_context {
	struct {
		const attr_t *attr;
		attr_value_t *value;
	} attrs[IM_ICATTR_MAX];
};

int input_context_new(input_context_t **ic, input_method_t *im)
{
	input_context_t *context;
	int err;
	int i;

	if (!ic) {
		return -EINVAL;
	}

	if (!(context = calloc(1, sizeof(*context)))) {
		return -ENOMEM;
	}

	for (i = err = 0; i < IM_ICATTR_MAX; i++) {
		context->attrs[i].attr = im->ic_attrs[i].attr;

		if (im->ic_attrs[i].value) {
			if ((err = attr_value_clone(&context->attrs[i].value, im->ic_attrs[i].value)) < 0) {
				break;
			}
		}
	}

	if (err) {
		input_context_free(&context);
	} else {
		*ic = context;
	}

	return err;
}

int input_context_free(input_context_t **ic)
{
	int i;

	if (!ic || !*ic) {
		return -EINVAL;
	}

	for (i = 0; i < IM_ICATTR_MAX; i++) {
		attr_value_free(&(*ic)->attrs[i].value);
	}

	free(*ic);
	*ic = NULL;
	return 0;
}
