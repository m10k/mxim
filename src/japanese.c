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

static const char_t *_i_adj_suffixes[] = {
	_suffix_karou, _suffix_i, _suffix_katta, _suffix_kereba, _suffix_kattara, _suffix_ku, NULL
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

static int get_i_adjective_conjugation(const char_t *kana, const size_t kana_len,
                                       conjugation_t ***results)
{
	int suffix_idx;
	int suffix_len;
	int stem_len;
	char_t *dict_form;
        conjugation_t *conjugation;

	if (!kana || !results) {
		return -EINVAL;
	}

	/*
	 * Check if the input ends with one of the suffixes
	 * that are possible i-adjective conjugations
	 */
	if ((suffix_idx = endswith_oneof(kana, kana_len, _i_adj_suffixes)) < 0) {
		return suffix_idx;
	}

	suffix_len = char_len(_i_adj_suffixes[suffix_idx]);
	stem_len = kana_len - suffix_len;

#if DEBUG_JAPANESE
	fprintf(stderr,
	        "suffix_idx = %d\n"
	        "suffix_len = %d\n"
	        "stem_len   = %d\n", suffix_idx, suffix_len, stem_len);
#endif /* DEBUG_JAPANESE */

	/*
	 * Get the dictionary form of the adjective by stripping
	 * the conjugated suffix and adding い to the stem
	 */
	if (char_concat(&dict_form, kana, stem_len, _suffix_i, 1) < 0) {
		fprintf(stderr, "Could not concatenate i adjective\n");
		return -ENOMEM;
	}

	if (conjugation_new(&conjugation, dict_form, 1, _i_adj_suffixes[suffix_idx], suffix_len) < 0) {
		free(dict_form);
		return -ENOMEM;
	}

	conjugation->type = JA_TYPE_ADJ_I;

#if DEBUG_JAPANESE
	{
		char *from;
		char *to;

		fprintf(stderr, "Adding %p to array %p\n", (void*)conjugation, (void*)results);
		char_to_utf8_dyn(kana, kana_len, &from);
		fprintf(stderr, "%s -> ", from);
		char_to_utf8_dyn(dict_form, stem_len + 1, &to);
		fprintf(stderr, "%s\n", to);

		free(from);
		free(to);
	}
#endif /* DEBUG_JAPANESE */

	if (array_add((void***)results, (void*)conjugation) < 0) {
#if DEBUG_JAPANESE
		fprintf(stderr, "Could not add adjective to array\n");
#endif /* DEBUG_JAPANESE */
		free(conjugation);
		free(dict_form);

		return -ENOMEM;
	}

	return 0;
}

int japanese_unconjugate(const char_t *kana, conjugation_t ***results)
{
	int kana_len;
	int err;

	if (!kana || !results) {
		return -EINVAL;
	}

	kana_len = char_len(kana);

	if ((err = get_i_adjective_conjugation(kana, kana_len, results)) >= 0) {
		/* word could be an い-adjective */
#if DEBUG_JAPANESE
		fprintf(stderr, "get_i_adjective() = %d\n", err);
#endif /* DEBUG_JAPANESE */
		return 0;
	}

	return err;
}
