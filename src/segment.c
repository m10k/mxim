/*
 * segment.c - This file is part of mxim
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

#include "aide.h"
#include "array.h"
#include "char.h"
#include "segment.h"
#include "suggestion.h"
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
	free((*segment)->candidates);
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

	/* Try to combine with the previous character */
	if (insert_pos > 0) {
		char_t combined;

		combined = char_combine(segment->input[insert_pos - 1], chr);

		if (combined != CHAR_INVALID) {
			segment->input[insert_pos - 1] = combined;
			return 0;
		}
	}

	tail_len = segment->len - insert_pos;
	memmove(segment->input + insert_pos + 1,
	        segment->input + insert_pos, tail_len);
	segment->input[insert_pos] = chr;
	segment->len++;

	return 1;
}

int segment_clear(segment_t *segment)
{
	if (!segment) {
		return -EINVAL;
	}

	memset(segment->input, 0, segment->size * sizeof(*segment->input));
	segment->len = 0;

	free(segment->candidates);
	segment->candidates = NULL;
	segment->num_candidates = 0;
	segment->selection = -1;

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
	static const char cursor[] = "<span foreground=\"red\">_</span>";
	static const char lbracket[] = "<span foreground=\"#3ae926\">[</span>";
	static const char rbracket[] = "<span foreground=\"#3ae926\">]</span>";

	string_t *input;
	string_t *escape;
	int err;
	int i;

	escape = NULL;

	if ((err = string_new(&input)) < 0) {
		return err;
	}

	if (selected &&
	    (err = string_append_utf8(input, lbracket, strlen(lbracket))) < 0) {
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
		const char *text;
		size_t text_len;

		if (segment->candidates && segment->selection >= 0) {
			text = suggestion_get_display(segment->candidates[segment->selection]);
			text_len = strlen(text);
		} else {
			text = NULL;
		}

		if (string_new(&escape) < 0 ||
		    (err = text ? string_append_utf8(escape, text, text_len) :
		     string_append_char(escape, segment->input, segment->len)) < 0 ||
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

			const char *text;
			size_t text_len;

			if ((err = string_append_utf8(input, "|", 1)) < 0) {
				goto cleanup;
			}

			if (i == segment->selection &&
			    (err = string_append_utf8(input, _selection_header,
			                              sizeof(_selection_header))) < 0) {
				goto cleanup;
			}

			text = suggestion_get_display(segment->candidates[i]);
			text_len = strlen(text);

			if ((err = string_new(&escape)) < 0 ||
			    (err = string_append_utf8(escape, text, text_len)) < 0 ||
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

		if ((err = string_append_utf8(input, rbracket, strlen(rbracket))) < 0) {
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
/*	segment->candidates[segment->selection]->candidate->priority++; */
	return snprintf(dst, dst_size, "%s", suggestion_get_value(segment->candidates[segment->selection]));
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

struct _count_and_cmp_args {
	suggestion_t *old_selection;
	int new_selection;
	int num_candidates;
};

int _count_and_cmp_candidates(suggestion_t *candidate, struct _count_and_cmp_args *args)
{
	if (args->old_selection && suggestion_cmp(candidate, args->old_selection) == 0) {
		args->new_selection = args->num_candidates;
	}

	args->num_candidates++;
	return 0;
}

int segment_set_candidates(segment_t *segment, suggestion_t **candidates)
{
	struct _count_and_cmp_args args;

	args.old_selection = NULL;
	args.new_selection = -1;
	args.num_candidates = 0;

	if (segment->selection >= 0 && segment->selection < segment->num_candidates) {
		args.old_selection = segment->candidates[segment->selection];
	}

	array_foreach((void***)&candidates, (int(*)(void*, void*))_count_and_cmp_candidates, &args);
	array_free((void***)&segment->candidates, (int(*)(void**))suggestion_free);
	segment->candidates = candidates;
	segment->num_candidates = args.num_candidates;
	segment->selection = args.new_selection;

	return args.num_candidates;
}

int segment_get_candidates(segment_t *segment, suggestion_t ***candidates)
{
	if (!segment || !candidates) {
		return -EINVAL;
	}

	if (!segment->candidates || !segment->num_candidates) {
		return -ENOENT;
	}

	*candidates = segment->candidates;
	return segment->num_candidates;
}

int segment_move_candidate(segment_t *segment, const int dir)
{
	if (!segment) {
		return -EINVAL;
	}

	if (segment->num_candidates == 0) {
		return -ENOENT;
	}

	segment->selection = (segment->selection + dir) % segment->num_candidates;

	while (segment->selection < 0) {
		segment->selection += segment->num_candidates;
	}

	return 0;
}

int segment_update_candidates(segment_t *segment)
{
	suggestion_t **candidates;

	if (!segment) {
		return -EINVAL;
	}

	candidates = NULL;

	if (segment->len > 0) {
		aide_suggest(segment->input, &candidates);
	}
	segment_set_candidates(segment, candidates);

	return 0;
}
