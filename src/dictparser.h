/*
 * dictparser.h - This file is part of mxim
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

#ifndef DICTPARSER_H
#define DICTPARSER_H

#include "dict.h"

typedef struct dict_parser dict_parser_t;
struct string_list;

int dict_parser_new(dict_parser_t **parser, const char *path);
int dict_parser_free(dict_parser_t **parser);
int dict_parser_get_dict(dict_parser_t *parser, dict_t **dict);

#endif /* DICTPARSER_H */
