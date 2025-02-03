/*
 * preedit.c - This file is part of mxim
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

#include "preedit.h"
#include "segment.h"
#include <errno.h>
#include <stdlib.h>

struct preedit {
        segment_t **segments;
	short num_segments;

	preedit_cursor_t cursor;
};

int preedit_new(preedit_t **preedit)
{
	preedit_t *p;
	int err;

	if (!(p = calloc(1, sizeof(*p)))) {
		return -ENOMEM;
	}

	if (!(p->segments = calloc(1, sizeof(*p->segments)))) {
		err = -ENOMEM;
	} else {
		err = segment_new(&p->segments[0]);
	}

	if (!err) {
		p->num_segments = 1;
		*preedit = p;
	} else {
		preedit_free(&p);
	}

	return err;
}

int preedit_free(preedit_t **preedit)
{
	if (!preedit || !*preedit) {
		return -EINVAL;
	}

	while ((*preedit)->num_segments > 0) {
		segment_free(&(*preedit)->segments[--(*preedit)->num_segments]);
	}
	free((*preedit)->segments);
	(*preedit)->segments = NULL;

	free(*preedit);
	*preedit = NULL;

	return 0;
}

int preedit_move(preedit_t *preedit, preedit_dir_t cursor_dir)
{
	if (!preedit) {
		return -EINVAL;
	}

	/* Assumption: The preedit always has at least one segment */

	if (cursor_dir.segment == PREEDIT_SEGMENT_FIRST) {
		preedit->cursor.segment = 0;
	} else if (cursor_dir.segment == PREEDIT_SEGMENT_LAST) {
		preedit->cursor.segment = preedit->num_segments - 1;
	} else {
		preedit->cursor.segment += cursor_dir.segment;

		if (preedit->cursor.segment < 0) {
			preedit->cursor.segment = 0;
		} else if (preedit->cursor.segment >= preedit->num_segments) {
			preedit->cursor.segment = preedit->num_segments - 1;
		}
	}

	if (cursor_dir.offset == PREEDIT_SEGMENT_START) {
		preedit->cursor.offset = 0;
	} else if (cursor_dir.offset == PREEDIT_SEGMENT_END) {
		preedit->cursor.offset = preedit->segments[preedit->cursor.segment]->len;
	} else {
		segment_t *segm;

		segm = preedit->segments[preedit->cursor.segment];
		preedit->cursor.offset += cursor_dir.offset;

		if (preedit->cursor.offset < 0) {
			preedit->cursor.offset = 0;
		} else if (preedit->cursor.offset > segm->len) {
			preedit->cursor.offset = segm->len;
		}
	}

	return 0;
}

int preedit_erase(preedit_t *preedit, preedit_dir_t cursor_dir)
{
	int err;

	if (!preedit) {
		return -EINVAL;
	}

	if ((err = preedit_move(preedit, cursor_dir)) < 0) {
		return err;
	}

	return segment_erase(preedit->segments[preedit->cursor.segment],
	                     preedit->cursor.offset);
}

int preedit_insert(preedit_t *preedit, char_t character, preedit_dir_t cursor_dir)
{
	int err;

	if (!preedit) {
		return -EINVAL;
	}

	err = segment_insert(preedit->segments[preedit->cursor.segment],
	                     character, preedit->cursor.offset);

	if (err > 0) {
		err = preedit_move(preedit, cursor_dir);
	}

	return err;
}

int preedit_clear(preedit_t *preedit)
{
	int err;

	/* Remove all but the first segment */
	while (preedit->num_segments > 1) {
		if ((err = segment_free(&preedit->segments[--preedit->num_segments])) < 0) {
			return err;
		}
	}

	preedit->cursor.segment = 0;
	preedit->cursor.offset = 0;

	return segment_clear(preedit->segments[0]);
}

int preedit_get_input(preedit_t *preedit, char *dst, const size_t dst_size)
{
	size_t offset;
	int err;
	short i;

	if (!preedit || !dst) {
		return -EINVAL;
	}

	for (offset = 0, i = 0; i < preedit->num_segments; i++) {
		if ((err = segment_get_input(preedit->segments[i],
		                             dst + offset,
		                             dst_size - offset)) < 0) {
			return err;
		}

		offset += err;
	}

	return (int)offset;
}

int preedit_get_input_decorated(const preedit_t *preedit, char **dst)
{
	string_t *input;
	int err;
	short i;

	if (!preedit || !dst) {
		return -EINVAL;
	}

	if ((err = string_new(&input)) < 0) {
		return -ENOMEM;
	}

	for (i = 0; i < preedit->num_segments; i++) {
		char *segment;
		int selected;
		int cursor;
		int len;

		segment = NULL;

		if (preedit->cursor.segment == i) {
			cursor = preedit->cursor.offset;
			selected = 1;
		} else {
			cursor = -1;
			selected = 0;
		}

		if ((len = segment_get_input_decorated(preedit->segments[i], selected,
		                                       cursor, &segment)) < 0) {
			err = len;
			goto cleanup;
		}

		err = string_append_utf8(input, segment, len);
		free(segment);

		if (err < 0) {
			goto cleanup;
		}
	}

cleanup:
	if (err >= 0) {
		err = string_get_utf8(input, dst);
	}
	string_free(&input);

	return err;
}

int preedit_get_output(const preedit_t *preedit, char *dst, const size_t dst_size)
{
	int offset;
	int i;

	for (offset = i = 0; i < preedit->num_segments; i++) {
		int len;

		if ((len = segment_get_output(preedit->segments[i],
		                              dst + offset,
		                              dst_size - offset)) < 0) {
			break;
		}

		offset += len;
	}

	return offset;
}

int preedit_move_candidate(preedit_t *preedit, const int dir)
{
	segment_t *segment;

	if (!preedit) {
		return -EINVAL;
	}

	if (preedit->cursor.segment < 0 ||
	    preedit->cursor.segment >= preedit->num_segments) {
		return -EBADFD;
	}

	if (!(segment = preedit->segments[preedit->cursor.segment])) {
		return -ENOENT;
	}

	return segment_move_candidate(segment, dir);
}

int preedit_select_candidate(preedit_t *preedit, const unsigned int candidate)
{
	segment_t *segment;

	if (!preedit) {
		return -EINVAL;
	}

	if (preedit->cursor.segment < 0 ||
	    preedit->cursor.segment >= preedit->num_segments) {
		return -EBADFD;
	}

	if (!(segment = preedit->segments[preedit->cursor.segment])) {
		return -ENOENT;
	}

	return segment_select_candidate(segment, (int)candidate);
}

int preedit_move_segment(preedit_t *preedit, const int dir)
{
	preedit_dir_t cursor_dir;

	if (!preedit) {
		return -EINVAL;
	}

	cursor_dir.segment = dir;
	cursor_dir.offset = PREEDIT_SEGMENT_END;

	return preedit_move(preedit, cursor_dir);
}

int preedit_insert_segment(preedit_t *preedit)
{
	segment_t *new_segment;
	segment_t **new_segments;
	short new_num_segments;
	int err;
	int src_idx;
	int dst_idx;

	if ((err = segment_new(&new_segment)) < 0) {
		return err;
	}

	new_num_segments = preedit->num_segments + 1;

	if (!(new_segments = calloc(new_num_segments, sizeof(*new_segments)))) {
		segment_free(&new_segment);
		return -ENOMEM;
	}

	for (src_idx = dst_idx = 0; src_idx < preedit->num_segments; src_idx++) {
		new_segments[dst_idx++] = preedit->segments[src_idx];

		/* insert new segment after the current */
		if (src_idx == preedit->cursor.segment) {
			new_segments[dst_idx++] = new_segment;
		}
	}

	free(preedit->segments);
	preedit->segments = new_segments;
	preedit->num_segments = new_num_segments;

	return 0;
}

int preedit_update_candidates(preedit_t *preedit)
{
	segment_t *segment;

	if (!preedit) {
		return -EINVAL;
	}

	if (preedit->cursor.segment < 0 ||
	    preedit->cursor.segment >= preedit->num_segments) {
		return -EBADFD;
	}

	if (!(segment = preedit->segments[preedit->cursor.segment])) {
		return -ENOENT;
	}

	return segment_update_candidates(segment);
}

int preedit_is_empty(const preedit_t *preedit)
{
	if (!preedit) {
		return -EINVAL;
	}

	return preedit->num_segments == 1 && preedit->segments[0]->len == 0;
}
