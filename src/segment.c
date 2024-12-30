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
#include "string.h"
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_SEGMENT_SIZE 32

int segment_new(segment_t **segment)
{
	segment_t *seg;

	if (!(seg = calloc(1, sizeof(*seg)))) {
		return -ENOMEM;
	}

	if (!(seg->input = calloc(INITIAL_SEGMENT_SIZE,
	                          sizeof(*seg->input)))) {
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

	free((*segment)->input);
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
	memmove(segment->input + pos,
	        segment->input + pos + 1,
	        sizeof(*segment->input) * tail_len);

	segment->len--;
	segment->input[segment->len] = CHAR_INVALID;

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
	if (!(new_characters = realloc(segment->input,
	                               new_size * sizeof(*segment->input)))) {
		return -ENOMEM;
	}

	segment->input = new_characters;
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
	memmove(segment->input + insert_pos + 1,
	        segment->input + insert_pos, tail_len);
	segment->input[insert_pos] = chr;
	segment->len++;

	return 0;
}

int segment_clear(segment_t *segment)
{
	if (!segment) {
		return -EINVAL;
	}

	memset(segment->input, 0, segment->size * sizeof(*segment->input));
	segment->len = 0;

	return 0;
}

int segment_get_input(segment_t *segment, char *dst, const size_t dst_size)
{
	if (!segment || !dst) {
		return -EINVAL;
	}

	return char_to_utf8(segment->input, segment->len, dst, dst_size);
}

int segment_get_input_decorated(segment_t *segment, const int selected, const int cursor_pos, char **dst)
{
	static const char cursor[] = "<span foreground=\"grey\">⇱</span>";
	string_t *input;
	string_t *escape;
	int err;
	int i;

	escape = NULL;

	if ((err = string_new(&input)) < 0) {
		return err;
	}

	if (selected &&
	    (err = string_append_utf8(input, "[", 1)) < 0) {
		goto cleanup;
	}

	if (cursor_pos >= 0 && cursor_pos <= segment->len) {
		if (string_new(&escape) < 0 ||
		    (err = string_append_char(escape, segment->input, cursor_pos)) < 0 ||
		    (err = string_replace(escape, "&", "&amp;")) < 0 ||
		    (err = string_replace(escape, "<", "&lt;")) < 0 ||
		    (err = string_replace(escape, ">", "&gt;")) < 0 ||
		    (err = string_append(input, escape)) < 0 ||
		    (err = string_free(&escape)) < 0) {
			goto cleanup;
		}

		if ((err = string_append_utf8(input, cursor, sizeof(cursor))) < 0) {
			goto cleanup;
		}

		if (string_new(&escape) < 0 ||
		    (err = string_append_char(escape, segment->input + cursor_pos,
		                              segment->len - cursor_pos)) < 0 ||
		    (err = string_replace(escape, "&", "&amp;")) < 0 ||
		    (err = string_replace(escape, "<", "&lt;")) < 0 ||
		    (err = string_replace(escape, ">", "&gt;")) < 0 ||
		    (err = string_append(input, escape)) < 0 ||
		    (err = string_free(&escape)) < 0) {
			goto cleanup;
		}
	} else {
		if (string_new(&escape) < 0 ||
		    (err = string_append_char(escape, segment->input, segment->len)) < 0 ||
		    (err = string_replace(escape, "&", "&amp;")) < 0 ||
		    (err = string_replace(escape, "<", "&lt;")) < 0 ||
		    (err = string_replace(escape, ">", "&gt;")) < 0 ||
		    (err = string_append(input, escape)) < 0 ||
		    (err = string_free(&escape)) < 0) {
			goto cleanup;
		}
	}

	if (selected) {
		for (i = 0; i < segment->num_candidates; i++) {
			static const char _selection_header[] = "<span foreground=\"blue\">";
			static const char _selection_trailer[] = "</span>";

			if ((err = string_append_utf8(input, "|", 1)) < 0) {
				goto cleanup;
			}

			if (i == segment->selection &&
			    (err = string_append_utf8(input, _selection_header,
			                              sizeof(_selection_header))) < 0) {
				goto cleanup;
			}

			if ((err = string_new(&escape)) < 0 ||
			    (err = string_append_utf8(escape, segment->candidates[i],
			                              strlen(segment->candidates[i]))) < 0 ||
			    (err = string_replace(escape, "&", "&amp;")) < 0 ||
			    (err = string_replace(escape, "<", "&lt;")) < 0 ||
			    (err = string_replace(escape, ">", "&gt;")) < 0 ||
			    (err = string_append(input, escape)) < 0 ||
			    string_free(&escape) < 0) {
				goto cleanup;
			}

			if (segment->selection == i &&
			    (err = string_append_utf8(input, _selection_trailer,
			                              sizeof(_selection_trailer))) < 0) {
				goto cleanup;
			}
		}

		if ((err = string_append_utf8(input, "]", 1)) < 0) {
			goto cleanup;
		}
	}

cleanup:
	string_free(&escape);

	if (err >= 0) {
		err = string_get_utf8(input, dst);
	}

	string_free(&input);
	return err;
}

int segment_get_output(segment_t *segment, char *dst, const size_t dst_size)
{
	if (!segment || !dst) {
		return -EINVAL;
	}

	if (segment->selection < 0 ||
	    segment->selection >= segment->num_candidates) {
		/* no candidate selected - return input */
		return segment_get_input(segment, dst, dst_size);
	}

	return snprintf(dst, dst_size, "%s", segment->candidates[segment->selection]);
}

int segment_select_candidate(segment_t *segment, const int selection)
{
	if (!segment) {
		return -EINVAL;
	}

	if (selection < 0 || selection >= segment->num_candidates) {
		return -EBADSLT;
	}

	segment->selection = selection;
	return 0;
}

int segment_set_candidates(segment_t *segment, char **candidates)
{
	char *old_selection;
	int new_selection;
	char **new_candidates;
	int num_candidates;

	new_selection = -1;
	old_selection = NULL;

	if (segment->selection < 0 || segment->selection >= segment->num_candidates) {
		old_selection = segment->candidates[segment->selection];
	}

	for (num_candidates = 0; candidates[num_candidates]; num_candidates++) {
		/*
		 * While we're checking the size of the array, check also if the old
		 * selection is present in the new candidate array.
		 */
		if (old_selection && old_selection == candidates[num_candidates]) {
			new_selection = num_candidates;
		}
	}

	if (!(new_candidates = calloc(num_candidates + 1, sizeof(*new_candidates)))) {
		return -ENOMEM;
	}
	memmove(new_candidates, candidates, num_candidates * sizeof(*candidates));

	free(segment->candidates);
	segment->candidates = new_candidates;
	segment->num_candidates = num_candidates;
	segment->selection = new_selection;

	return num_candidates;
}

int segment_get_candidates(segment_t *segment, char ***candidates)
{
	char **new_candidates;

	if (!segment || !candidates) {
		return -EINVAL;
	}

	if (!segment->num_candidates) {
		return -ENOENT;
	}

	if (!(new_candidates = calloc(segment->num_candidates + 1, sizeof(*new_candidates)))) {
		return -ENOMEM;
	}

	memmove(new_candidates, segment->candidates,
	        segment->num_candidates * sizeof(*new_candidates));

	return segment->num_candidates;
}

int segment_move_candidate(segment_t *segment, const int dir)
{
	if (!segment) {
		return -EINVAL;
	}

	segment->selection = (segment->selection + dir) % segment->num_candidates;

	while (segment->selection < 0) {
		segment->selection += segment->num_candidates;
	}

	return 0;
}
