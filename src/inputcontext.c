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

#include "char.h"
#include "inputcontext.h"
#include "inputmethod.h"
#include "preedit.h"
#include "ximclient.h"
#include "ximtypes.h"
#include "xhandler.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>

extern x_handler_t *xhandler;

struct input_context {
	int im;
	int ic;

	struct {
		const attr_t *attr;
		attr_value_t *value;
	} attrs[IM_ICATTR_MAX];

	Window window;
	xim_client_t *client;
	preedit_t *preedit;
	void *priv;

	lang_t lang;
};

int input_context_new(input_context_t **dst, xim_client_t *client, const int im, const int ic)
{
	input_method_t *method;
	input_context_t *context;
	int err;
	int i;

	if (!dst) {
		return -EINVAL;
	}

	if ((err = xim_client_get_im(client, im, &method)) < 0) {
		return err;
	}

	if (!(context = calloc(1, sizeof(*context)))) {
		return -ENOMEM;
	}

	err = preedit_new(&(context->preedit));

	if (!err) {
		context->client = client;
		context->im = im;
		context->ic = ic;

		for (i = err = 0; i < IM_ICATTR_MAX; i++) {
			context->attrs[i].attr = method->ic_attrs[i].attr;

			if (method->ic_attrs[i].value) {
				if ((err = attr_value_clone(&context->attrs[i].value,
				                            method->ic_attrs[i].value)) < 0) {
					break;
				}
			}
		}
	}

	if (err) {
		input_context_free(&context);
	} else {
		*dst = context;
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

int input_context_set_attribute(input_context_t *ic, attr_value_t *val)
{
	attr_value_t *clone;
	int err;
	int idx;

	if (!ic || !val) {
		return -EINVAL;
	}

	if (val->id == 0 || val->id > IM_ICATTR_MAX) {
		return -EBADSLT;
	}

	idx = val->id - 1;

	if ((err = attr_value_clone(&clone, val)) < 0) {
		return err;
	}

	attr_value_free(&ic->attrs[idx].value);
	ic->attrs[idx].value = clone;

	if (strcmp(ic->attrs[idx].attr->name, XNClientWindow) == 0) {
		x_handler_get_client_window(xhandler, (Window)*(uint32_t*)clone->data,
		                            &ic->window);
	}

	return 0;
}

int input_context_get_attribute(input_context_t *ic, int id, attr_value_t **val)
{
	int idx;

	if (!ic || !val || id <= 0 || id > IM_ICATTR_MAX) {
		return -EINVAL;
	}

	idx = id - 1;

	if (!ic->attrs[idx].value) {
		return -ENOENT;
	}

	return attr_value_clone(val, ic->attrs[idx].value);
}

int input_context_set_data(input_context_t *ic, void *data)
{
	if (!ic) {
		return -EINVAL;
	}

	ic->priv = data;
	return 0;
}

int input_context_get_data(input_context_t *ic, void **data)
{
	if (!ic || !data) {
		return -EINVAL;
	}

	*data = ic->priv;
	return 0;
}

int input_context_get_im(input_context_t *ic)
{
	return ic->im;
}

int input_context_get_ic(input_context_t *ic)
{
	return ic->ic;
}

int input_context_get_client(input_context_t *ic, xim_client_t **client)
{
	if (!ic || !client) {
		return -EINVAL;
	}

	*client = ic->client;
	return 0;
}

int input_context_insert(input_context_t *ic, const char_t chr)
{
	preedit_dir_t dir;

	dir.segment = 0;
	dir.offset = 1;

	return preedit_insert(ic->preedit, chr, dir);
}

int input_context_erase(input_context_t *ic, int dir)
{
	preedit_dir_t cursor_dir;

	cursor_dir.segment = 0;
	cursor_dir.offset = dir;

	return preedit_erase(ic->preedit, cursor_dir);
}

int input_context_set_language(input_context_t *ic, const lang_t language)
{
	if (!ic) {
		return -EINVAL;
	}

	ic->lang = language;
	return 0;
}

int input_context_get_language(input_context_t *ic, lang_t *lang)
{
	if (!ic || !lang) {
		return -EINVAL;
	}

	*lang = ic->lang;
	return 0;
}
