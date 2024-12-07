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

#include "ximproto.h"
#include "inputmethod.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>

static input_method_t _null_im = {
	.locale = "invalid"
};

static input_method_t *_input_methods[] = {
	&_null_im
};

input_method_t* input_method_for_locale(const char *locale)
{
	int i;

	for (i = 0; i < (sizeof(_input_methods) / sizeof(_input_methods[0])); i++) {
		input_method_t *im;

		im = _input_methods[i];

		if (!locale || /* caller is okay with anything */
		    !im->locale || /* IM can handle any locale */
		    strcmp(locale, im->locale) == 0) {
			return im;
		}
	}

	return NULL;
}

static int _count_im_attrs(input_method_t *im)
{
	int count;
	int i;

	if (!im) {
		return -EINVAL;
	}

	for (count = i = 0; i < IM_IMATTR_MAX; i++) {
		if (im->im_attrs[i].attr) {
			count++;
		}
	}

	return count;
}

static int _count_ic_attrs(input_method_t *im)
{
	int count;
	int i;

	if (!im) {
		return -EINVAL;
	}

	for (count = i = 0; i < IM_ICATTR_MAX; i++) {
		if (im->ic_attrs[i].attr) {
			count++;
		}
	}

	return count;
}

int input_method_get_im_attrs(input_method_t *im, attr_t ***dst)
{
	attr_t **attrs;
	int num_attrs;
	int src_idx;
	int dst_idx;

	if ((num_attrs = _count_im_attrs(im)) < 0) {
		return num_attrs;
	}

	if (!(attrs = calloc(num_attrs + 1, sizeof(*attrs)))) {
		return -ENOMEM;
	}

	for (src_idx = dst_idx = 0; src_idx < IM_IMATTR_MAX; src_idx++) {
		const attr_t *src_attr;
		attr_t *attr;
		int err;

		/* attributes array may be fragmented */
		if (!(src_attr = im->im_attrs[src_idx].attr)) {
			continue;
		}

		if ((err = attr_clone(&attr, src_attr)) < 0) {
			attrs_free(&attrs);
			return err;
		}

		/* the position in the array + 1 is the attribute id */
		attr->id = src_idx + 1;
		attrs[dst_idx++] = attr;
	}

	*dst = attrs;
	return 0;
}

int input_method_get_ic_attrs(input_method_t *im, attr_t ***dst)
{
	attr_t **attrs;
	int num_attrs;
	int src_idx;
	int dst_idx;

	if ((num_attrs = _count_ic_attrs(im)) < 0) {
		return num_attrs;
	}

	if (!(attrs = calloc(num_attrs + 1, sizeof(*attrs)))) {
		return -ENOMEM;
	}

	for (src_idx = dst_idx = 0; src_idx < IM_ICATTR_MAX; src_idx++) {
		const attr_t *src_attr;
		attr_t *attr;
		int err;

		if (!(src_attr = im->ic_attrs[src_idx].attr)) {
			continue;
		}

		if ((err = attr_clone(&attr, im->ic_attrs[src_idx].attr)) < 0) {
			attrs_free(&attrs);
			return err;
		}

		attr->id = src_idx + 1;
		attrs[dst_idx++] = attr;
	}

	*dst = attrs;
	return 0;
}
