/*
 * inputmethod.h - This file is part of mxim
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

#ifndef INPUTMETHOD_H
#define INPUTMETHOD_H

#include "ximtypes.h"
#include "inputcontext.h"
#include <stdint.h>
#include <X11/Xlib.h>

typedef struct input_method input_method_t;

struct input_method {
	/* Unique ID of the input method */
	uint16_t id;

	/* The input style that is implemented by the input method */
	XIMStyle input_style;

	/* Attributes of the input method */
	const attr_t *im_attrs;
	attr_value_t **im_values;

	/* The attributes and default values of input contexts */
	const attr_t *ic_attrs;
	attr_value_t **ic_values;

	/* The locale that is supported by the IM. NULL for any */
	char *locale;

	/* Extensions supported by the input method */
	const ext_t *exts;

	/* The encodings supported by the IM */
	const char **encodings;

	/* Event handler called for each XIM_FORWARD_EVENT message */
	int (*event)(input_method_t*, input_context_t*, int, int, XEvent*);
};

input_method_t* input_method_for_locale(const char *locale);

#endif /* INPUTMETHOD_H */
