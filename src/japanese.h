/*
 * japanese.h - This file is part of mxim
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

#ifndef MXIM_JAPANESE_H
#define MXIM_JAPANESE_H

#include "char.h"
#include "conjugation.h"

typedef enum {
	JA_TYPE_OTHER = 0,
	JA_TYPE_ADJ_I,
	JA_TYPE_ADJ_YOI,
	JA_TYPE_ADJ_KARI,
	JA_TYPE_ADJ_KU,
	JA_TYPE_ADJ_NA,
	JA_TYPE_VERB,
	JA_TYPE_VERB_1,
	JA_TYPE_VERB_1_KURERU,
	JA_TYPE_VERB_5,
	JA_TYPE_VERB_5_IKU,
	JA_TYPE_VERB_5_ARU,
	JA_TYPE_VERB_5_RU_IRREG,
	JA_TYPE_VERB_5_U_SPEC,
	JA_TYPE_VERB_5_URU_OLD,
	JA_TYPE_VERB_KURU,
	JA_TYPE_VERB_NU_IRREG,
	JA_TYPE_VERB_RU_IRREG,
	JA_TYPE_VERB_SURU,
	JA_TYPE_VERB_SU
} japanese_type_t;

int japanese_unconjugate(const char_t *kana, conjugation_t ***results);

#endif /* MXIM_JAPANESE_H */
