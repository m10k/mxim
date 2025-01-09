/*
 * string.h - This file is part of mxim
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

#ifndef STRING_H
#define STRING_H

#include <stddef.h>

typedef struct string string_t;

int string_new(string_t **str);
int string_free(string_t **str);

int string_clear(string_t *str);
int string_append(string_t *dst, const string_t *src);
int string_append_char(string_t *dst, const char_t *src, const size_t src_len);
int string_append_utf8(string_t *dst, const char *src, const size_t src_len);
int string_append_fmt(string_t *dst, const char *ftm, ...);
int string_replace(string_t *str, const char *search, const char *replace);
int string_get_utf8(string_t *str, char **dst);

#endif /* STRING_H */
