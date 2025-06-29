#include "array.h"
#include "char.h"
#include "conjugation.h"
#include "trie.h"
#include "japanese.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct conjugation_data {
	/*
	 * To convert an input to dictionary form, trim `conj.len` bytes and
	 * add `dict.suffix`. To convert the dictionary form back to the input,
	 * trim `dict.len` bytes and add `conj.suffix`. If `irregular.dict` is
	 * not NULL, use that as the dictionary form.
	 */
	struct {
		char_t *suffix;
		int len;
	} conj;

	struct {
		char_t *suffix;
		int len;
	} dict;

	struct {
		char_t *dict;
		int len;
	} irregular;

	int type;
};

struct conjugation_table_entry {
	char *conj;  /* The index for the trie, if `match` is NULL */
	char *dict;
	int type;
	char *match; /* The index for the trie */
	char *irregular;
};

int cdata_free(struct conjugation_data **cdata)
{
	if (!cdata) {
		return -EINVAL;
	}

	free((*cdata)->conj.suffix);
	(*cdata)->conj.suffix = NULL;
	free((*cdata)->dict.suffix);
	(*cdata)->dict.suffix = NULL;
	free((*cdata)->irregular.dict);
	(*cdata)->irregular.dict = NULL;
	free(*cdata);
	*cdata = NULL;

	return 0;
}

static int ctable_get_key(const struct conjugation_table_entry *ctable, char_t **key)
{
	if (!ctable || !key) {
		return -EINVAL;
	}

	if (ctable->match) {
		return char_from_utf8(ctable->match, strlen(ctable->match), key);
	}

	return char_from_utf8(ctable->conj, strlen(ctable->conj), key);
}

static int cdata_from_ctable_entry(struct conjugation_data **cdata,
                                   const struct conjugation_table_entry *entry)
{
	struct conjugation_data *data;
	char *dict_suffix;
	int err;

	if (!entry) {
		return -EINVAL;
	}

	if (!entry->conj) {
		return -EBADR;
	}

	if (!(data = calloc(1, sizeof(*data)))) {
		return -ENOMEM;
	}

	data->type = entry->type;

	if ((err = char_from_utf8(entry->conj, strlen(entry->conj), &data->conj.suffix)) < 0) {
		goto cleanup;
	}
	data->conj.len = char_len(data->conj.suffix);

	if (entry->irregular) {
		err = char_from_utf8(entry->irregular, strlen(entry->irregular),
		                     &data->irregular.dict);
		if (err < 0) {
			goto cleanup;
		}
		data->irregular.len = char_len(data->irregular.dict);
	}

	dict_suffix = entry->dict ? entry->dict : "";
	if ((err = char_from_utf8(dict_suffix, strlen(dict_suffix), &data->dict.suffix)) < 0) {
		goto cleanup;
	}
	data->dict.len = char_len(data->dict.suffix);

cleanup:
	if (err < 0) {
		cdata_free(&data);
	} else {
		*cdata = data;
	}

	return err;
}

static int trie_insert_ctable_entry(trie_t *trie, const struct conjugation_table_entry *entry)
{
	struct conjugation_data *cdata;
	char_t *key;
	int err;

	cdata = NULL;
	key = NULL;

	if ((err = ctable_get_key(entry, &key)) < 0) {
		return err;
	}

	if ((err = cdata_from_ctable_entry(&cdata, entry)) >= 0) {
		if ((err = trie_insert_reverse(trie, key, char_len(key), (const void**)&cdata, 1)) < 0) {
			cdata_free(&cdata);
		}
	}

	free(key);
	return err;
}

static const struct conjugation_table_entry _conjugation_table[] = {
	{ "い",           "い", JA_TYPE_ADJ_I },
	{ "くない",       "い", JA_TYPE_ADJ_I },
	{ "かった",       "い", JA_TYPE_ADJ_I },
	{ "くなかった",   "い", JA_TYPE_ADJ_I },
	{ "かろう",       "い", JA_TYPE_ADJ_I },
	{ "くなかろう",   "い", JA_TYPE_ADJ_I },
	{ "ければ",       "い", JA_TYPE_ADJ_I },
	{ "くなければ",   "い", JA_TYPE_ADJ_I },
	{ "かったら",     "い", JA_TYPE_ADJ_I },
	{ "くなかったら", "い", JA_TYPE_ADJ_I },
	{ "く",           "い", JA_TYPE_ADJ_I },
	{ "くなく",       "い", JA_TYPE_ADJ_I },
	{ "くて",         "い", JA_TYPE_ADJ_I },
	{ "くなくて",     "い", JA_TYPE_ADJ_I },
	{ "すぎる",       "い", JA_TYPE_ADJ_I },
	{ "そう",         "い", JA_TYPE_ADJ_I },

	{ "い",           "い", JA_TYPE_ADJ_YOI, "よい" },
	{ "くない",       "い", JA_TYPE_ADJ_YOI, "よくない" },
	{ "かった",       "い", JA_TYPE_ADJ_YOI, "よかった" },
	{ "くなかった",   "い", JA_TYPE_ADJ_YOI, "よくなかった" },
	{ "かろう",       "い", JA_TYPE_ADJ_YOI, "よかろう" },
	{ "くなかろう",   "い", JA_TYPE_ADJ_YOI, "よくなかろう" },
	{ "ければ",       "い", JA_TYPE_ADJ_YOI, "よければ" },
	{ "くなければ",   "い", JA_TYPE_ADJ_YOI, "よくなければ" },
	{ "かったら",     "い", JA_TYPE_ADJ_YOI, "よかったら" },
	{ "くなかったら", "い", JA_TYPE_ADJ_YOI, "よくなかったら" },
	{ "く",           "い", JA_TYPE_ADJ_YOI, "よく" },
	{ "くなく",       "い", JA_TYPE_ADJ_YOI, "よくなく" },
	{ "くて",         "い", JA_TYPE_ADJ_YOI, "よくて" },
	{ "くなくて",     "い", JA_TYPE_ADJ_YOI, "よくなくて" },
	{ "すぎる",       "い", JA_TYPE_ADJ_YOI, "よさすぎる" },
	{ "そう",         "い", JA_TYPE_ADJ_YOI, "よさそう" },

	/* This entry might not be necessary */
	{ "い",           "い", JA_TYPE_ADJ_YOI, "いい" },

	{ "し",               "", JA_TYPE_VERB_SURU },
	{ "する",             "", JA_TYPE_VERB_SURU },
	{ "します",           "", JA_TYPE_VERB_SURU },
	{ "しない",           "", JA_TYPE_VERB_SURU },
	{ "しません",         "", JA_TYPE_VERB_SURU },
	{ "すれば",           "", JA_TYPE_VERB_SURU },
	{ "しよう",           "", JA_TYPE_VERB_SURU },
	{ "しましょう",       "", JA_TYPE_VERB_SURU },
	{ "するまい",         "", JA_TYPE_VERB_SURU },
	{ "しますまい",       "", JA_TYPE_VERB_SURU },
	{ "しろ",             "", JA_TYPE_VERB_SURU },
	{ "しなさい",         "", JA_TYPE_VERB_SURU },
	{ "するな",           "", JA_TYPE_VERB_SURU },
	{ "して",             "", JA_TYPE_VERB_SURU },
	{ "しまして",         "", JA_TYPE_VERB_SURU },
	{ "しないで",         "", JA_TYPE_VERB_SURU },
	{ "しなく",           "", JA_TYPE_VERB_SURU },
	{ "しなくて",         "", JA_TYPE_VERB_SURU },
	{ "しませんでして",   "", JA_TYPE_VERB_SURU },
	{ "した",             "", JA_TYPE_VERB_SURU },
	{ "しました",         "", JA_TYPE_VERB_SURU },
	{ "したら",           "", JA_TYPE_VERB_SURU },
	{ "しましたら",       "", JA_TYPE_VERB_SURU },
	{ "しなかった",       "", JA_TYPE_VERB_SURU },
	{ "しませんでした",   "", JA_TYPE_VERB_SURU },
	{ "しなかったら",     "", JA_TYPE_VERB_SURU },
	{ "しませんでしたら", "", JA_TYPE_VERB_SURU },
	{ "しなければ",       "", JA_TYPE_VERB_SURU },
	{ "せず",             "", JA_TYPE_VERB_SURU },
	{ "すべき",           "", JA_TYPE_VERB_SURU },
	{ "すべからず",       "", JA_TYPE_VERB_SURU },

	{ "できる",           "", JA_TYPE_VERB_SURU },
	{ "できます",         "", JA_TYPE_VERB_SURU },
	{ "できない",         "", JA_TYPE_VERB_SURU },
	{ "できません",       "", JA_TYPE_VERB_SURU },
	{ "できなく",         "", JA_TYPE_VERB_SURU },
	{ "できなくて",       "", JA_TYPE_VERB_SURU },
	{ "できて",           "", JA_TYPE_VERB_SURU },
	{ "できまして",       "", JA_TYPE_VERB_SURU },
	{ "できず",           "", JA_TYPE_VERB_SURU },
	{ "できた",           "", JA_TYPE_VERB_SURU },
	{ "できました",       "", JA_TYPE_VERB_SURU },
	{ "できなかった",     "", JA_TYPE_VERB_SURU },
	{ "できなかったら",   "", JA_TYPE_VERB_SURU },
	{ "できなければ",     "", JA_TYPE_VERB_SURU },
	{ "できませんでした", "", JA_TYPE_VERB_SURU },
	{ "できませんでして", "", JA_TYPE_VERB_SURU },

	{ "される",                 "", JA_TYPE_VERB_SURU },
	{ "されます",               "", JA_TYPE_VERB_SURU },
	{ "されない",               "", JA_TYPE_VERB_SURU },
	{ "されません",             "", JA_TYPE_VERB_SURU },
	{ "されなく",               "", JA_TYPE_VERB_SURU },
	{ "されなくて",             "", JA_TYPE_VERB_SURU },
	{ "されないで",             "", JA_TYPE_VERB_SURU },
	{ "されて",                 "", JA_TYPE_VERB_SURU },
	{ "されまして",             "", JA_TYPE_VERB_SURU },
	{ "されず",                 "", JA_TYPE_VERB_SURU },
	{ "された",                 "", JA_TYPE_VERB_SURU },
	{ "されました",             "", JA_TYPE_VERB_SURU },
	{ "されなかった",           "", JA_TYPE_VERB_SURU },
	{ "されませんでした",       "", JA_TYPE_VERB_SURU },
	{ "されなかったら",         "", JA_TYPE_VERB_SURU },
	{ "されませんでしたら",     "", JA_TYPE_VERB_SURU },
	{ "されなければ",           "", JA_TYPE_VERB_SURU },
	{ "されたら",               "", JA_TYPE_VERB_SURU },
	{ "されましたら",           "", JA_TYPE_VERB_SURU },
	{ "されれば",               "", JA_TYPE_VERB_SURU },

	{ "させる",                 "", JA_TYPE_VERB_SURU },
	{ "させます",               "", JA_TYPE_VERB_SURU },
	{ "させない",               "", JA_TYPE_VERB_SURU },
	{ "させません",             "", JA_TYPE_VERB_SURU },
	{ "させなく",               "", JA_TYPE_VERB_SURU },
	{ "させなくて",             "", JA_TYPE_VERB_SURU },
	{ "させないで",             "", JA_TYPE_VERB_SURU },
	{ "させて",                 "", JA_TYPE_VERB_SURU },
	{ "させまして",             "", JA_TYPE_VERB_SURU },
	{ "させず",                 "", JA_TYPE_VERB_SURU },
	{ "させた",                 "", JA_TYPE_VERB_SURU },
	{ "させました",             "", JA_TYPE_VERB_SURU },
	{ "させなかった",           "", JA_TYPE_VERB_SURU },
	{ "させませんでした",       "", JA_TYPE_VERB_SURU },
	{ "させなかったら",         "", JA_TYPE_VERB_SURU },
	{ "させませんでしたら",     "", JA_TYPE_VERB_SURU },
	{ "させなければ",           "", JA_TYPE_VERB_SURU },
	{ "させたら",               "", JA_TYPE_VERB_SURU },
	{ "させましたら",           "", JA_TYPE_VERB_SURU },
	{ "させれば",               "", JA_TYPE_VERB_SURU },

	{ "させられる",             "", JA_TYPE_VERB_SURU },
	{ "させられます",           "", JA_TYPE_VERB_SURU },
	{ "させられない",           "", JA_TYPE_VERB_SURU },
	{ "させられません",         "", JA_TYPE_VERB_SURU },
	{ "させられなく",           "", JA_TYPE_VERB_SURU },
	{ "させられなくて",         "", JA_TYPE_VERB_SURU },
	{ "させられないで",         "", JA_TYPE_VERB_SURU },
	{ "させられて",             "", JA_TYPE_VERB_SURU },
	{ "させられまして",         "", JA_TYPE_VERB_SURU },
	{ "させられず",             "", JA_TYPE_VERB_SURU },
	{ "させられた",             "", JA_TYPE_VERB_SURU },
	{ "させられました",         "", JA_TYPE_VERB_SURU },
	{ "させられなかった",       "", JA_TYPE_VERB_SURU },
	{ "させられませんでした",   "", JA_TYPE_VERB_SURU },
	{ "させられなかったら",     "", JA_TYPE_VERB_SURU },
	{ "させられませんでしたら", "", JA_TYPE_VERB_SURU },
	{ "させられなければ",       "", JA_TYPE_VERB_SURU },
	{ "させられたら",           "", JA_TYPE_VERB_SURU },
	{ "させられましたら",       "", JA_TYPE_VERB_SURU },
	{ "させられれば",           "", JA_TYPE_VERB_SURU },

	{ NULL, NULL, 0, NULL }
};

static int make_conjugation(const char_t *key, struct conjugation_data *cdata, conjugation_t **conj)
{
	char_t *dict_form;
	int err;

	/*
	 * To get the dictionary form, we have to trim `cdata->conj.len` bytes from
	 * the end of `key` and append `cdata->dict.suffix` to it.
	 */
	if ((err = char_concat(&dict_form, key, char_len(key) - cdata->conj.len,
	                       cdata->dict.suffix, cdata->dict.len)) >= 0) {
		char *utf8;

		char_to_utf8_dyn(dict_form, char_len(dict_form), &utf8);
		fprintf(stderr, " : %s (%d)\n", utf8, cdata->dict.len);
		free(utf8);

		err = conjugation_new(conj, dict_form, cdata->dict.len,
		                      cdata->conj.suffix, cdata->conj.len);
		if (!err) {
			(*conj)->type = cdata->type;
		}
		free(dict_form);
	}

	return err;
}

static int _ctrie_lookup(trie_t *ctree, const char_t *key, conjugation_t ***results)
{
	struct conjugation_data **cdata;
	int num_values;
	int err;
	int i;

	cdata = NULL;

	{
		char *utf8;

		char_to_utf8_dyn(key, char_len(key), &utf8);
		fprintf(stderr, "Reverse lookup: \"%s\"\n", utf8);
		fprintf(stderr, "trie_get_values_reverse(%p, %p \"%s\", %d, TRIE_LOOKUP_COLLECT, %p)\n",
		        (void*)ctree, (void*)key, utf8, char_len(key), (void*)&cdata);
		free(utf8);
	}

	num_values = trie_get_values_reverse(ctree, key, char_len(key), TRIE_LOOKUP_COLLECT,
	                                     (void***)&cdata);
	if (num_values < 0) {
		fprintf(stderr, "trie_get_values_reverse: %s\n", strerror(-num_values));
		return num_values;
	} else {
		fprintf(stderr, "trie_get_values_reverse: %d\n", num_values);
	}

	for (i = 0; i < num_values; i++) {
		conjugation_t *conj;

		conj = NULL;

		fprintf(stderr, "cdata[%d] = %p\n", i, (void*)cdata[i]);

		if ((err = make_conjugation(key, cdata[i], &conj)) < 0) {
			break;
		}

		if ((err = array_add((void***)results, conj)) < 0) {
			conjugation_free(&conj);
			break;
		}
	}

	array_free((void***)&cdata, NULL);

	fprintf(stderr, " ~ %d results\n", num_values);

	return num_values;
}

static int _conjugation_trie_init(trie_t **ctrie)
{
	int err;
	int i;

	if ((err = trie_new(ctrie)) < 0) {
		return err;
	}

	for (i = 0; _conjugation_table[i].conj; i++) {
		fprintf(stderr, "Inserting %s\n", _conjugation_table[i].conj);
		if ((err = trie_insert_ctable_entry(*ctrie, &_conjugation_table[i])) < 0) {
			return err;
		}
	}

	return 0;
}

int japanese_unconjugate(const char_t *kana, conjugation_t ***results)
{
	static trie_t *ctrie = NULL;
	int err;

	if (!ctrie) {
		if ((err = _conjugation_trie_init(&ctrie)) < 0) {
			return err;
		}
	}

	return _ctrie_lookup(ctrie, kana, results);
}
