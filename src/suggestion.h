/*
 * suggestion.h - This file is part of mxim
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

#ifndef SUGGESTION_H
#define SUGGESTION_H

struct suggestion {
	char *display;
	char *value;
};

typedef struct suggestion suggestion_t;

int suggestion_new(suggestion_t **suggestion, const char *value, const char *display);
int suggestion_free(suggestion_t **suggestion);

const char* suggestion_get_value(suggestion_t *suggestion);
const char* suggestion_get_display(suggestion_t *suggestion);
int suggestion_cmp(const suggestion_t *a, const suggestion_t *b);

#endif /* SUGGESTION_H */
