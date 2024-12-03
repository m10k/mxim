/*
 * ximproto.c - This file is part of mxim
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

#include "ximproto.h"
#include "ximtypes.h"
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct XIM_PACKET {
	uint8_t opcode_major;
	uint8_t opcode_minor;
	uint16_t length;
} __attribute__((packed));

struct XIM_ERROR {
	uint16_t im;
	uint16_t ic;
	uint16_t flag;
	uint16_t error_code;
	uint16_t detail_len;
	uint16_t detail_type;
	uint8_t detail[];
};

struct XIM_CONNECT {
	uint8_t byte_order;
	uint8_t unused;

	struct {
		uint16_t major;
		uint16_t minor;
	} client_ver;

	struct {
		uint16_t num_protos;
		char protos[];
	} auth;
} __attribute__((packed));

struct XIM_CONNECT_REPLY {
	struct {
		uint16_t major;
		uint16_t minor;
	} server_ver;
} __attribute__((packed));

struct XIM_OPEN {
	uint8_t len;
	char str[];
};

struct XIM_OPEN_REPLY {
	uint16_t im_id;
	/* length of LISTofXIMATTR */
	/* LISTofXIMATTR */
	/* length of LISTofXICATTR */
	/* 2 bytes padding */
	/* LISTofXICATTR */
};

struct XIM_QUERY_EXTENSION {
	uint16_t im;
	uint16_t exts_len;
	char exts[];
	/* pad */
};

struct XIM_QUERY_EXTENSION_REPLY {
	uint16_t im;
	uint16_t exts_len;
	uint8_t exts[];
};

struct XIM_ENCODING_NEGOTIATION {
	uint16_t im;
	uint8_t encodings[];
	/* pad */
	/* length of encoding details */
	/* 2 bytes padding */
	/* list of encoding details */
};

struct XIM_ENCODING_NEGOTIATION_REPLY {
	uint16_t im;
	uint16_t category;
	int16_t encoding;
	uint16_t unused;
};

struct XIM_GET_IM_VALUES {
	uint16_t im;
	uint16_t len_attrs;
	uint16_t attrs[];
};

struct XIM_GET_IM_VALUES_REPLY {
	uint16_t im;
	uint16_t len_values;
	uint8_t values[];
};

struct XIM_SET_IM_VALUES {
	uint16_t im;
	uint16_t len_values;
	uint8_t values[];
};

struct XIM_SET_IM_VALUES_REPLY {
	uint16_t im;
	uint16_t unused;
};

struct XIM_CREATE_IC {
	uint16_t im;
	uint16_t len_values;
	uint8_t values[];
};

struct XIM_CREATE_IC_REPLY {
	uint16_t im;
	uint16_t ic;
};

struct XIM_GET_IC_VALUES {
	uint16_t im;
	uint16_t ic;
	uint16_t len_attrs;
	uint16_t attrs[];
};

struct XIM_GET_IC_VALUES_REPLY {
	uint16_t im;
	uint16_t ic;
	uint16_t len_values;
	uint8_t values[];
};

struct XIM_SET_IC_VALUES {
	uint16_t im;
	uint16_t ic;
	uint16_t len_values;
	uint16_t unused;
	uint8_t values[];
};

struct XIM_SET_IC_VALUES_REPLY {
	uint16_t im;
	uint16_t ic;
};

static const struct {
	xim_msg_type_t type;
	char *name;
	size_t size;
} _msg_info[] = {
	[XIM_ERROR] = {
		.type = XIM_ERROR,
		.name = "XIM_ERROR",
		.size = sizeof(xim_msg_error_t)
	},
	[XIM_CONNECT] = {
		.type = XIM_CONNECT,
		.name = "XIM_CONNECT",
		.size = sizeof(xim_msg_connect_t)
	},
	[XIM_CONNECT_REPLY] = {
		.type = XIM_CONNECT_REPLY,
		.name = "XIM_CONNECT_REPLY",
		.size = sizeof(xim_msg_connect_reply_t)
	},
	[XIM_OPEN] = {
		.type = XIM_OPEN,
		.name = "XIM_OPEN",
		.size = sizeof(xim_msg_open_t)
	},
	[XIM_OPEN_REPLY] = {
		.type = XIM_OPEN_REPLY,
		.name = "XIM_OPEN_REPLY",
		.size = sizeof(xim_msg_open_reply_t)
	},
	[XIM_QUERY_EXTENSION] = {
		.type = XIM_QUERY_EXTENSION,
		.name = "XIM_QUERY_EXTENSION",
		.size = sizeof(xim_msg_query_extension_t)
	},
	[XIM_QUERY_EXTENSION_REPLY] = {
		.type = XIM_QUERY_EXTENSION_REPLY,
		.name = "XIM_QUERY_EXTENSION_REPLY",
		.size = sizeof(xim_msg_query_extension_reply_t)
	},
	[XIM_ENCODING_NEGOTIATION] = {
		.type = XIM_ENCODING_NEGOTIATION,
		.name = "XIM_ENCODING_NEGOTIATION",
		.size = sizeof(xim_msg_encoding_negotiation_t)
	},
	[XIM_ENCODING_NEGOTIATION_REPLY] = {
		.type = XIM_ENCODING_NEGOTIATION_REPLY,
		.name = "XIM_ENCODING_NEGOTIATION_REPLY",
		.size = sizeof(xim_msg_encoding_negotiation_reply_t)
	},
	[XIM_GET_IM_VALUES] = {
		.type = XIM_GET_IM_VALUES,
		.name = "XIM_GET_IM_VALUES",
		.size = sizeof(xim_msg_get_im_values_t)
	},
	[XIM_SET_IM_VALUES] = {
		.type = XIM_SET_IM_VALUES,
		.name = "XIM_SET_IM_VALUES",
		.size = sizeof(xim_msg_set_im_values_t)
	},
	[XIM_CREATE_IC] = {
		.type = XIM_CREATE_IC,
		.name = "XIM_CREATE_IC",
		.size = sizeof(xim_msg_create_ic_t)
	},
	[XIM_CREATE_IC_REPLY] = {
		.type = XIM_CREATE_IC_REPLY,
		.name = "XIM_CREATE_IC_REPLY",
		.size = sizeof(xim_msg_create_ic_reply_t)
	},
	[XIM_GET_IC_VALUES] = {
		.type = XIM_GET_IC_VALUES,
		.name = "XIM_GET_IC_VALUES",
		.size = sizeof(xim_msg_get_ic_values_t)
	},
	[XIM_GET_IC_VALUES_REPLY] = {
		.type = XIM_GET_IC_VALUES_REPLY,
		.name = "XIM_GET_IC_VALUES_REPLY",
		.size = sizeof(xim_msg_get_ic_values_reply_t)
	},
	[XIM_SET_IC_VALUES] = {
		.type = XIM_SET_IC_VALUES,
		.name = "XIM_SET_IC_VALUES",
		.size = sizeof(xim_msg_set_ic_values_t)
	},
	[XIM_SET_IC_VALUES_REPLY] = {
		.type = XIM_SET_IC_VALUES_REPLY,
		.name = "XIM_SET_IC_VALUES_REPLY",
		.size = sizeof(xim_msg_set_ic_values_reply_t)
	}
};

static int need_more_data(struct XIM_PACKET *src, const size_t src_len)
{
	return (src_len < sizeof(*src) || /* check if header is there */
	        src_len < (sizeof(*src) + src->length * 4)); /* check if payload is there */
}

static int decode_XIM_ERROR(xim_msg_t **dst, const struct XIM_ERROR *src, const size_t src_len)
{
	xim_msg_error_t *msg;
	int parsed_len;
	int padded_len;

	if (src_len < sizeof(*src)) {
		return -ENOMSG;
	}

	if (src_len < (sizeof(*msg) + src->detail_len)) {
		return -EBADMSG;
	}

	if (!(msg = calloc(1, sizeof(*msg)))) {
		return -ENOMEM;
	}

	if (!(msg->detail = malloc(src->detail_len))) {
		free(msg);
		return -ENOMEM;
	}

	msg->im = src->im;
	msg->ic = src->ic;
	msg->flags = src->flag;
	msg->error = src->error_code;
	msg->detail_len = src->detail_len;
	msg->detail_type = src->detail_type;
	parsed_len = sizeof(*src) + src->detail_len;
	padded_len = parsed_len + PAD(parsed_len);

	memmove(msg->detail, src->detail, msg->detail_len);

	*dst = (xim_msg_t*)msg;
	return padded_len;
}

static int decode_XIM_CONNECT(xim_msg_t **dst, const struct XIM_CONNECT *src, const size_t src_len)
{
	xim_msg_connect_t *msg;
	size_t remaining_len;
	int n;
	int skip;

	if (!(msg = calloc(1, sizeof(*msg)))) {
		return -ENOMEM;
	}

	msg->byte_order = src->byte_order;
	msg->client_ver.major = src->client_ver.major;
	msg->client_ver.minor = src->client_ver.minor;

	if (!(msg->auth.protos = calloc(src->auth.num_protos + 1, sizeof(char*)))) {
		free(msg);
		return -ENOMEM;
	}

	msg->auth.num_protos = src->auth.num_protos;
	remaining_len = src_len - offsetof(struct XIM_CONNECT, auth.num_protos);

	for (skip = n = 0; n < src->auth.num_protos; n++) {
		int parsed;

		if ((parsed = decode_STRING(&msg->auth.protos[n],
		                            (uint8_t*)src->auth.protos + skip,
		                            remaining_len)) < 0) {
			break;
		}

		skip += parsed;
		remaining_len -= parsed;
	}

	*dst = (xim_msg_t*)msg;
	return skip + sizeof(*src);
}

static int decode_XIM_OPEN(xim_msg_t **dst, const struct XIM_OPEN *src, const size_t src_len)
{
	xim_msg_open_t *msg;
	char *locale;
	int parsed_len;
	int padding;
	int padded_len;

	if ((parsed_len = decode_STR(&locale, (const uint8_t*)src, src_len)) < 0) {
		return parsed_len;
	}

	if (!(msg = calloc(1, sizeof(*msg)))) {
		free(locale);
		return -ENOMEM;
	}

	msg->locale = locale;
	padding = PAD(parsed_len);
	padded_len = parsed_len + padding;

	*dst = (xim_msg_t*)msg;
	return padded_len;
}

static int decode_XIM_QUERY_EXTENSION(xim_msg_t **dst, const struct XIM_QUERY_EXTENSION *src,
                                      const size_t src_len)
{
	xim_msg_query_extension_t *msg;
	int computed_len;
	int padding_len;
	int padded_len;
	int parsed_len;

	computed_len = sizeof(*src) + src->exts_len;
	padding_len = PAD(computed_len);
	padded_len = computed_len + padding_len;

	if (padded_len > src_len) {
		return -EBADMSG;
	}

	if (!(msg = calloc(1, sizeof(*msg)))) {
		return -ENOMEM;
	}

	msg->im = src->im;

	for (parsed_len = sizeof(*src); parsed_len < computed_len; ) {
		char **new_exts;
		char *ext;
		int ext_len;

		if ((ext_len = decode_STR(&ext, (uint8_t*)src + parsed_len, src_len - parsed_len)) < 0) {
			free(msg);
			return -EBADMSG;
		}

		if (!(new_exts = realloc(msg->exts, sizeof(char*) * msg->num_exts + 1))) {
			free(msg);
			return -ENOMEM;
		}

		new_exts[msg->num_exts++] = ext;
		msg->exts = new_exts;

	        parsed_len += ext_len;
	}

	*dst = (xim_msg_t*)msg;
	return parsed_len + padding_len;
}

struct LISTofSTR {
	uint16_t len;
	uint8_t strings[];
} __attribute__((packed));

static int count_strings(const struct LISTofSTR *list, const size_t list_len)
{
	size_t offset;
	int count;

	if (list_len < sizeof(*list)) {
		return -ENOMSG;
	}

	if (list_len < (sizeof(*list) + list->len)) {
		return -EBADMSG;
	}

	count = 0;
	offset = 0;

	while (offset < list->len) {
		count++;
		offset += list->strings[offset] + 1;
	}

	return count;
}

static int decode_LISTofSTR(char ***dst, const void *src, const size_t src_len)
{
	const struct LISTofSTR *raw;
	int num_strings;
	char **strings;
	int parsed_len;
	int i;

	if (src_len < sizeof(*raw)) {
		return -ENOMSG;
	}

	raw = src;
	if ((num_strings = count_strings(raw, src_len)) < 0) {
		return num_strings;
	}

	if (!(strings = calloc(num_strings + 1, sizeof(char*)))) {
		return -ENOMEM;
	}

	for (i = 0, parsed_len = 0; i < num_strings; i++) {
		int str_len;

		if ((str_len = decode_STR(&strings[i], raw->strings + parsed_len,
		                          src_len - sizeof(*raw) - parsed_len)) < 0) {
			break;
		}

		parsed_len += str_len;
	}

	if (i != num_strings) {
		/* did not decode all strings */
		while (i >= 0) {
			/* FIXME: i-- */
			free(strings[i]);
		}
		free(strings);

		return -EBADMSG;
	}

	*dst = strings;
	return parsed_len + sizeof(*raw);
}

static int decode_XIM_ENCODING_NEGOTIATION(xim_msg_t **dst, const struct XIM_ENCODING_NEGOTIATION *src,
                                           const size_t src_len)
{
	xim_msg_encoding_negotiation_t *msg;
	char **encodings;
	int parsed_len;
	uint16_t details_len;

	if (src_len < sizeof(*src)) {
		return -ENOMSG;
	}

	if (!(msg = calloc(1, sizeof(*msg)))) {
		return -ENOMEM;
	}

	if ((parsed_len = decode_LISTofSTR(&encodings, src->encodings,
	                                   src_len - sizeof(*src))) < 0) {
		return parsed_len;
	}

	msg->im = src->im;
	msg->encodings = encodings;
	parsed_len += PAD(parsed_len);
	details_len = *(uint16_t*)(src->encodings + parsed_len);
	parsed_len += 4; /* skip over details size and its padding */
	parsed_len += details_len; /* skip over details */

	/* FIXME: Implement parsing of details? */

	*dst = (xim_msg_t*)msg;
	return parsed_len;
}

static int decode_XIM_GET_IM_VALUES(xim_msg_t **dst, const struct XIM_GET_IM_VALUES *src,
                                    const size_t src_len)
{
	xim_msg_get_im_values_t *msg;
	int data_len;
	int padded_len;
	int i;

	if (src_len < sizeof(*src)) {
		return -ENOMSG;
	}

	data_len = sizeof(*src) + src->len_attrs;
	padded_len = data_len + PAD(data_len);

	if (src_len < data_len) {
		return -EBADMSG;
	}

	if (!(msg = calloc(1, sizeof(*msg)))) {
		return -ENOMEM;
	}

	msg->im = src->im;
	msg->num_attrs = src->len_attrs / sizeof(uint16_t);

	if (!(msg->attrs = calloc(msg->num_attrs, sizeof(int)))) {
		free(msg);
		return -ENOMEM;
	}

	for (i = 0; i < msg->num_attrs; i++) {
		msg->attrs[i] = src->attrs[i];
	}

	*dst = (xim_msg_t*)msg;
	return padded_len;
}

static int decode_XIM_SET_IM_VALUES(xim_msg_t **dst, const struct XIM_SET_IM_VALUES *src,
                                    const size_t src_len)
{
	xim_msg_set_im_values_t *msg;
	int parsed_len;

	if (src_len < sizeof(*src)) {
		return -ENOMSG;
	}

	if (src_len < (sizeof(*src) + src->len_values)) {
		return -EBADMSG;
	}

	if (!(msg = calloc(1, sizeof(*msg)))) {
		return -ENOMEM;
	}

	msg->im = src->im;

	if ((parsed_len = decode_LISTofATTRIBUTE(&msg->values, src->values, src->len_values)) < 0) {
		free(msg);
		return parsed_len;
	}

	*dst = (xim_msg_t*)msg;
	return parsed_len + sizeof(*src);
}

static int decode_XIM_GET_IC_VALUES(xim_msg_t **dst, const struct XIM_GET_IC_VALUES *src,
                                    const size_t src_len)
{
	xim_msg_get_ic_values_t *msg;
	int data_len;
	int padded_len;
	int i;

	if (src_len < sizeof(*src)) {
		return -ENOMSG;
	}

	data_len = sizeof(*src) + src->len_attrs;
	padded_len = data_len + PAD(data_len);

	if (src_len < data_len) {
		return -EBADMSG;
	}

	if (!(msg = calloc(1, sizeof(*msg)))) {
		return -ENOMEM;
	}

	msg->im = src->im;
	msg->ic = src->ic;
	msg->num_attrs = src->len_attrs / sizeof(uint16_t);

	if (!(msg->attrs = calloc(msg->num_attrs, sizeof(int)))) {
		free(msg);
		return -ENOMEM;
	}

	for (i = 0; i < msg->num_attrs; i++) {
		msg->attrs[i] = src->attrs[i];
	}

	*dst = (xim_msg_t*)msg;
	return padded_len;
}

static int decode_XIM_SET_IC_VALUES(xim_msg_t **dst, const struct XIM_SET_IC_VALUES *src,
                                    const size_t src_len)
{
	xim_msg_set_ic_values_t *msg;
	int parsed_len;

	if (!dst || !src) {
		return -EINVAL;
	}

	if (src_len < sizeof(*src)) {
		return -ENOMSG;
	}

	if (src_len < (sizeof(*src) + src->len_values)) {
		return -EBADMSG;
	}

	if (!(msg = calloc(1, sizeof(*msg)))) {
		return -ENOMEM;
	}

	msg->im = src->im;
	msg->ic = src->ic;

	if ((parsed_len = decode_LISTofATTRIBUTE(&msg->values, src->values, src->len_values)) < 0) {
		free(msg);
		return parsed_len;
	}

	*dst = (xim_msg_t*)msg;
	return parsed_len + sizeof(*src);
}

static int decode_XIM_CREATE_IC(xim_msg_t **dst, const struct XIM_CREATE_IC *src,
                                const size_t src_len)
{
	xim_msg_create_ic_t *msg;
	int decoded_len;

	if (src_len < sizeof(*src)) {
		return -ENOMSG;
	}

	if (src_len < sizeof(*src) + src->len_values) {
		return -EBADMSG;
	}

	if (!(msg = calloc(1, sizeof(*msg)))) {
		return -ENOMEM;
	}

	msg->im = src->im;

	if ((decoded_len = decode_LISTofATTRIBUTE(&msg->values, src->values, src->len_values)) < 0) {
		free(msg);
		return decoded_len;
	}

	*dst = (xim_msg_t*)msg;
	return decoded_len + sizeof(*src);
}

int xim_msg_decode(xim_msg_t **dst, const uint8_t *src, const size_t src_len)
{
	struct XIM_PACKET *hdr;
	xim_msg_t *msg;
	int err;

	hdr = (struct XIM_PACKET*)src;
	msg = NULL;

	if (need_more_data(hdr, src_len)) {
		err = -EAGAIN;
	} else {
		switch (hdr->opcode_major) {
		case XIM_ERROR:
			fprintf(stderr, "Decoding XIM_ERROR\n");
			err = decode_XIM_ERROR(&msg, (struct XIM_ERROR*)(hdr + 1),
			                       src_len - sizeof(*hdr));
			break;

		case XIM_CONNECT:
			fprintf(stderr, "Decoding XIM_CONNECT\n");
			err = decode_XIM_CONNECT(&msg, (struct XIM_CONNECT*)(hdr + 1),
			                         src_len - sizeof(*hdr));
			break;

		case XIM_OPEN:
			fprintf(stderr, "Decoding XIM_OPEN\n");
			err = decode_XIM_OPEN(&msg, (struct XIM_OPEN*)(hdr + 1),
			                      src_len - sizeof(*hdr));
			break;

		case XIM_QUERY_EXTENSION:
			fprintf(stderr, "Decoding XIM_QUERY_EXTENSION\n");
			err = decode_XIM_QUERY_EXTENSION(&msg, (struct XIM_QUERY_EXTENSION*)(hdr + 1),
			                                 src_len - sizeof(*hdr));
			break;

		case XIM_ENCODING_NEGOTIATION:
			fprintf(stderr, "Decoding XIM_ENCODING_NEGOTIATION\n");
			err = decode_XIM_ENCODING_NEGOTIATION(&msg,
			                                      (struct XIM_ENCODING_NEGOTIATION*)(hdr + 1),
			                                      src_len - sizeof(*hdr));
			break;

		case XIM_GET_IM_VALUES:
			fprintf(stderr, "Decoding XIM_GET_IM_VALUES\n");
			err = decode_XIM_GET_IM_VALUES(&msg, (struct XIM_GET_IM_VALUES*)(hdr + 1),
			                               src_len - sizeof(*hdr));
			break;

		case XIM_SET_IM_VALUES:
			fprintf(stderr, "Decoding XIM_SET_IM_VALUES\n");
			err = decode_XIM_SET_IM_VALUES(&msg, (struct XIM_SET_IM_VALUES*)(hdr + 1),
			                               src_len - sizeof(*hdr));
			break;

		case XIM_GET_IC_VALUES:
			fprintf(stderr, "Decoding XIM_GET_IC_VALUES\n");
			err = decode_XIM_GET_IC_VALUES(&msg, (struct XIM_GET_IC_VALUES*)(hdr + 1),
			                               src_len - sizeof(*hdr));
			break;

		case XIM_SET_IC_VALUES:
			fprintf(stderr, "Decoding XIM_SET_IC_VALUES\n");
			err = decode_XIM_SET_IC_VALUES(&msg, (struct XIM_SET_IC_VALUES*)(hdr + 1),
			                               src_len - sizeof(*hdr));
			break;

		case XIM_CREATE_IC:
			fprintf(stderr, "Decoding XIM_CREATE_IC\n");
			err = decode_XIM_CREATE_IC(&msg, (struct XIM_CREATE_IC*)(hdr + 1),
			                           src_len - sizeof(*hdr));
			break;

		default:
			fprintf(stderr, "Decoding of %hhu type message not implemented\n",
			        hdr->opcode_major);
			err = -ENOSYS;
			break;
		}

		if (err > 0) {
			msg->type = hdr->opcode_major;
			msg->subtype = hdr->opcode_minor;
			msg->length = hdr->length * 4;

			err += sizeof(*hdr);
			*dst = msg;
		}
	}

	return err;
}

int xim_msg_new(xim_msg_t **msg, const xim_msg_type_t type)
{
	xim_msg_t *m;

	if (!msg) {
		return -EINVAL;
	}

	if (!(m = calloc(1, _msg_info[type].size))) {
		return -ENOMEM;
	}

	m->type = type;
	m->subtype = 0;

	*msg = m;
	return 0;
}

static int encode_XIM_ERROR(xim_msg_error_t *src, uint8_t *dst, const size_t dst_size)
{
	struct XIM_ERROR *raw;
	int data_len;
	int padded_len;
	int padding_len;

	if (!src || !dst) {
		return -EINVAL;
	}

	data_len = sizeof(*raw) + src->detail_len;
	padding_len = PAD(data_len);
	padded_len = data_len + padding_len;

	if (dst_size < padded_len) {
		return -EMSGSIZE;
	}

	raw = (struct XIM_ERROR*)dst;
	raw->im = src->im;
	raw->ic = src->ic;
	raw->flag = src->flags;
	raw->error_code = src->error;
	raw->detail_len = src->detail_len;
	raw->detail_type = src->detail_type;
	memmove(raw->detail, src->detail, src->detail_len);
	memset(raw->detail + src->detail_len, 0, padding_len);

	return padded_len;
}

static int encode_XIM_CONNECT_REPLY(xim_msg_connect_reply_t *src, uint8_t *dst, const size_t dst_size)
{
	struct XIM_CONNECT_REPLY *raw;

	if (!src || !dst) {
		return -EINVAL;
	}

	if (dst_size < sizeof(*raw)) {
		return -EMSGSIZE;
	}

	raw = (struct XIM_CONNECT_REPLY*)dst;
	raw->server_ver.major = src->server_ver.major;
	raw->server_ver.minor = src->server_ver.minor;

	return sizeof(*raw);
}

static int encode_XIM_OPEN_REPLY(xim_msg_open_reply_t *src, uint8_t *dst, const size_t dst_size)
{
	struct XIM_OPEN_REPLY *raw;
	size_t required_size;
	uint16_t num_imattrs;
	uint16_t len_imattrs;
	uint16_t num_icattrs;
	uint16_t len_icattrs;
	int encoded_len;
	int i;

	if (!src || !dst) {
		return -EINVAL;
	}

	required_size = 4 * sizeof(uint16_t);
	num_imattrs = num_icattrs = 0;
	len_imattrs = len_icattrs = 0;

	if (src->im_attrs) {
		for (i = 0; src->im_attrs[i].name; i++) {
			size_t attr_len;

			fprintf(stderr, "XIMATTR: %s\n", src->im_attrs[i].name);
			attr_len = 6 + strlen(src->im_attrs[i].name);
			required_size += attr_len + PAD(attr_len);
			num_imattrs++;
			len_imattrs += attr_len + PAD(attr_len);
		}
	}

	if (src->ic_attrs) {
		for (i = 0; src->ic_attrs[i].name; i++) {
			size_t attr_len;

			fprintf(stderr, "XICATTR: %s\n", src->ic_attrs[i].name);
			attr_len = 6 + strlen(src->ic_attrs[i].name);
			required_size += attr_len + PAD(attr_len);
			num_icattrs++;
			len_icattrs += attr_len + PAD(attr_len);
		}
	}

	if (dst_size < required_size) {
		return -EMSGSIZE;
	}

	raw = (struct XIM_OPEN_REPLY*)dst;
	raw->im_id = src->id;
	*(uint16_t*)(raw + 1) = len_imattrs;
	encoded_len = sizeof(*raw) + sizeof(uint16_t);

	for (i = 0; i < num_imattrs; i++) {
		int len;

		if ((len = encode_ATTR(&src->im_attrs[i],
		                       dst + encoded_len,
		                       dst_size - encoded_len)) < 0) {
			return len;
		}
		encoded_len += len;
	}

	*(uint16_t*)(dst + encoded_len + 0) = len_icattrs;
	*(uint16_t*)(dst + encoded_len + 2) = 0;
	encoded_len += 4;

	for (i = 0; i < num_icattrs; i++) {
		int len;

		if ((len = encode_ATTR(&src->ic_attrs[i],
		                       dst + encoded_len,
		                       dst_size - encoded_len)) < 0) {
			return len;
		}
		encoded_len += len;
	}

	return encoded_len;
}

static int encode_XIM_QUERY_EXTENSION_REPLY(xim_msg_query_extension_reply_t *src,
                                            uint8_t *dst, const size_t dst_size)
{
	struct XIM_QUERY_EXTENSION_REPLY *raw;
	int len_exts;
	int i;

	if (!src || !dst) {
		return -EINVAL;
	}

	if (dst_size < sizeof(*raw)) {
		return -ENOMEM;
	}

	raw = (struct XIM_QUERY_EXTENSION_REPLY*)dst;
	raw->im = src->im;
	len_exts = 0;

	if (src->exts) {
		for (i = 0; src->exts[i].name; i++) {
			int ext_len;

			if ((ext_len = encode_EXT(&src->exts[i], raw->exts + len_exts,
			                          dst_size - sizeof(*raw) - len_exts)) < 0) {
				return ext_len;
			}

			len_exts += ext_len;
		}
	}
	raw->exts_len = len_exts;

	return len_exts + sizeof(*raw);
}

static int encode_XIM_ENCODING_NEGOTIATION_REPLY(xim_msg_encoding_negotiation_reply_t *src,
                                                 uint8_t *dst, const size_t dst_size)
{
	struct XIM_ENCODING_NEGOTIATION_REPLY *raw;

	if (!src || !dst) {
		return -EINVAL;
	}

	if (dst_size < sizeof(*raw)) {
		return -ENOMEM;
	}

	raw = (struct XIM_ENCODING_NEGOTIATION_REPLY*)dst;
	raw->im = src->im;
	raw->category = src->category;
	raw->encoding = src->encoding;
	raw->unused = 0;

	return sizeof(*raw);
}

static int encode_XIM_GET_IM_VALUES_REPLY(xim_msg_get_im_values_reply_t *src,
                                          uint8_t *dst, const size_t dst_size)
{
	struct XIM_GET_IM_VALUES_REPLY *raw;
	int required_size;
	int encoded_len;
	int i;

	if (!src || !dst) {
		return -EINVAL;
	}

	if (!src->values && src->num_values > 0) {
		return -EBADMSG;
	}

	required_size = sizeof(*raw);
	for (i = 0; i < src->num_values; i++) {
		int value_len;

		value_len = sizeof(uint16_t) + sizeof(uint16_t) + src->values[i]->len;
		required_size += value_len + PAD(value_len);
	}

	if (dst_size < required_size) {
		return -EMSGSIZE;
	}

	raw = (struct XIM_GET_IM_VALUES_REPLY*)dst;
	raw->im = src->im;
	encoded_len = sizeof(*raw);
	for (i = 0; i < src->num_values; i++) {
		int value_len;

		value_len = encode_ATTRIBUTE(src->values[i], dst + encoded_len,
		                             dst_size - encoded_len);

		if (value_len < 0) {
			return value_len;
		}

		encoded_len += value_len;
	}

	raw->len_values = encoded_len - sizeof(*raw);

	return encoded_len;
}

static int encode_XIM_SET_IM_VALUES_REPLY(xim_msg_set_im_values_reply_t *src,
                                          uint8_t *dst, const size_t dst_size)
{
	struct XIM_SET_IM_VALUES_REPLY *raw;

	if (!src || !dst) {
		return -EINVAL;
	}

	if (dst_size < sizeof(*raw)) {
		return -EMSGSIZE;
	}

	raw = (struct XIM_SET_IM_VALUES_REPLY*)dst;
	raw->im = src->im;
	raw->unused = 0;

	return sizeof(*raw);
}

static int encode_XIM_GET_IC_VALUES_REPLY(xim_msg_get_ic_values_reply_t *src,
                                          uint8_t *dst, const size_t dst_size)
{
	struct XIM_GET_IC_VALUES_REPLY *raw;
	int required_size;
	int encoded_len;
	int i;

	if (!src || !dst) {
		return -EINVAL;
	}

	if (!src->values && src->num_values > 0) {
		return -EBADMSG;
	}

	required_size = sizeof(*raw);
	for (i = 0; i < src->num_values; i++) {
		int value_len;

		value_len = sizeof(uint16_t) + sizeof(uint16_t) + src->values[i]->len;
		required_size += value_len + PAD(value_len);
	}

	if (dst_size < required_size) {
		return -EMSGSIZE;
	}

	raw = (struct XIM_GET_IC_VALUES_REPLY*)dst;
	raw->im = src->im;
	raw->ic = src->ic;
	encoded_len = sizeof(*raw);
	for (i = 0; i < src->num_values; i++) {
		int value_len;

		value_len = encode_ATTRIBUTE(src->values[i], dst + encoded_len,
		                             dst_size - encoded_len);

		if (value_len < 0) {
			return value_len;
		}

		encoded_len += value_len;
	}

	raw->len_values = encoded_len - sizeof(*raw);

	return encoded_len;
}

static int encode_XIM_SET_IC_VALUES_REPLY(xim_msg_set_ic_values_reply_t *src,
                                          uint8_t *dst, const size_t dst_size)
{
	struct XIM_SET_IC_VALUES_REPLY *raw;

	if (!src || !dst) {
		return -EINVAL;
	}

	if (dst_size < sizeof(*raw)) {
		return -EMSGSIZE;
	}

	raw = (struct XIM_SET_IC_VALUES_REPLY*)dst;
	raw->im = src->im;
	raw->ic = src->ic;

	return sizeof(*raw);
}

static int encode_XIM_CREATE_IC_REPLY(xim_msg_create_ic_reply_t *src, uint8_t *dst, const size_t dst_size)
{
	struct XIM_CREATE_IC_REPLY *raw;

	if (!src || !dst) {
		return -EINVAL;
	}

	if (dst_size < sizeof(*raw)) {
		return -ENOMEM;
	}

	raw = (struct XIM_CREATE_IC_REPLY*)dst;
	raw->im = src->im;
	raw->ic = src->ic;

	return sizeof(*raw);
}

int xim_msg_encode(xim_msg_t *src, uint8_t *dst, const size_t dst_size)
{
	struct XIM_PACKET *hdr;
	int payload_len;
	int ret_val;

	if (!src || !dst) {
		return -EINVAL;
	}

	if (dst_size < sizeof(struct XIM_PACKET)) {
		return -ENOMEM;
	}

	hdr = (struct XIM_PACKET*)dst;
	ret_val = -EFAULT;
	payload_len = -1;

	switch (src->type) {
	case XIM_ERROR:
		payload_len = encode_XIM_ERROR((xim_msg_error_t*)src,
		                               (uint8_t*)(hdr + 1),
		                               dst_size - sizeof(*hdr));
		break;

	case XIM_CONNECT_REPLY:
		payload_len = encode_XIM_CONNECT_REPLY((xim_msg_connect_reply_t*)src,
		                                       (uint8_t*)(hdr + 1),
		                                       dst_size - sizeof(*hdr));
		break;

	case XIM_OPEN_REPLY:
		payload_len = encode_XIM_OPEN_REPLY((xim_msg_open_reply_t*)src,
		                                    (uint8_t*)(hdr + 1),
		                                    dst_size - sizeof(*hdr));
		break;

	case XIM_QUERY_EXTENSION_REPLY:
		payload_len = encode_XIM_QUERY_EXTENSION_REPLY((xim_msg_query_extension_reply_t*)src,
		                                               (uint8_t*)(hdr + 1),
		                                               dst_size - sizeof(*hdr));
		break;

	case XIM_ENCODING_NEGOTIATION_REPLY:
		payload_len = encode_XIM_ENCODING_NEGOTIATION_REPLY((xim_msg_encoding_negotiation_reply_t*)src,
		                                                    (uint8_t*)(hdr + 1),
		                                                    dst_size - sizeof(*hdr));
		break;

	case XIM_GET_IM_VALUES_REPLY:
		payload_len = encode_XIM_GET_IM_VALUES_REPLY((xim_msg_get_im_values_reply_t*)src,
		                                             (uint8_t*)(hdr + 1),
		                                             dst_size - sizeof(*hdr));
		break;

	case XIM_SET_IM_VALUES_REPLY:
		payload_len = encode_XIM_SET_IM_VALUES_REPLY((xim_msg_set_im_values_reply_t*)src,
		                                             (uint8_t*)(hdr + 1),
		                                             dst_size - sizeof(*hdr));
		break;

	case XIM_GET_IC_VALUES_REPLY:
		payload_len = encode_XIM_GET_IC_VALUES_REPLY((xim_msg_get_ic_values_reply_t*)src,
		                                             (uint8_t*)(hdr + 1),
		                                             dst_size - sizeof(*hdr));
		break;

	case XIM_SET_IC_VALUES_REPLY:
		payload_len = encode_XIM_SET_IC_VALUES_REPLY((xim_msg_set_ic_values_reply_t*)src,
		                                             (uint8_t*)(hdr + 1),
		                                             dst_size - sizeof(*hdr));
		break;

	case XIM_CREATE_IC_REPLY:
		payload_len = encode_XIM_CREATE_IC_REPLY((xim_msg_create_ic_reply_t*)src,
		                                         (uint8_t*)(hdr + 1),
		                                         dst_size - sizeof(*hdr));
		break;

	default:
		ret_val = -ENOSYS;
		break;
	}

	if (payload_len >= 0) {
		hdr->opcode_major = src->type;
		hdr->opcode_minor = src->subtype;
		hdr->length = payload_len / 4;

		ret_val = sizeof(*hdr) + payload_len;
	}

	return ret_val;
}
