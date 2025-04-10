/*
 * aide.c - This file is part of mxim
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

#define _GNU_SOURCE
#include "aide.h"
#include "array.h"
#include "dict.h"
#include "dictparser.h"
#include "parray.h"
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static dict_t **_dicts = NULL;

static int _get_dict_path(char **output)
{
	const char *home;
	char *path;

	if (!(home = getenv("HOME"))) {
		return -ENOENT;
	}

	if (asprintf(&path, "%s/.config/mxim/dicts", home) < 0) {
		return -errno;
	}

	*output = path;
	return 0;
}

static int _list_dicts_in_path(const char *path, char ***output)
{
	DIR *dict_dir;
	struct dirent *entry;
	char **dicts;
	int err;

	err = 0;
	dicts = NULL;

	if (!(dict_dir = opendir(path))) {
		return -errno;
	}

	while (!err && (entry = readdir(dict_dir))) {
		char *dict_name;
		int len;

		/* Ignore hidden files */
		if (entry->d_name[0] == '.') {
			continue;
		}

		/* Ignore files that don't end in ".mxim" */
		if ((len = strlen(entry->d_name)) < 5 ||
		    strcmp(entry->d_name + len - 5, ".mxim") != 0) {
			continue;
		}

		if (asprintf(&dict_name, "%s/%s", path, entry->d_name) < 0) {
			err = -errno;
			break;
		}

		if ((err = array_add((void***)&dicts, dict_name)) < 0) {
			free(dict_name);
		} else {
			err = 0;
		}
	}

	closedir(dict_dir);

	if (err) {
		array_free((void***)&dicts, ARRAY_GENERIC_FREE);
	} else {
		*output = dicts;
	}

	return err;
}

static int _get_dict_paths(char ***dicts)
{
	char *dict_path;
	int err;

	if (_get_dict_path(&dict_path) < 0) {
		return -ENOENT;
	}

	err = _list_dicts_in_path(dict_path, dicts);
	free(dict_path);

	return err;
}

int _open_dict(dict_t **dict, const char *path)
{
	dict_parser_t *parser;
	int err;

#if MXIM_DEBUG
	fprintf(stderr, "Opening dict: %s\n", path);
#endif /* MXIM_DEBUG */

	if ((err = dict_parser_new(&parser, path)) < 0) {
		return err;
	}

	err = dict_parser_get_dict(parser, dict);
	dict_parser_free(&parser);

	return err;
}

int aide_init(void)
{
	char **dict_paths;
	int err;
	int i;

	if ((err = _get_dict_paths(&dict_paths)) < 0) {
		return err;
	}

	for (i = 0; dict_paths[i]; i++) {
		dict_t *dict;

		if ((err = _open_dict(&dict, dict_paths[i])) < 0) {
			fprintf(stderr, "Could not open dict `%s': %s\n", dict_paths[i], strerror(-err));
			continue;
		}

		if ((err = array_add((void***)&_dicts, dict)) < 0) {
			dict_free(&dict);
		}
	}

	array_free((void***)&dict_paths, ARRAY_GENERIC_FREE);
	return 0;
}

static int _cmp_candidate_priority(const dict_candidate_t *a,
                                   const dict_candidate_t *b)
{
	return b->priority - a->priority;
}

static int _make_suggestion(dict_candidate_t *candidate, suggestion_t ***suggestions)
{
	suggestion_t *suggestion;

	if (suggestion_new(&suggestion, candidate->value, NULL) < 0) {
		return -ENOMEM;
	}

	if (array_add((void***)suggestions, suggestion) < 0) {
		suggestion_free(&suggestion);
		return -ENOMEM;
	}

	return 0;
}

int aide_suggest(const char_t *key, suggestion_t ***suggestions)
{
	dict_candidate_t **candidates;
	parray_t *parray;
	dict_entry_t **entries;
	int err;
	int i;

	parray = NULL;
	entries = NULL;
	candidates = NULL;

	if ((err = parray_new(&parray, (int(*)(const void*, const void*))_cmp_candidate_priority)) < 0) {
		goto cleanup;
	}

	for (i = 0; _dicts[i]; i++) {
		int j;

		if ((err = dict_lookup(_dicts[i], key, DICT_LOOKUP_MODE_PREDICT, &entries)) < 0) {
			continue;
		}

		for (j = 0; entries[j] && j < 10; j++) {
			parray_insert(parray, (const void**)entries[j]->candidates,
			              entries[j]->num_candidates);
		}
	}

	if (!(err = parray_get_items(parray, (const void***)&candidates))) {
		err = array_foreach((void***)&candidates,
		                    (int(*)(void*, void*))_make_suggestion,
		                    suggestions);
	}

cleanup:
	free(candidates);
	parray_free(&parray);
	/* only free the array, not the items that it points to */
	free(entries);

	return err;
}
