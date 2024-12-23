/*
 * segment.c - This file is part of mxim
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
#include "segment.h"
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_SEGMENT_SIZE 32

int segment_new(segment_t **segment)
{
	segment_t *seg;

	if (!(seg = calloc(1, sizeof(*seg)))) {
		return -ENOMEM;
	}

	if (!(seg->characters = calloc(INITIAL_SEGMENT_SIZE,
	                               sizeof(*seg->characters)))) {
		free(seg);
		return -ENOMEM;
	}

	seg->size = INITIAL_SEGMENT_SIZE;
	*segment = seg;
	return 0;
}

int segment_free(segment_t **segment)
{
	if (!segment || !*segment) {
		return -EINVAL;
	}

	free((*segment)->characters);
	free(*segment);
	*segment = 0;
	return 0;
}

int segment_erase(segment_t *segment, const short pos)
{
	short tail_len;

	if (!segment) {
		return -EINVAL;
	}

	if (pos < 0 || pos >= segment->len) {
		return -EOVERFLOW;
	}

	tail_len = segment->len - pos - 1;
	memmove(segment->characters + pos,
	        segment->characters + pos + 1,
	        sizeof(*segment->characters) * tail_len);

	segment->len--;
	segment->characters[segment->len] = CHAR_INVALID;

	return 0;
}

static int _segment_grow(segment_t *segment)
{
	char_t *new_characters;
	short new_size;

	if (SHRT_MAX - INITIAL_SEGMENT_SIZE < segment->size) {
		return -EMSGSIZE;
	}

	new_size = segment->size + INITIAL_SEGMENT_SIZE;
	if (!(new_characters = realloc(segment->characters,
	                               new_size * sizeof(*segment->characters)))) {
		return -ENOMEM;
	}

	segment->characters = new_characters;
	segment->size = new_size;

	return 0;
}

int segment_insert(segment_t *segment, const char_t chr,
                   const short pos)
{
	int err;
	short tail_len;
	short insert_pos;

	if (!segment) {
		return -EINVAL;
	}

	if (pos < 0) {
		insert_pos = 0;
	} else if (pos > segment->len) {
		insert_pos = segment->len;
	} else {
		insert_pos = pos;
	}

	if (segment->len == (segment->size - 1) &&
	    ((err = _segment_grow(segment)) < 0)) {
		return err;
	}

	tail_len = segment->len - insert_pos;
	memmove(segment->characters + insert_pos + 1,
	        segment->characters + insert_pos, tail_len);
	segment->characters[insert_pos] = chr;
	segment->len++;

	return 0;
}

int segment_clear(segment_t *segment)
{
	if (!segment) {
		return -EINVAL;
	}

	memset(segment->characters, 0, segment->size * sizeof(*segment->characters));
	segment->len = 0;

	return 0;
}
