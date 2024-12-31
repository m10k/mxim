/*
 * preedit.h - This file is part of mxim
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

#ifndef PREEDIT_H
#define PREEDIT_H

#include "segment.h"
#include "string.h"
#include <limits.h>

#define PREEDIT_SEGMENT_FIRST SHRT_MIN
#define PREEDIT_SEGMENT_LAST  SHRT_MAX
#define PREEDIT_SEGMENT_START SEGMENT_START
#define PREEDIT_SEGMENT_END   SEGMENT_END

struct preedit_iter {
	short segment;
	short offset;
};

typedef struct preedit_iter preedit_cursor_t;
typedef struct preedit_iter preedit_dir_t;

typedef struct preedit preedit_t;

int preedit_new(preedit_t **preedit);
int preedit_free(preedit_t **preedit);

int preedit_move(preedit_t *preedit, preedit_dir_t cursor_dir);
int preedit_erase(preedit_t *preedit, preedit_dir_t cursor_dir);
int preedit_insert(preedit_t *preedit, char_t chr, preedit_dir_t cursor_dir);
int preedit_clear(preedit_t *preedit);

int preedit_get_input(preedit_t *preedit, char *dst, const size_t dst_size);
int preedit_get_input_decorated(const preedit_t *preedit, char **dst);
int preedit_get_output(const preedit_t *preedit, char *dst, const size_t dst_size);

int preedit_move_candidate(preedit_t *preedit, const int dir);
int preedit_select_candidate(preedit_t *preedit, const unsigned int candidate);
int preedit_move_segment(preedit_t *preedit, const int dir);
int preedit_insert_segment(preedit_t *preedit);

#endif /* PREEDIT_H */
