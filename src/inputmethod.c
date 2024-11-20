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
