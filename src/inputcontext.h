/*
 * inputmethod.h - This file is part of mxim
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

#ifndef INPUTCONTEXT_H
#define INPUTCONTEXT_H

#include "char.h"
#include "ximtypes.h"
#include "ximclient.h"
#include <X11/Xlib.h>

typedef struct input_context input_context_t;
typedef struct input_method input_method_t;

int input_context_new(input_context_t **dst, xim_client_t *client, const int im, const int ic);
int input_context_free(input_context_t **ic);
int input_context_set_attribute(input_context_t *ic, attr_value_t *val);
int input_context_get_attribute(input_context_t *ic, int id, attr_value_t **val);

int input_context_set_data(input_context_t *ic, void *priv);
int input_context_get_data(input_context_t *ic, void **priv);
int input_context_get_im(input_context_t *ic);
int input_context_get_ic(input_context_t *ic);
int input_context_get_client(input_context_t *ic, xim_client_t **client);

int input_context_insert(input_context_t *ic, const char_t chr);
int input_context_erase(input_context_t *ic, int dir);

int input_context_set_language(input_context_t *ic, const lang_t language);
int input_context_get_language(input_context_t *ic, lang_t *language);

int input_context_cursor_move(input_context_t *ic, const short segment, const short offset);

int input_context_move_candidate(input_context_t *ic, const int dir);
int input_context_select_candidate(input_context_t *ic, const unsigned int candidate);

int input_context_move_segment(input_context_t *ic, const int dir);
int input_context_insert_segment(input_context_t *ic);

int input_context_redraw(const input_context_t *ic);
int input_context_commit(input_context_t *ic);

#endif /* INPUTCONTEXT_H */
