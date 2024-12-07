/*
 * ximtypes.c - This file is part of mxim
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

#include "ximtypes.h"
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct XIMSTRING {
	uint16_t len;
	char data[];
} __attribute__((packed));

struct XIMATTR {
	uint16_t id;
	uint16_t type;
	char string[];
} __attribute__((packed));

struct XIMATTRIBUTE {
	uint16_t id;
	uint16_t data_len;
	uint8_t data[];
} __attribute__((packed));

struct XIMEXT {
	uint8_t major;
	uint8_t minor;
	uint8_t name[];
} __attribute__((packed));

int decode_STRING(char **dst, const uint8_t *src, const size_t src_len)
{
	struct XIMSTRING *raw;
        char *parsed;
        size_t len;
        int parsed_len;
        int padded_len;

        if (src_len < sizeof(*raw)) {
                return -ENOMSG;
        }
        raw = (struct XIMSTRING*)src;

        if (src_len < (sizeof(*raw) + raw->len)) {
	        return -EBADMSG;
        }
        len = raw->len + 1;

        if (!(parsed = malloc(len))) {
                return -ENOMEM;
        }

        snprintf(parsed, len, "%s", raw->data);
        parsed_len = sizeof(*raw) + raw->len;
        padded_len = parsed_len + PAD(parsed_len);

        *dst = parsed;
        return padded_len;
}

int decode_STR(char **dst, const uint8_t *src, const size_t src_len)
{
	struct {
		uint8_t len;
		char data[];
	} __attribute__((packed)) *raw;
	char *parsed;
	size_t len;
	int parsed_len;

	if (src_len < sizeof(*raw)) {
		return -ENOMSG;
	}
	raw = (void*)src;

	if (src_len < (sizeof(*raw) + raw->len)) {
		return -EBADMSG;
	}
	len = raw->len + 1;

	if (!(parsed = malloc(len))) {
		return -ENOMEM;
	}

	snprintf(parsed, len, "%s", raw->data);
	parsed_len = sizeof(*raw) + raw->len;

	*dst = parsed;
	return parsed_len;
}

int decode_ATTR(attr_t **dst, const uint8_t *src, const size_t src_len)
{
	struct XIMATTR *raw;
	attr_t *attr;
	int string_len;
	int decoded_len;
	int padded_len;

	if (src_len < sizeof(*raw)) {
		return -ENOMSG;
	}

	raw = (struct XIMATTR*)src;

	if (!(attr = calloc(1, sizeof(*attr)))) {
		return -ENOMEM;
	}

	attr->id = raw->id;
	attr->type = raw->type;
	string_len = decode_STRING(&attr->name, (uint8_t*)&raw->string, src_len - sizeof(*raw));

	if (string_len < 0) {
		free(attr);
		return -ENOMSG;
	}

	decoded_len = sizeof(*raw) + string_len;
	padded_len = decoded_len + PAD(decoded_len);

	*dst = attr;
	return padded_len;
}

int decode_ATTRIBUTE(attr_value_t **dst, const uint8_t *src, const size_t src_len)
{
	struct XIMATTRIBUTE *raw;
	attr_value_t *attr;
	int decoded_len;
	int padded_len;

	if (src_len < sizeof(*raw)) {
		return -ENOMSG;
	}

	raw = (struct XIMATTRIBUTE*)src;

	if (src_len < (sizeof(*raw) + raw->data_len)) {
		return -EBADMSG;
	}

	if (!(attr = calloc(1, sizeof(*attr)))) {
		return -ENOMEM;
	}

	attr->id = raw->id;
	attr->len = raw->data_len;

	if (!(attr->data = malloc(raw->data_len))) {
		free(attr);
		return -ENOMEM;
	}

	memcpy(attr->data, raw->data, attr->len);
	decoded_len = sizeof(*raw) + attr->len;
	padded_len = decoded_len + PAD(decoded_len);

	*dst = attr;
	return padded_len;
}

int encode_STRING(const char *src, uint8_t *dst, const size_t dst_size)
{
	struct XIMSTRING *raw;
	size_t string_len;
	size_t raw_len;
	size_t padding_len;
	size_t padded_len;

	string_len = strlen(src);
	if (string_len > UINT16_MAX) {
		return -EOVERFLOW;
	}

	raw_len = sizeof(*raw) + string_len;
	padding_len = PAD(raw_len);
	padded_len = raw_len + padding_len;

	if (dst_size < padded_len) {
		return -ENOMEM;
	}

	raw = (struct XIMSTRING*)dst;
	raw->len = strlen(src);
	memcpy(raw->data, src, string_len);
	memset(raw->data + string_len, 0, padding_len);

	return (int)padded_len;
}

int encode_ATTR(const attr_t *src, uint8_t *dst, const size_t dst_size)
{
	struct XIMATTR *raw;
	size_t raw_len;
	size_t padding_len;
	size_t padded_len;

	raw_len = sizeof(*raw) + sizeof(uint16_t) + strlen(src->name);
	padding_len = PAD(raw_len);
	padded_len = raw_len + padding_len;

	if (dst_size < padded_len) {
		return -ENOMEM;
	}

	raw = (struct XIMATTR*)dst;
	raw->id = src->id;
	raw->type = src->type;
	encode_STRING(src->name, (uint8_t*)(raw + 1), dst_size - sizeof(*raw));
	memset(raw + raw_len, 0, padding_len);

	return (int)padded_len;
}

int encode_ATTRIBUTE(const attr_value_t *src, uint8_t *dst, const size_t dst_size)
{
	struct XIMATTRIBUTE *raw;
	size_t raw_len;
	size_t padding_len;
	size_t padded_len;

	raw_len = sizeof(*raw) + src->len;
	padding_len = PAD(raw_len);
	padded_len = raw_len + padding_len;

	if (dst_size < padded_len) {
		return -ENOMEM;
	}

	raw = (struct XIMATTRIBUTE*)dst;
	raw->id = src->id;
	raw->data_len = src->len;
	memcpy(raw->data, src->data, src->len);
	memset(raw->data + src->len, 0, padding_len);

	return (int)padded_len;
}

int encode_EXT(const ext_t *src, uint8_t *dst, const size_t dst_size)
{
	struct XIMEXT *raw;
	size_t raw_len;
	size_t padding_len;
	size_t padded_len;
	int name_len;

	raw_len = sizeof(*raw) + strlen(src->name) + sizeof(uint16_t);
	padding_len = PAD(raw_len);
	padded_len = raw_len + padding_len;

	if (dst_size < padded_len) {
		return -ENOMEM;
	}

	raw = (struct XIMEXT*)dst;
	raw->major = src->major;
	raw->minor = src->minor;

	if ((name_len = encode_STRING(src->name, raw->name, dst_size - sizeof(*raw))) < 0) {
		return name_len;
	}

	return sizeof(*raw) + name_len;
}

static int _count_attributes(const uint8_t *list, const size_t list_len)
{
	struct XIMATTRIBUTE *attribute;
	int num_attributes;
	size_t offset;

	offset = 0;
	num_attributes = 0;

	while (offset < list_len) {
		attribute = (struct XIMATTRIBUTE*)(list + offset);
		num_attributes++;
		offset += sizeof(*attribute) + attribute->data_len;
	}

	return num_attributes;
}

int decode_LISTofATTRIBUTE(attr_value_t ***dst, const void *src, const size_t src_len)
{
	attr_value_t **attributes;
	int num_attributes;
	int parsed_len;
	int i;

	if ((num_attributes = _count_attributes(src, src_len)) < 0) {
		return num_attributes;
	}

	if (!(attributes = calloc(num_attributes + 1, sizeof(attr_value_t*)))) {
		return -ENOMEM;
	}

	for (i = 0, parsed_len = 0; i < num_attributes; i++) {
		int attribute_len;

		if ((attribute_len = decode_ATTRIBUTE(&attributes[i],
		                                      src + parsed_len,
		                                      src_len - parsed_len)) < 0) {
			break;
		}

		parsed_len += attribute_len;
	}

	if (i != num_attributes) {
		/* did not decode all attributes */

		while (i >= 0) {
			free(attributes[i--]);
		}
		free(attributes);

		return -EBADMSG;
	}

	*dst = attributes;
	return parsed_len;
}

int attr_clone(attr_t **dst, const attr_t *src)
{
	attr_t *a;

	if (!(a = calloc(1, sizeof(*a)))) {
		return -ENOMEM;
	}

	if (!(a->name = strdup(src->name))) {
		free(a);
		return -ENOMEM;
	}

	a->id = src->id;
	a->type = src->type;

	*dst = a;
	return 0;
}

int attr_free(attr_t **attr)
{
	if (!attr || !*attr) {
		return -EINVAL;
	}

	free((*attr)->name);
	free(*attr);
	*attr = NULL;

	return 0;
}

int attrs_free(attr_t ***attrs)
{
	int i;

	if (!attrs || !*attrs) {
		return -EINVAL;
	}

	for (i = 0; (*attrs)[i]; i++) {
		attr_free(&(*attrs)[i]);
	}
	free(*attrs);
	*attrs = NULL;

	return 0;
}

int attr_value_clone(attr_value_t **dst, const attr_value_t *src)
{
	attr_value_t *av;

	if (!(av = calloc(1, sizeof(*av)))) {
		return -ENOMEM;
	}

	if (!(av->data = malloc(src->len))) {
		free(av);
		return -ENOMEM;
	}

	av->id = src->id;
	av->len = src->len;
	memmove(av->data, src->data, src->len);

	*dst = av;
	return 0;
}

int attr_value_free(attr_value_t **value)
{
	if (!value || !*value) {
		return -EINVAL;
	}

	free((*value)->data);
	free(*value);
	*value = NULL;

	return 0;
}

int attr_values_free(attr_value_t ***values)
{
	int i;

	if (!values || !*values) {
		return -EINVAL;
	}

	for (i = 0; (*values)[i]; i++) {
		attr_value_free(&(*values)[i]);
	}
	free(*values);
	*values = NULL;

	return 0;
}
