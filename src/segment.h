/*
 * segment.h - This file is part of mxim
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

#ifndef SEGMENT_H
#define SEGMENT_H

#include "char.h"
#include <limits.h>

struct segment {
	char_t *input;
	short size;
	short len;
};

typedef struct segment segment_t;

#define SEGMENT_START SHRT_MIN
#define SEGMENT_END   SHRT_MAX

int segment_new(segment_t **segment);
int segment_free(segment_t **segment);
int segment_erase(segment_t *segment, const short pos);
int segment_insert(segment_t *segment, const char_t chr, const short pos);
int segment_clear(segment_t *segment);

int segment_get_input(segment_t *segment, char *dst, const size_t dst_size);

#endif /* SEGMENT_H */
