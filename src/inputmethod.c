/*
 * inputmethod.c - This file is part of mxim
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

#include "inputmethod.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>

struct input_method {

};

struct im_list {
	const char *locale;
	input_method_t *im;
	struct im_list *next;
};

static struct im_list *_input_methods = NULL;

int input_method_register(input_method_t *im, const char *locale)
{
	struct im_list *entry;
	struct im_list **last;

	if (!im) {
		return -EINVAL;
	}

	if (!(entry = calloc(1, sizeof(*entry)))) {
		return -ENOMEM;
	}

	entry->locale = locale;
	entry->im = im;
	for (last = &_input_methods; *last; ) {
		last = &(*last)->next;
	}
	*last = entry;

	return 0;
}

input_method_t* input_method_for_locale(const char *locale)
{
	struct im_list *cur;

	for (cur = _input_methods; cur; cur = cur->next) {
		if (!locale || /* caller is okay with anything */
		    !cur->locale || /* IM can handle any locale */
		    strcmp(locale, cur->locale) == 0) {
			return cur->im;
		}
	}

	return NULL;
}

int input_method_new(input_method_t **im)
{
	input_method_t *method;

	if (!im) {
		return -EINVAL;
	}

	if (!(method = calloc(1, sizeof(*method)))) {
		return -ENOMEM;
	}

	*im = method;
	return 0;
}

int input_method_free(input_method_t **im)
{
	if (!im || !*im) {
		return -EINVAL;
	}

	free(*im);
	*im = NULL;
	return 0;
}
