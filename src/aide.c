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
#include "conjugation.h"
#include "dict.h"
#include "dictparser.h"
#include "parray.h"
#include "japanese.h"
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static dict_t **_dicts = NULL;

static int aide_unconjugate(const char_t *conjugated, conjugation_t ***conjugations);

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

static int _cmp_suggestion(const suggestion_t *a,
                           const suggestion_t *b)
{
	return b->priority - a->priority;
}

static int _candidate_to_suggestion(suggestion_t **suggestion, dict_candidate_t *candidate,
                                    conjugation_t *conjugation)
{
	if (!suggestion || !candidate || !conjugation) {
		return -EINVAL;
	}

	suggestion_t *suggest;
	char *conjugated;
	int err;

	/* Do not conjugate if the types don't match */
	if (conjugation->type && candidate->type != conjugation->type) {
		return -EDOM;
	}

	if ((err = conjugation_conjugate(&conjugated, candidate->value,
	                                 conjugation)) < 0) {
		return err;
	}

	err = suggestion_new(&suggest, conjugated, NULL);

	if (!err) {
		suggest->priority = candidate->priority;
		suggest->data = candidate;
		*suggestion = suggest;
	}

	free(conjugated);
	return err;
}

struct _entries_to_suggestions_args {
	parray_t *parray;
	conjugation_t *conjugation;
};

static int _entries_to_suggestions(dict_entry_t *entry, struct _entries_to_suggestions_args *args)
{
	int err;
	int i;

	if (!entry || !args || !args->parray || !args->conjugation) {
		return -EINVAL;
	}

	for (i = 0; i < entry->num_candidates; i++) {
		dict_candidate_t *candidate;
		suggestion_t *suggestion;

		candidate = entry->candidates[i];

		if ((err = _candidate_to_suggestion(&suggestion, candidate, args->conjugation)) < 0) {
			continue;
		}

		if ((err = parray_insert(args->parray, (const void**)&suggestion, 1)) < 0) {
			suggestion_free(&suggestion);
			return err;
		}
	}

	return 0;
}

static int _lookup_conjugation(conjugation_t *conjugation, parray_t *parray)
{
	struct _entries_to_suggestions_args args;
	dict_lookup_mode_t lkup_mode;
	dict_entry_t **entries;
	int err;
	int i;

	if (!conjugation || !parray) {
		return -EINVAL;
	}

	args.conjugation = conjugation;
	args.parray = parray;
	entries = NULL;
	err = 0;

	/* Get only exact matches if we are going to conjugate */
	lkup_mode = conjugation->conjugation_len > 0 ?
		DICT_LOOKUP_MODE_EXACT : DICT_LOOKUP_MODE_PREDICT;

	/* look up the conjugation in each of our dictionaries and collect results in `entries' */
	for (i = 0; _dicts[i]; i++) {
		dict_lookup(_dicts[i], conjugation->dict_form, lkup_mode, &entries);
	}

	/*
	 * Each entry (type dict_entry_t) contains multiple candidates (type dict_candidate_t), which
	 * we have to convert into suggestions (type suggestion_t). Suggestions will retain a reference
	 * to the dict candidate that they were created from (for collecting statistics on frequently
	 * used words, to improve suggestion quality), as well as the priority.
	 */
	err = array_foreach((void***)&entries, (int(*)(void*, void*))_entries_to_suggestions, &args);

	/* elements in `entries' are shared, so don't free them */
	free(entries);

	/* don't stop the calling loop if this call didn't yield any results */
	if (err == -ENOENT) {
		err = 0;
	}

	return err;
}

int aide_suggest(const char_t *key, suggestion_t ***suggestions)
{
	parray_t *parray;
	conjugation_t **conjugations;
	int err;

	parray = NULL;
	conjugations = NULL;

	/* parray will be used to order suggestions presented to the user */
	if ((err = parray_new(&parray, (int(*)(const void*, const void*))_cmp_suggestion)) < 0) {
		return err;
	}

	/*
	 * The input may be a conjugated form of a word, so we need to determine the
	 * dictionary form. The `aide_unconjugate()` method always succeeds (except
	 * if we run out of memory or pass invalid inputs): If the input is not a
	 * recognized conjugation, it returns a null-conjugation, i.e. a conjugation
	 * that contains the raw input and can be used as if it was a conjugation.
	 */
	if ((err = aide_unconjugate(key, &conjugations)) >= 0) {
		/*
		 * Because we cannot decide which one is the correct conjugation (for example,
		 * いった could be a conjugation of いく, いう, etc) we query all conjugations
		 * and return all results to the caller. Ultimately, the user will have to make
		 * the decision.
		 */
		array_foreach((void***)&conjugations, (int(*)(void*, void*))_lookup_conjugation, parray);

		err = parray_get_items(parray, (const void***)suggestions);
	}

	array_free((void***)&conjugations, (int(*)(void**))conjugation_free);
	parray_free(&parray);

	return err;
}

static int aide_unconjugate(const char_t *conjugated, conjugation_t ***conjugations)
{
	conjugation_t *conjugation;
	int err;

	japanese_unconjugate(conjugated, conjugations);

	/* Finally, add a null conjugation */
	if (!(err = conjugation_new(&conjugation, conjugated, 0, NULL, 0))) {
		if ((err = array_add((void***)conjugations, conjugation)) < 0) {
			conjugation_free(&conjugation);
		}
	}

	return err;
}
