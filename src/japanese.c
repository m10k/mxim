#include "array.h"
#include "char.h"
#include "conjugation.h"
#include "japanese.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define A 0
#define I 1
#define U 2
#define E 3
#define O 4

static const char_t _a_retsu[] = {
	CHAR_JA_WA, CHAR_JA_NA, CHAR_JA_TA, CHAR_JA_SA, CHAR_JA_KA,
	CHAR_JA_GA, CHAR_JA_RA, CHAR_JA_HA, CHAR_JA_BA, CHAR_JA_MA, CHAR_INVALID
};
static const char_t _i_retsu[] = {
	CHAR_JA_I, CHAR_JA_NI, CHAR_JA_TI, CHAR_JA_SI, CHAR_JA_KI,
	CHAR_JA_GI, CHAR_JA_RI, CHAR_JA_HI, CHAR_JA_BI, CHAR_JA_MI, CHAR_INVALID
};
static const char_t _u_retsu[] = {
	CHAR_JA_U, CHAR_JA_NU, CHAR_JA_TU, CHAR_JA_SU, CHAR_JA_KU,
	CHAR_JA_GU, CHAR_JA_RU, CHAR_JA_HU, CHAR_JA_BU, CHAR_JA_MU, CHAR_INVALID
};
static const char_t _e_retsu[] = {
	CHAR_JA_E, CHAR_JA_NE, CHAR_JA_TE, CHAR_JA_SE, CHAR_JA_KE,
	CHAR_JA_GE, CHAR_JA_RE, CHAR_JA_HE, CHAR_JA_BE, CHAR_JA_ME, CHAR_INVALID
};
static const char_t _o_retsu[] = {
	CHAR_JA_O, CHAR_JA_NO, CHAR_JA_TO, CHAR_JA_SO, CHAR_JA_KO,
	CHAR_JA_GO, CHAR_JA_RO, CHAR_JA_HO, CHAR_JA_BO, CHAR_JA_MO, CHAR_INVALID
};

static const char_t *_retsu[] = {
	[A] = _a_retsu,
	[I] = _i_retsu,
	[U] = _u_retsu,
	[E] = _e_retsu,
	[O] = _o_retsu
};

static int _kana_in_retsu(const char_t kana, const int retsu)
{
	int result;
	int idx;

	if (retsu < A || retsu > O) {
		return -EINVAL;
	}

	result = -ENOENT;

	for (idx = 0; _retsu[retsu][idx] != CHAR_INVALID; idx++) {
		if (_retsu[retsu][idx] == kana) {
			result = idx;
			break;
		}
	}

	return result;
}

static int _kana_index(const char_t chr)
{
	int result;
	int retsu;

	result = -ENOENT;

	for (retsu = A; retsu <= O; retsu++) {
		int idx;

		if ((idx = _kana_in_retsu(chr, retsu)) >= 0) {
			result = idx;
			break;
		}
	}

	return result;
}

static char_t _convert_kana(const char_t chr, const int retsu)
{
	int idx;

	assert(retsu >= A);
	assert(retsu <= O);

	if ((idx = _kana_index(chr)) < 0) {
		return CHAR_INVALID;
	}

	return _retsu[retsu][idx];
}

static const char_t _suffix_karou[] = {
	CHAR_JA_KA, CHAR_JA_RO, CHAR_JA_U, CHAR_INVALID
};
static const char_t _suffix_i[] = {
	CHAR_JA_I, CHAR_INVALID
};
static const char_t _suffix_katta[] = {
	CHAR_JA_KA, CHAR_JA_tu, CHAR_JA_TA, CHAR_INVALID
};
static const char_t _suffix_kereba[] = {
	CHAR_JA_KE, CHAR_JA_RE, CHAR_JA_BA, CHAR_INVALID
};
static const char_t _suffix_kattara[] = {
	CHAR_JA_KA, CHAR_JA_tu, CHAR_JA_TA, CHAR_JA_RA, CHAR_INVALID
};
static const char_t _suffix_ku[] = {
	CHAR_JA_KU, CHAR_INVALID
};
static const char_t _suffix_sou[] = {
	CHAR_JA_SO, CHAR_JA_U, CHAR_INVALID
};

static const char_t _suffix_ii[] = {
	CHAR_JA_I, CHAR_JA_I, CHAR_INVALID
};
static const char_t _suffix_yokarou[] = {
	CHAR_JA_YO, CHAR_JA_KA, CHAR_JA_RO, CHAR_JA_U, CHAR_INVALID
};
static const char_t _suffix_yoi[] = {
	CHAR_JA_YO, CHAR_JA_I, CHAR_INVALID
};
static const char_t _suffix_yokatta[] = {
	CHAR_JA_YO, CHAR_JA_KA, CHAR_JA_tu, CHAR_JA_TA, CHAR_INVALID
};
static const char_t _suffix_yokereba[] = {
	CHAR_JA_YO, CHAR_JA_KE, CHAR_JA_RE, CHAR_JA_BA, CHAR_INVALID
};
static const char_t _suffix_yokattara[] = {
	CHAR_JA_YO, CHAR_JA_KA, CHAR_JA_tu, CHAR_JA_TA, CHAR_JA_RA, CHAR_INVALID
};
static const char_t _suffix_yoku[] = {
	CHAR_JA_YO, CHAR_JA_KU, CHAR_INVALID
};
static const char_t _suffix_yosasou[] = {
	CHAR_JA_YO, CHAR_JA_SA, CHAR_JA_SO, CHAR_JA_U, CHAR_INVALID
};

static const char_t *_i_adj_suffixes[] = {
	_suffix_karou, _suffix_i, _suffix_katta, _suffix_kereba, _suffix_kattara, _suffix_ku,
	_suffix_sou, NULL
};
static const char_t *_yoi_adj_suffixes[] = {
	_suffix_yokarou, _suffix_yoi, _suffix_yokatta, _suffix_yokereba, _suffix_yokattara,
	_suffix_yoku, _suffix_yosasou, NULL
};
static const char_t *_ii_adj_suffixes[] = {
	_suffix_yokarou, _suffix_ii, _suffix_yokatta, _suffix_yokereba, _suffix_yokattara,
	_suffix_yoku, _suffix_yosasou, NULL
};

static int endswith_oneof(const char_t *str, const int str_len,
                          const char_t **suffixes)
{
	int i;

	if (!str || !suffixes) {
		return -EINVAL;
	}

	for (i = 0; suffixes[i]; i++) {
		int y;

		y = char_endswith(str, str_len, suffixes[i], char_len(suffixes[i]));
#if DEBUG_JAPANESE
		fprintf(stderr, "char_endswith(str, %d, suffixes[%d], %d) == %d\n",
		        str_len, i, char_len(suffixes[i]), y);
#endif /* DEBUG_JAPANESE */

		if (y == 0) {
			return i;
		}
	}

	return -ENOENT;
}

static int probe_conjugation_by_suffix_match(const char_t *kana, const int kana_len,
                                             const char_t **suffixes, const int type,
                                             const char_t *dict_suffix, const int dict_suffix_len,
                                             conjugation_t ***results)
{
	int suffix_idx;
	int suffix_len;
	int stem_len;
	int dict_len;
	char_t *dict_form;
	conjugation_t *conjugation;
	int err;

	if (!kana || !results || !suffixes) {
		return -EINVAL;
	}

	if ((suffix_idx = endswith_oneof(kana, kana_len, suffixes)) < 0) {
		/* this is fine, don't return an error */
		return 0;
	}
	suffix_len = char_len(suffixes[suffix_idx]);
	stem_len = kana_len - suffix_len;

	if ((dict_len = char_concat(&dict_form, kana, stem_len, dict_suffix, char_len(dict_suffix))) < 0) {
		fprintf(stderr, "char_concat() = %d\n", dict_len);
		return dict_len;
	}

#if DEBUG_JAPANESE
	{
		char *utf8;

		char_to_utf8_dyn(dict_form, char_len(dict_form), &utf8);
		fprintf(stderr, "%s: dict_form: %s\n", __func__, utf8);
		free(utf8);
	}
#endif /* DEBUG_JAPANESE */

	if ((err = conjugation_new(&conjugation, dict_form, dict_suffix_len,
	                           suffixes[suffix_idx], suffix_len)) < 0) {
		free(dict_form);
		fprintf(stderr, "conjugation_new() = %d\n", err);
		return err;
	}
	conjugation->type = type;

#if DEBUG_JAPANESE
	{
		char *from;
		char *to;

		fprintf(stderr, "kana_len = %d, suffix_len = %d, dict_suffix_len = %d, type = %d\n",
		        kana_len, suffix_len, dict_suffix_len, type);
		fprintf(stderr, "Adding %p to array %p\n", (void*)conjugation, (void*)results);
		char_to_utf8_dyn(kana, kana_len, &from);
		fprintf(stderr, "%s -> ", from);
		char_to_utf8_dyn(dict_form, dict_len, &to);
		fprintf(stderr, "%s\n", to);

		free(from);
		free(to);
	}
#endif /* DEBUG_JAPANESE */

	if (array_add((void***)results, (void*)conjugation) < 0) {
		free(conjugation);
		free(dict_form);

		return -ENOMEM;
	}

	return 0;
}

static int get_i_adjective_conjugation(const char_t *kana, const size_t kana_len,
                                       conjugation_t ***results)
{
	static const struct {
		const char_t **input_suffixes;
		const char_t *dict_suffix;
		int type;
	} _probes[] = {
		{
			.input_suffixes = _yoi_adj_suffixes,
			.dict_suffix    = _suffix_yoi,
			.type           = JA_TYPE_ADJ_YOI
		}, {
			.input_suffixes = _ii_adj_suffixes,
			.dict_suffix    = _suffix_ii,
			.type           = JA_TYPE_ADJ_YOI
		}, {
			.input_suffixes = _i_adj_suffixes,
			.dict_suffix    = _suffix_i,
			.type           = JA_TYPE_ADJ_I
		}, {
			.input_suffixes = NULL,
			.dict_suffix    = NULL,
			.type           = 0
		}
	};

	int err;
	int i;

	for (i = err = 0; err >= 0 && _probes[i].input_suffixes; i++) {
		err = probe_conjugation_by_suffix_match(kana, kana_len, _probes[i].input_suffixes,
		                                        _probes[i].type, _probes[i].dict_suffix, 1,
		                                        results);
	}

	return err;
}

static const struct {
	int (*func)(const char_t *, const size_t, conjugation_t ***);
	const char *name;
} _deconjugators[] = {
	{
		.func = get_i_adjective_conjugation,
		.name = "get_i_adjective_conjugation"
	}, {
		.func = NULL,
		.name = NULL
	}
};

int japanese_unconjugate(const char_t *kana, conjugation_t ***results)
{
	int kana_len;
	int res;
	int i;

	if (!kana || !results) {
		return -EINVAL;
	}

	kana_len = char_len(kana);
	res = -ENOENT;

	for (i = 0; _deconjugators[i].func; i++) {
		int err;

		if ((err = _deconjugators[i].func(kana, kana_len, results)) < 0) {
#if DEBUG_JAPANESE
			fprintf(stderr, "%s() = %d\n", _deconjugators[i].name, err);
#endif /* DEBUG_JAPANESE */
			if (err != -ENOMEM) {
				res = err;
				break;
			}
		} else {
			res = 0;
		}
	}

	return res;

}
