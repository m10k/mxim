/*
 * ximtypes.h - This file is part of mxim
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

#ifndef XIMTYPES_H
#define XIMTYPES_H

#include <stdint.h>
#include <sys/types.h>

#define PAD(n) ((4 - ((n) % 4)) % 4)

typedef enum {
	ATTR_TYPE_CARD32    =      3,
	ATTR_TYPE_WINDOW    =      5,
	ATTR_TYPE_XIMSTYLES =     10,
	ATTR_TYPE_INVALID   = 0xfefe
} attr_type_t;

typedef struct {
	uint16_t id;
	attr_type_t type;
	char *name;
} attr_t;

typedef struct {
	uint16_t id;
	size_t len;
	void *data;
} attr_value_t;

typedef struct {
	uint8_t major;
	uint8_t minor;
	char *name;
} ext_t;

int decode_STRING(char **dst, const uint8_t *src, const size_t src_len);
int decode_STR(char **dst, const uint8_t *src, const size_t src_len);
int decode_ATTR(attr_t **attr, const uint8_t *src, const size_t src_len);
int decode_ATTRIBUTE(attr_value_t **val, const uint8_t *src, const size_t src_len);
int decode_LISTofATTRIBUTE(attr_value_t ***dst, const void *src, const size_t src_len);

int encode_STRING(const char *src, uint8_t *dst, const size_t dst_size);
int encode_ATTR(const attr_t *src, uint8_t *dst, const size_t dst_size);
int encode_ATTRIBUTE(const attr_value_t *src, uint8_t *dst, const size_t dst_size);
int encode_EXT(const ext_t *src, uint8_t *dst, const size_t dst_size);

#define decode_ENCODINGINFO decode_STRING

int attr_clone(attr_t **dst, const attr_t *src);
int attr_free(attr_t **attr);
int attrs_free(attr_t ***attrs);

int attr_value_clone(attr_value_t **dst, const attr_value_t *src);
int attr_value_free(attr_value_t **value);
int attr_values_free(attr_value_t ***values);

#endif /* XIMTYPES_H */
