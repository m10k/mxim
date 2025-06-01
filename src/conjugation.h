/*
 * conjugation.h - This file is part of mxim
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

#ifndef MXIM_CONJUGATION_H
#define MXIM_CONJUGATION_H

#include "char.h"

typedef struct conjugation conjugation_t;

struct conjugation {
	char_t *dict_form;
	int suffix_len;
	char_t *conjugation;
	int conjugation_len;
	int type;
};

int conjugation_new(conjugation_t **dst, const char_t *dict_form, const int suffix_len,
                    const char_t *conjugation, const int conjugation_len);
int conjugation_free(conjugation_t **conjugation);

int conjugation_conjugate(char **dst, const char *src, conjugation_t *conjugation);

#endif /* MXIM_CONJUGATION_H */
