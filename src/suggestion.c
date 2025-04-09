/*
 * suggestion.c - This file is part of mxim
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

#define _POSIX_C_SOURCE 200809L

#include "suggestion.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int suggestion_new(suggestion_t **suggestion, const char *value, const char *display)
{
	suggestion_t *sug;

	if (!suggestion || !value) {
		return -EINVAL;
	}

	if (!(sug = malloc(sizeof(*sug)))) {
		return -ENOMEM;
	}

	sug->display = display ? strdup((char*)display) : NULL;
	sug->value = value ? strdup((char*)value) : NULL;

	if ((display != NULL) != (sug->display != NULL) ||
	    (value != NULL) != (sug->value != NULL)) {
		suggestion_free(&sug);
		return -ENOMEM;
	}

	*suggestion = sug;
	return 0;
}

int suggestion_free(suggestion_t **suggestion)
{
	if (!suggestion) {
		return -EINVAL;
	}

	free((*suggestion)->value);
	free((*suggestion)->display);
	free(*suggestion);
	*suggestion = NULL;
	return 0;
}

const char *suggestion_get_value(suggestion_t *suggestion)
{
	return suggestion->value;
}

const char *suggestion_get_display(suggestion_t *suggestion)
{
	return suggestion->display ? suggestion->display : suggestion->value;
}

int suggestion_cmp(const suggestion_t *a, const suggestion_t *b)
{
	return strcmp(a->value, b->value);
}
